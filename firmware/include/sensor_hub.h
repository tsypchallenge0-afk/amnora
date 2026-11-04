#pragma once

#include <cstdint>

#include <Adafruit_MPU6050.h>
#include <Adafruit_SHT31.h>
#include <HX711.h>

#include "config.h"
#include "load_estimator.h"
#include "orientation.h"
#include "pulse_monitor.h"
#include "types.h"

enum class CalibrationStatus : uint8_t {
  Idle,
  Collecting,
  Complete,
  Failed
};

class SensorHub {
 public:
  bool begin();
  WorkloadSnapshot update(uint32_t nowMs);
  bool startNeutralCalibration(uint32_t nowMs);
  bool startTareCalibration(uint32_t nowMs);
  CalibrationStatus neutralCalibrationStatus() const { return neutralCalibrationStatus_; }
  CalibrationStatus tareCalibrationStatus() const { return tareCalibrationStatus_; }

 private:
  bool updateImuPair(uint32_t nowMs);
  void updateEnvironment(uint32_t nowMs);
  void updateLoads(uint32_t nowMs);
  void updateNeutralCalibration(uint32_t nowMs);
  void updateTareCalibration(uint32_t nowMs);

  Adafruit_MPU6050 upperImu_;
  Adafruit_MPU6050 pelvisImu_;
  Adafruit_SHT31 environment_;
  HX711 leftHx711_;
  HX711 rightHx711_;
  PulseMonitor pulse_;

  ComplementaryAngleFilter upperFilter_{config::IMU_COMPLEMENTARY_ACCEL_WEIGHT};
  ComplementaryAngleFilter pelvisFilter_{config::IMU_COMPLEMENTARY_ACCEL_WEIGHT};
  RelativePostureCalibration postureCalibration_;
  LoadEstimator leftLoad_{0.0f};
  LoadEstimator rightLoad_{0.0f};

  WorkloadSnapshot latest_{};
  float upperPitchDeg_ = 0.0f;
  float pelvisPitchDeg_ = 0.0f;
  bool upperReady_ = false;
  bool pelvisReady_ = false;
  bool environmentReady_ = false;
  bool leftLoadReady_ = false;
  bool rightLoadReady_ = false;
  bool pulseReady_ = false;
  uint32_t lastEnvironmentMs_ = 0;
  uint32_t lastLeftLoadMs_ = 0;
  uint32_t lastRightLoadMs_ = 0;
  uint32_t neutralCalibrationStartedMs_ = 0;
  uint32_t tareCalibrationStartedMs_ = 0;
  uint8_t neutralCalibrationCount_ = 0;
  float neutralUpperSum_ = 0.0f;
  float neutralPelvisSum_ = 0.0f;
  float neutralRelativeSum_ = 0.0f;
  float neutralRelativeSquareSum_ = 0.0f;
  CalibrationStatus neutralCalibrationStatus_ = CalibrationStatus::Idle;
  CalibrationStatus tareCalibrationStatus_ = CalibrationStatus::Idle;
  bool tareLeftActive_ = false;
  bool tareRightActive_ = false;
  uint8_t tareLeftCount_ = 0;
  uint8_t tareRightCount_ = 0;
  int64_t tareLeftSum_ = 0;
  int64_t tareRightSum_ = 0;
};
