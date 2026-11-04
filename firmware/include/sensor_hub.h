#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_SHT31.h>
#include <HX711.h>

#include "config.h"
#include "load_estimator.h"
#include "orientation.h"
#include "pulse_monitor.h"
#include "types.h"

class SensorHub {
 public:
  bool begin();
  WorkloadSnapshot update(uint32_t nowMs);
  bool captureNeutral();
  bool tareHandles();

 private:
  bool updateImuPair(uint32_t nowMs);
  void updateEnvironment(uint32_t nowMs);
  void updateLoads(uint32_t nowMs);

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
};
