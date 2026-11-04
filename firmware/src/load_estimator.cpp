#include "load_estimator.h"

float LoadEstimator::kilograms(long rawCounts) const {
  if (!ready()) return 0.0f;
  return static_cast<float>(rawCounts - zeroCounts_) / countsPerKg_;
}
