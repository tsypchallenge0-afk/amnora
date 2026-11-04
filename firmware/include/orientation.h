#pragma once

#include <cstdint>

// One-axis complementary filter. Supply accel angle and matching gyro axis.
class ComplementaryAngleFilter {
 public:
  explicit ComplementaryAngleFilter(float accelWeight = 0.02f)
      : accelWeight_(accelWeight) {}
  void begin(float accelAngleDeg, uint32_t nowMs);
  float update(float accelAngleDeg, float gyroRateDegPerSec, uint32_t nowMs);
  bool ready() const { return ready_; }

 private:
  float accelWeight_;
  float angleDeg_ = 0.0f;
  uint32_t previousMs_ = 0;
  bool ready_ = false;
};

class RelativePostureCalibration {
 public:
  bool captureNeutral(float upperBackAngleDeg, float pelvisAngleDeg,
                      bool bothValid);
  float relativeAngle(float upperBackAngleDeg, float pelvisAngleDeg) const;
  bool ready() const { return ready_; }

 private:
  float neutralOffsetDeg_ = 0.0f;
  bool ready_ = false;
};
