#include "button_debouncer.h"

bool ButtonDebouncer::update(bool electricalPressed, uint32_t nowMs) {
  if (!initialized_) {
    initialized_ = true;
    rawState_ = electricalPressed;
    stableState_ = electricalPressed;
    changedAtMs_ = nowMs;
    return false;
  }
  if (electricalPressed != rawState_) {
    rawState_ = electricalPressed;
    changedAtMs_ = nowMs;
  }
  if (rawState_ != stableState_ &&
      static_cast<uint32_t>(nowMs - changedAtMs_) >= debounceMs_) {
    stableState_ = rawState_;
    return stableState_;
  }
  return false;
}
