#pragma once

#include <cstdint>

enum class FeedbackEvent : uint8_t {
  PostureReminder,
  LoadReminder,
  SosRequested,
  SmsAccepted,
  CommunicationFailure
};

class FeedbackController {
 public:
  void begin();
  void trigger(FeedbackEvent event, uint32_t nowMs);
  void update(uint32_t nowMs);

 private:
  void applyOutputs(bool active);
  bool active_ = false;
  bool patternRunning_ = false;
  uint8_t pulsesRemaining_ = 0;
  uint32_t nextTransitionMs_ = 0;
  uint32_t pulseOnMs_ = 180;
  uint32_t pulseOffMs_ = 220;
};
