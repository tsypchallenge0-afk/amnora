#include "sensor_hub.h"

#include <Arduino.h>
#include <Wire.h>
#include <cmath>

#include "config.h"
#include "pins.h"

namespace {
constexpr uint8_t kUpperImuAddress = 0x68;
constexpr uint8_t kPelvisImuAddress = 0x69;
constexpr uint8_t kEnvironmentAddress = 0x44;
constexpr float kRadiansToDegrees = 57.2957795f;
constexpr uint32_t kEnvironmentStaleMs = 5000;
}  // namespace

bool SensorHub::begin() {
  Wire.begin(pins::SENSOR_I2C_SDA, pins::SENSOR_I2C_SCL);
  Wire.setClock(100000);

  upperReady_ = upperImu_.begin(kUpperImuAddress, &Wire);
  pelvisReady_ = pelvisImu_.begin(kPelvisImuAddress, &Wire);
  environmentReady_ = environment_.begin(kEnvironmentAddress);
  pulseReady_ = pulse_.begin();

  if (upperReady_) upperImu_.setFilterBandwidth(MPU6050_BAND_21_HZ);
  if (pelvisReady_) pelvisImu_.setFilterBandwidth(MPU6050_BAND_21_HZ);

  leftLoadReady_ = pins::HX711_LEFT_DOUT >= 0 && pins::HX711_LEFT_SCK >= 0;
  rightLoadReady_ = pins::HX711_RIGHT_DOUT >= 0 && pins::HX711_RIGHT_SCK >= 0;
  if (leftLoadReady_) {
    leftHx711_.begin(pins::HX711_LEFT_DOUT, pins::HX711_LEFT_SCK);
    leftLoad_.setCalibration(config::LOAD_COUNTS_PER_KG_LEFT);
  }
  if (rightLoadReady_) {
    rightHx711_.begin(pins::HX711_RIGHT_DOUT, pins::HX711_RIGHT_SCK);
    rightLoad_.setCalibration(config::LOAD_COUNTS_PER_KG_RIGHT);
  }

  latest_.health.upperImu = upperReady_;
  latest_.health.pelvisImu = pelvisReady_;
  latest_.health.environment = environmentReady_;
  latest_.health.pulse = pulseReady_;
  latest_.health.leftLoadCell = leftLoadReady_;
  latest_.health.rightLoadCell = rightLoadReady_;
  return upperReady_ || pelvisReady_ || environmentReady_ || leftLoadReady_ ||
         rightLoadReady_ || pulseReady_;
}

WorkloadSnapshot SensorHub::update(uint32_t nowMs) {
  updateImuPair(nowMs);
  updateEnvironment(nowMs);
  updateLoads(nowMs);
  latest_.pulseBpm = pulse_.update(nowMs);
  latest_.health.pulse = pulseReady_;
  return latest_;
}

bool SensorHub::updateImuPair(uint32_t nowMs) {
  sensors_event_t upperAccel, upperGyro, upperTemp;
  sensors_event_t pelvisAccel, pelvisGyro, pelvisTemp;
  const bool upperRead = upperReady_ && upperImu_.getEvent(&upperAccel, &upperGyro, &upperTemp);
  const bool pelvisRead = pelvisReady_ && pelvisImu_.getEvent(&pelvisAccel, &pelvisGyro, &pelvisTemp);
  const bool upperOk = upperRead && isfinite(upperAccel.acceleration.x) &&
      isfinite(upperAccel.acceleration.y) && isfinite(upperAccel.acceleration.z) &&
      isfinite(upperGyro.gyro.y);
  const bool pelvisOk = pelvisRead && isfinite(pelvisAccel.acceleration.x) &&
      isfinite(pelvisAccel.acceleration.y) && isfinite(pelvisAccel.acceleration.z) &&
      isfinite(pelvisGyro.gyro.y);
  latest_.health.upperImu = upperOk;
  latest_.health.pelvisImu = pelvisOk;

  if (!upperOk || !pelvisOk) {
    latest_.relativeTrunkAngleDeg.valid = false;
    return false;
  }

  // Sagittal-plane pitch for the documented mounting orientation. Re-map the
  // axes here if the sensor boards are rotated relative to one another.
  const float upperAccelPitch = atan2f(
      -upperAccel.acceleration.x,
      sqrtf(upperAccel.acceleration.y * upperAccel.acceleration.y +
            upperAccel.acceleration.z * upperAccel.acceleration.z)) * kRadiansToDegrees;
  const float pelvisAccelPitch = atan2f(
      -pelvisAccel.acceleration.x,
      sqrtf(pelvisAccel.acceleration.y * pelvisAccel.acceleration.y +
            pelvisAccel.acceleration.z * pelvisAccel.acceleration.z)) * kRadiansToDegrees;

  if (!upperFilter_.ready()) upperFilter_.begin(upperAccelPitch, nowMs);
  if (!pelvisFilter_.ready()) pelvisFilter_.begin(pelvisAccelPitch, nowMs);
  upperPitchDeg_ = upperFilter_.update(upperAccelPitch,
      upperGyro.gyro.y * kRadiansToDegrees, nowMs);
  pelvisPitchDeg_ = pelvisFilter_.update(pelvisAccelPitch,
      pelvisGyro.gyro.y * kRadiansToDegrees, nowMs);

  if (!postureCalibration_.ready()) {
    latest_.relativeTrunkAngleDeg.valid = false;
    return true;
  }

  latest_.relativeTrunkAngleDeg.value =
      postureCalibration_.relativeAngle(upperPitchDeg_, pelvisPitchDeg_);
  latest_.relativeTrunkAngleDeg.timestampMs = nowMs;
  latest_.relativeTrunkAngleDeg.valid = true;
  return true;
}

