#include "posture.h"

void PostureTracker::reset(uint32_t nowMs) {
  aboveThreshold_ = false;
  aboveSinceMs_ = nowMs;
  dwellMs_ = 0;
  lastUpdateMs_ = nowMs;
}

bool PostureTracker::update(float relativeAngleDeg, bool valid, uint32_t nowMs,
                            float thresholdDeg, uint32_t dwellThresholdMs) {
  if (!valid) {
    // A gap breaks continuity: don't count missing sensor time as exposure.
    aboveThreshold_ = false;
    dwellMs_ = 0;
    lastUpdateMs_ = nowMs;
    return false;
  }

  if (relativeAngleDeg >= thresholdDeg) {
    if (!aboveThreshold_) {
      aboveThreshold_ = true;
      aboveSinceMs_ = nowMs;
    }
    dwellMs_ = nowMs - aboveSinceMs_;  // rollover-safe unsigned subtraction
    lastUpdateMs_ = nowMs;
    return dwellMs_ >= dwellThresholdMs;
  }

  aboveThreshold_ = false;
  dwellMs_ = 0;
  lastUpdateMs_ = nowMs;
  return false;
}
