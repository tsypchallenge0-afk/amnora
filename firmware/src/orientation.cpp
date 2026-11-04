#include "orientation.h"

void ComplementaryAngleFilter::begin(float accelAngleDeg, uint32_t nowMs) {
  angleDeg_ = accelAngleDeg;
  previousMs_ = nowMs;
  ready_ = true;
}

float ComplementaryAngleFilter::update(float accelAngleDeg,
                                       float gyroRateDegPerSec,
                                       uint32_t nowMs) {
  if (!ready_) {
    begin(accelAngleDeg, nowMs);
    return angleDeg_;
  }
  const uint32_t elapsedMs = nowMs - previousMs_;
  previousMs_ = nowMs;
  const float dt = static_cast<float>(elapsedMs) / 1000.0f;
  const float gyroPrediction = angleDeg_ + gyroRateDegPerSec * dt;
  angleDeg_ = (1.0f - accelWeight_) * gyroPrediction + accelWeight_ * accelAngleDeg;
  return angleDeg_;
}

bool RelativePostureCalibration::captureNeutral(float upperBackAngleDeg,
                                                 float pelvisAngleDeg,
                                                 bool bothValid) {
  if (!bothValid) return false;
  neutralOffsetDeg_ = upperBackAngleDeg - pelvisAngleDeg;
  ready_ = true;
  return true;
}

float RelativePostureCalibration::relativeAngle(float upperBackAngleDeg,
                                                 float pelvisAngleDeg) const {
  if (!ready_) return 0.0f;
  return (upperBackAngleDeg - pelvisAngleDeg) - neutralOffsetDeg_;
}
