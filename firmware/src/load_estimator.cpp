#include "load_estimator.h"

float LoadEstimator::kilograms(long rawCounts) const {
  if (!ready()) return 0.0f;
  // Convert before subtracting so an extreme signed raw-count difference cannot
  // overflow the platform's long integer type.
  const float result = (static_cast<float>(rawCounts) -
                        static_cast<float>(zeroCounts_)) / countsPerKg_;
  return std::isfinite(result) ? result : 0.0f;
}
