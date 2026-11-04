#pragma once

#include <cstdint>

class PostureTracker {
 public:
  void reset(uint32_t nowMs);
  bool update(float relativeAngleDeg, bool valid, uint32_t nowMs,
              float thresholdDeg, float releaseThresholdDeg,
              uint32_t dwellThresholdMs);
  uint32_t dwellMs() const { return dwellMs_; }

 private:
  bool aboveThreshold_ = false;
  uint32_t aboveSinceMs_ = 0;
  uint32_t dwellMs_ = 0;
};
