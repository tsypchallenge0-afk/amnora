#pragma once

#include <cmath>

class LoadEstimator {
 public:
  explicit LoadEstimator(float countsPerKg = 1.0f) : countsPerKg_(countsPerKg) {}
  void setCalibration(float countsPerKg) {
    countsPerKg_ = std::isfinite(countsPerKg) ? countsPerKg : 0.0f;
  }
  void tare(long rawCounts) { zeroCounts_ = rawCounts; tared_ = true; }
  bool ready() const { return tared_ && std::isfinite(countsPerKg_) && std::fabs(countsPerKg_) > 0.0001f; }
  float kilograms(long rawCounts) const;

 private:
  float countsPerKg_;
  long zeroCounts_ = 0;
  bool tared_ = false;
};