void SensorHub::updateEnvironment(uint32_t nowMs) {
  if (!environmentReady_) {
    latest_.health.environment = false;
    latest_.ambientTemperatureC.valid = false;
    latest_.relativeHumidityPct.valid = false;
    return;
  }
  if (static_cast<uint32_t>(nowMs - lastEnvironmentMs_) >= config::ENVIRONMENT_SAMPLE_PERIOD_MS) {
    float temperature = NAN;
    float humidity = NAN;
    const bool readOk = environment_.readBoth(&temperature, &humidity);
    const bool valid = readOk && isfinite(temperature) && isfinite(humidity) &&
                       temperature >= -40.0f && temperature <= 125.0f &&
                       humidity >= 0.0f && humidity <= 100.0f;
    latest_.ambientTemperatureC = {temperature, nowMs, valid};
    latest_.relativeHumidityPct = {humidity, nowMs, valid};
    latest_.health.environment = valid;
    lastEnvironmentMs_ = nowMs;
  } else if (static_cast<uint32_t>(nowMs - lastEnvironmentMs_) > kEnvironmentStaleMs) {
    latest_.ambientTemperatureC.valid = false;
    latest_.relativeHumidityPct.valid = false;
    latest_.health.environment = false;
  }
}

void SensorHub::updateLoads(uint32_t nowMs) {
  if (leftLoadReady_ && leftHx711_.is_ready()) {
    const long raw = leftHx711_.read();
    lastLeftLoadMs_ = nowMs;
    latest_.leftHandleRawCounts = {raw, true};
    if (leftLoad_.ready()) {
      latest_.leftHandleLoadKg = {leftLoad_.kilograms(raw), nowMs, true};
    } else {
      latest_.leftHandleLoadKg.valid = false;
    }
  }
  if (rightLoadReady_ && rightHx711_.is_ready()) {
    const long raw = rightHx711_.read();
    lastRightLoadMs_ = nowMs;
    latest_.rightHandleRawCounts = {raw, true};
    if (rightLoad_.ready()) {
      latest_.rightHandleLoadKg = {rightLoad_.kilograms(raw), nowMs, true};
    } else {
      latest_.rightHandleLoadKg.valid = false;
    }
  }
  latest_.health.leftLoadCell = leftLoadReady_ && lastLeftLoadMs_ != 0 &&
      static_cast<uint32_t>(nowMs - lastLeftLoadMs_) <= 2000;
  latest_.health.rightLoadCell = rightLoadReady_ && lastRightLoadMs_ != 0 &&
      static_cast<uint32_t>(nowMs - lastRightLoadMs_) <= 2000;
  latest_.health.leftLoadCalibrated = leftLoad_.ready();
  latest_.health.rightLoadCalibrated = rightLoad_.ready();
  if (lastLeftLoadMs_ == 0 || static_cast<uint32_t>(nowMs - lastLeftLoadMs_) > 1000) {
    latest_.leftHandleRawCounts.valid = false;
    latest_.leftHandleLoadKg.valid = false;
  }
  if (lastRightLoadMs_ == 0 || static_cast<uint32_t>(nowMs - lastRightLoadMs_) > 1000) {
    latest_.rightHandleRawCounts.valid = false;
    latest_.rightHandleLoadKg.valid = false;
  }
}

bool SensorHub::captureNeutral() {
  if (!latest_.health.upperImu || !latest_.health.pelvisImu) return false;
  return postureCalibration_.captureNeutral(upperPitchDeg_, pelvisPitchDeg_, true);
}

bool SensorHub::tareHandles() {
  bool taredAtLeastOne = false;
  if (leftLoadReady_ && leftHx711_.is_ready()) {
    leftLoad_.tare(leftHx711_.read());
    taredAtLeastOne = true;
  }
  if (rightLoadReady_ && rightHx711_.is_ready()) {
    rightLoad_.tare(rightHx711_.read());
    taredAtLeastOne = true;
  }
  return taredAtLeastOne;
}
