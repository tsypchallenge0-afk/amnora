#include <Arduino.h>
#include "config.h"
#include "load_estimator.h"
#include "pins.h"
#include "posture.h"
#include "sos_controller.h"
#include "types.h"

namespace {
PostureTracker posture;
LoadEstimator leftLoad;
LoadEstimator rightLoad;
SosController sos;
uint32_t lastSampleMs = 0;

void reportConfiguration() {
  Serial.println("AGRI-SAFE firmware starting");
  Serial.println("Sensor acquisition drivers are selected per confirmed breakout variant.");
  Serial.printf("T-Call modem UART ESP32 RX=%d TX=%d; modem control pins reserved.\n",
                pins::MODEM_RX, pins::MODEM_TX);
  Serial.println("Unassigned GPIO values in pins.h must be reviewed before attaching hardware.");
}

void sampleAndProcess(uint32_t nowMs) {
  // Sensor-specific acquisition is intentionally isolated from decision logic.
  // Replace these invalid samples with driver output after selecting exact modules.
  WorkloadSnapshot sample{};
  sample.relativeTrunkAngleDeg.valid = false;
  sample.leftHandleLoadKg.valid = false;
  sample.rightHandleLoadKg.valid = false;
  sample.pulseBpm.valid = false;
  sample.ambientTemperatureC.valid = false;
  sample.relativeHumidityPct.valid = false;

  sample.postureReminder = posture.update(
      sample.relativeTrunkAngleDeg.value,
      sample.relativeTrunkAngleDeg.valid,
      nowMs,
      config::POSTURE_WARNING_DEG,
      config::POSTURE_WARNING_DWELL_MS);
  sample.bendingDwellMs = posture.dwellMs();
  sample.loadReminder =
      (sample.leftHandleLoadKg.valid && sample.leftHandleLoadKg.value >= config::HANDLE_LOAD_WARNING_KG) ||
      (sample.rightHandleLoadKg.valid && sample.rightHandleLoadKg.value >= config::HANDLE_LOAD_WARNING_KG);

  // Wire feedback patterns to a nonblocking output scheduler in the integration layer.
  (void)sample;
}
}  // namespace

void setup() {
  Serial.begin(config::SERIAL_BAUD);
  delay(50);
  reportConfiguration();
  posture.reset(millis());
  lastSampleMs = millis();
}

void loop() {
  const uint32_t nowMs = millis();
  if (static_cast<uint32_t>(nowMs - lastSampleMs) >= config::SAMPLE_PERIOD_MS) {
    lastSampleMs = nowMs;
    sampleAndProcess(nowMs);
  }

  sos.tick(nowMs, config::SOS_CANCEL_WINDOW_MS, config::SOS_RETRY_DELAY_MS,
           config::SOS_MAX_ATTEMPTS);
  // Button, sensor-driver, feedback, and asynchronous modem adapters attach here.
}
