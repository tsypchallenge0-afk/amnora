#include <Arduino.h>

#include "button_debouncer.h"
#include "config.h"
#include "feedback.h"
#include "pins.h"
#include "posture.h"
#include "sensor_hub.h"
#include "sim800l_transport.h"
#include "sos_controller.h"
#include "types.h"

namespace {
SensorHub sensors;
PostureTracker posture;
SosController sos;
Sim800lTransport modem;
FeedbackController feedback;
ButtonDebouncer sosButton(config::BUTTON_DEBOUNCE_MS);
ButtonDebouncer cancelButton(config::BUTTON_DEBOUNCE_MS);
WorkloadSnapshot latest{};
uint32_t lastSampleMs = 0;
uint32_t lastReportMs = 0;
bool networkRequestActive = false;
bool smsRequestActive = false;
bool previousPostureReminder = false;
bool previousLoadReminder = false;
CalibrationStatus previousNeutralCalibrationStatus = CalibrationStatus::Idle;
CalibrationStatus previousTareCalibrationStatus = CalibrationStatus::Idle;
constexpr char kEmergencyMessage[] =
    "AGRI-SAFE emergency request. Please contact the worker. This device has no GPS location.";

void printHelp() {
  Serial.println(F("Commands: c = average neutral posture; z = average tare unloaded handles; ? = help"));
  Serial.println(F("Calibration collects multiple samples and continues servicing SOS and sensors."));
  Serial.println(F("SOS is activated only by the configured physical button."));
}

void reportCalibrationChanges() {
  const CalibrationStatus neutralStatus = sensors.neutralCalibrationStatus();
  if (neutralStatus != previousNeutralCalibrationStatus) {
    if (neutralStatus == CalibrationStatus::Complete) {
      Serial.println(F("Neutral calibration complete."));
    } else if (neutralStatus == CalibrationStatus::Failed) {
      Serial.println(F("Neutral calibration failed: hold still and retry when both IMUs are available."));
    }
    previousNeutralCalibrationStatus = neutralStatus;
  }

  const CalibrationStatus tareStatus = sensors.tareCalibrationStatus();
  if (tareStatus != previousTareCalibrationStatus) {
    if (tareStatus == CalibrationStatus::Complete) {
      Serial.println(F("Handle tare complete."));
    } else if (tareStatus == CalibrationStatus::Failed) {
      Serial.println(F("Handle tare failed or timed out; keep both handles unloaded and retry."));
    }
    previousTareCalibrationStatus = tareStatus;
  }
}

void reportHealth() {
  Serial.printf("sensor health: upperIMU=%s pelvisIMU=%s SHT31=%s MAX30102=%s leftHX711=%s (cal %s) rightHX711=%s (cal %s)\n",
                latest.health.upperImu ? "ok" : "unavailable",
                latest.health.pelvisImu ? "ok" : "unavailable",
                latest.health.environment ? "ok" : "unavailable",
                latest.health.pulse ? "initialized" : "unavailable",
                latest.health.leftLoadCell ? "active" : "unavailable",
                latest.health.leftLoadCalibrated ? "yes" : "no",
                latest.health.rightLoadCell ? "active" : "unavailable",
                latest.health.rightLoadCalibrated ? "yes" : "no");
}

void reportSnapshot(uint32_t nowMs) {
  Serial.printf("{\"ms\":%lu,\"trunk_deg\":", static_cast<unsigned long>(nowMs));
  if (latest.relativeTrunkAngleDeg.valid) Serial.print(latest.relativeTrunkAngleDeg.value, 2);
  else Serial.print(F("null"));
  Serial.print(F(",\"left_handle_kg\":"));
  if (latest.leftHandleLoadKg.valid) Serial.print(latest.leftHandleLoadKg.value, 2);
  else Serial.print(F("null"));
  Serial.print(F(",\"left_raw_counts\":"));
  if (latest.leftHandleRawCounts.valid) Serial.print(latest.leftHandleRawCounts.value);
  else Serial.print(F("null"));
  Serial.print(F(",\"right_handle_kg\":"));
  if (latest.rightHandleLoadKg.valid) Serial.print(latest.rightHandleLoadKg.value, 2);
  else Serial.print(F("null"));
  Serial.print(F(",\"right_raw_counts\":"));
  if (latest.rightHandleRawCounts.valid) Serial.print(latest.rightHandleRawCounts.value);
  else Serial.print(F("null"));
  Serial.print(F(",\"ambient_c\":"));
  if (latest.ambientTemperatureC.valid) Serial.print(latest.ambientTemperatureC.value, 2);
  else Serial.print(F("null"));
  Serial.print(F(",\"rh_pct\":"));
  if (latest.relativeHumidityPct.valid) Serial.print(latest.relativeHumidityPct.value, 2);
  else Serial.print(F("null"));
  Serial.print(F(",\"pulse_bpm\":"));
  if (latest.pulseBpm.valid) Serial.print(latest.pulseBpm.value, 1);
  else Serial.print(F("null"));
  Serial.printf(",\"posture_alert\":%s,\"load_alert\":%s}\n",
                latest.postureReminder ? "true" : "false",
                latest.loadReminder ? "true" : "false");
}

void handleSerialCommand(char command, uint32_t nowMs) {
  switch (command) {
    case 'c':
    case 'C':
      Serial.println(sensors.startNeutralCalibration(nowMs)
          ? F("Neutral calibration started. Stand comfortably neutral and hold still for about 2 seconds.")
          : F("Could not start neutral calibration: check IMU availability or an active calibration."));
      break;
    case 'z':
    case 'Z':
      Serial.println(sensors.startTareCalibration(nowMs)
          ? F("Handle tare started. Keep the configured handles unloaded until completion.")
          : F("Could not start tare: no HX711 pins are configured or tare is already active."));
      break;
    case '?':
      printHelp();
      break;
    default:
      break;
  }
}

void processSerial(uint32_t nowMs) {
  while (Serial.available() > 0) {
    handleSerialCommand(static_cast<char>(Serial.read()), nowMs);
  }
}

void processButtons(uint32_t nowMs) {
  if (pins::SOS_BUTTON >= 0 &&
      sosButton.update(digitalRead(pins::SOS_BUTTON) == LOW, nowMs)) {
    sos.trigger(nowMs);
    feedback.trigger(FeedbackEvent::SosRequested, nowMs);
    Serial.println(F("SOS requested. Cancel it with the physical cancel button during the confirmation window."));
  }
  if (pins::CANCEL_BUTTON >= 0 &&
      cancelButton.update(digitalRead(pins::CANCEL_BUTTON) == LOW, nowMs)) {
    sos.cancel();
    Serial.println(F("Cancel input received."));
  }
}

void processModemEvents(uint32_t nowMs) {
  switch (modem.takeEvent()) {
    case ModemEvent::NetworkReady:
      networkRequestActive = false;
      sos.modemReady(true, nowMs);
      Serial.println(F("Cellular registration available."));
      break;
    case ModemEvent::NetworkUnavailable:
      networkRequestActive = false;
      sos.modemReady(false, nowMs);
      feedback.trigger(FeedbackEvent::CommunicationFailure, nowMs);
      Serial.println(F("Cellular registration unavailable."));
      break;
    case ModemEvent::SmsAccepted:
      smsRequestActive = false;
      sos.smsResult(true, nowMs);
      feedback.trigger(FeedbackEvent::SmsAccepted, nowMs);
      Serial.println(F("SMS submission accepted by modem/network; delivery is not confirmed."));
      break;
    case ModemEvent::SmsFailed:
      smsRequestActive = false;
      sos.smsResult(false, nowMs);
      feedback.trigger(FeedbackEvent::CommunicationFailure, nowMs);
      Serial.println(F("SMS submission failed or timed out."));
      break;
    case ModemEvent::None:
      break;
  }
}

void driveSos(uint32_t nowMs) {
  if (sos.shouldCheckModem() && !networkRequestActive && modem.idle()) {
    networkRequestActive = modem.requestNetworkCheck(nowMs);
  }
  if (sos.shouldSubmitSms() && !smsRequestActive) {
    smsRequestActive = modem.sendSms(config::EMERGENCY_PHONE_NUMBER,
                                     kEmergencyMessage, nowMs);
  }
}

void sample(uint32_t nowMs) {
  latest = sensors.update(nowMs);
  latest.postureReminder = posture.update(
      latest.relativeTrunkAngleDeg.value,
      latest.relativeTrunkAngleDeg.valid,
      nowMs,
      config::POSTURE_WARNING_DEG,
      config::POSTURE_RESET_DEG,
      config::POSTURE_WARNING_DWELL_MS);
  latest.bendingDwellMs = posture.dwellMs();
  latest.loadReminder =
      (latest.leftHandleLoadKg.valid && latest.leftHandleLoadKg.value >= config::HANDLE_LOAD_WARNING_KG) ||
      (latest.rightHandleLoadKg.valid && latest.rightHandleLoadKg.value >= config::HANDLE_LOAD_WARNING_KG);
  if (latest.postureReminder && !previousPostureReminder) {
    feedback.trigger(FeedbackEvent::PostureReminder, nowMs);
  }
  if (latest.loadReminder && !previousLoadReminder) {
    feedback.trigger(FeedbackEvent::LoadReminder, nowMs);
  }
  previousPostureReminder = latest.postureReminder;
  previousLoadReminder = latest.loadReminder;
}
}  // namespace

