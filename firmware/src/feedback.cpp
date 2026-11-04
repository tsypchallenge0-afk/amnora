#include "feedback.h"

#include <Arduino.h>

#include "pins.h"

void FeedbackController::begin() {
  if (pins::VIBRATION_DRIVER >= 0) {
    pinMode(pins::VIBRATION_DRIVER, OUTPUT);
    digitalWrite(pins::VIBRATION_DRIVER, LOW);
  }
  if (pins::BUZZER_DRIVER >= 0) {
    pinMode(pins::BUZZER_DRIVER, OUTPUT);
    digitalWrite(pins::BUZZER_DRIVER, LOW);
  }
  if (pins::BOARD_LED >= 0) {
    pinMode(pins::BOARD_LED, OUTPUT);
    digitalWrite(pins::BOARD_LED, LOW);
  }
}

void FeedbackController::trigger(FeedbackEvent event, uint32_t nowMs) {
  switch (event) {
    case FeedbackEvent::PostureReminder:
      pulsesRemaining_ = 1;
      pulseOnMs_ = 160;
      pulseOffMs_ = 200;
      break;
    case FeedbackEvent::LoadReminder:
      pulsesRemaining_ = 2;
      pulseOnMs_ = 160;
      pulseOffMs_ = 200;
      break;
    case FeedbackEvent::SosRequested:
      pulsesRemaining_ = 3;
      pulseOnMs_ = 350;
      pulseOffMs_ = 250;
      break;
    case FeedbackEvent::SmsAccepted:
      pulsesRemaining_ = 2;
      pulseOnMs_ = 90;
      pulseOffMs_ = 120;
      break;
    case FeedbackEvent::CommunicationFailure:
      pulsesRemaining_ = 4;
      pulseOnMs_ = 100;
      pulseOffMs_ = 100;
      break;
  }
  active_ = true;
  patternRunning_ = true;
  nextTransitionMs_ = nowMs + pulseOnMs_;
  applyOutputs(true);
}

void FeedbackController::update(uint32_t nowMs) {
  if (!patternRunning_ || static_cast<int32_t>(nowMs - nextTransitionMs_) < 0) return;
  if (active_) {
    if (pulsesRemaining_ > 0) --pulsesRemaining_;
    if (pulsesRemaining_ == 0) {
      active_ = false;
      patternRunning_ = false;
      applyOutputs(false);
      return;
    }
    active_ = false;
    applyOutputs(false);
    nextTransitionMs_ = nowMs + pulseOffMs_;
    return;
  }
  active_ = true;
  applyOutputs(true);
  nextTransitionMs_ = nowMs + pulseOnMs_;
}

void FeedbackController::applyOutputs(bool active) {
  const uint8_t level = active ? HIGH : LOW;
  if (pins::VIBRATION_DRIVER >= 0) digitalWrite(pins::VIBRATION_DRIVER, level);
  if (pins::BUZZER_DRIVER >= 0) digitalWrite(pins::BUZZER_DRIVER, level);
  if (pins::BOARD_LED >= 0) digitalWrite(pins::BOARD_LED, level);
}
