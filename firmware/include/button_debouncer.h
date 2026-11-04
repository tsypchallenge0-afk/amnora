#pragma once

#include <cstdint>

class ButtonDebouncer {
 public:
  explicit ButtonDebouncer(uint32_t debounceMs) : debounceMs_(debounceMs) {}
  // Returns true once for each stable transition into the pressed state.
  bool update(bool electricalPressed, uint32_t nowMs);

 private:
  uint32_t debounceMs_;
  uint32_t changedAtMs_ = 0;
  bool rawState_ = false;
  bool stableState_ = false;
  bool initialized_ = false;
};
