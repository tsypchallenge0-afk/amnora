#pragma once

class LoadEstimator {
 public:
  explicit LoadEstimator(float countsPerKg = 1.0f) : countsPerKg_(countsPerKg) {}
  void setCalibration(float countsPerKg) { countsPerKg_ = countsPerKg; }
  void tare(long rawCounts) { zeroCounts_ = rawCounts; tared_ = true; }
  bool ready() const { return tared_ && countsPerKg_ != 0.0f; }
  float kilograms(long rawCounts) const;

 private:
  float countsPerKg_;
  long zeroCounts_ = 0;
  bool tared_ = false;
};