void setup() {
  Serial.begin(config::SERIAL_BAUD);
  if (pins::SOS_BUTTON >= 0) pinMode(pins::SOS_BUTTON, INPUT_PULLUP);
  if (pins::CANCEL_BUTTON >= 0) pinMode(pins::CANCEL_BUTTON, INPUT_PULLUP);
  sensors.begin();
  modem.begin(millis());
  feedback.begin();
  posture.reset(millis());
  lastSampleMs = millis();
  lastReportMs = lastSampleMs;
  sample(lastSampleMs);
  printHelp();
  reportHealth();
}

void loop() {
  const uint32_t nowMs = millis();
  processSerial(nowMs);
  processButtons(nowMs);
  feedback.update(nowMs);
  modem.update(nowMs);
  processModemEvents(nowMs);

  if (static_cast<uint32_t>(nowMs - lastSampleMs) >= config::SAMPLE_PERIOD_MS) {
    lastSampleMs = nowMs;
    sample(nowMs);
  }

  sos.tick(nowMs, config::SOS_CANCEL_WINDOW_MS, config::SOS_RETRY_DELAY_MS,
           config::SOS_MODEM_TIMEOUT_MS, config::SOS_SMS_TIMEOUT_MS,
           config::SOS_MAX_ATTEMPTS);
  driveSos(nowMs);

  if (static_cast<uint32_t>(nowMs - lastReportMs) >= 500) {
    lastReportMs = nowMs;
    reportSnapshot(nowMs);
  }
  reportCalibrationChanges();
}
