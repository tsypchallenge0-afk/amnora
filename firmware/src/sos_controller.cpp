#include "sos_controller.h"

void SosController::trigger(uint32_t nowMs) {
  if (state_ != SosState::Idle && state_ != SosState::Submitted && state_ != SosState::Failed) return;
  state_ = SosState::CancelWindow;
  deadlineMs_ = nowMs;
  attempts_ = 0;
  confirmed_ = false;
}

void SosController::cancel() {
  if (state_ == SosState::CancelWindow) state_ = SosState::Idle;
}

void SosController::confirm() {
  if (state_ == SosState::CancelWindow) confirmed_ = true;
}

void SosController::modemReady(bool ready, uint32_t nowMs) {
  if (state_ != SosState::ModemCheck) return;
  if (ready) {
    state_ = SosState::Submitting;
    ++attempts_;
  } else {
    state_ = SosState::Failed;
  }
  deadlineMs_ = nowMs;
}

void SosController::smsResult(bool accepted, uint32_t nowMs) {
  if (state_ != SosState::Submitting) return;
  if (accepted) {
    // Means modem/network accepted submission, not confirmed recipient delivery.
    state_ = SosState::Submitted;
  } else {
    state_ = SosState::RetryWait;
    deadlineMs_ = nowMs;
  }
}

void SosController::tick(uint32_t nowMs, uint32_t cancelWindowMs,
                         uint32_t retryDelayMs, uint8_t maxAttempts) {
  if (state_ == SosState::CancelWindow &&
      (confirmed_ || static_cast<uint32_t>(nowMs - deadlineMs_) >= cancelWindowMs)) {
    state_ = SosState::ModemCheck;
    deadlineMs_ = nowMs;
  } else if (state_ == SosState::RetryWait &&
             static_cast<uint32_t>(nowMs - deadlineMs_) >= retryDelayMs) {
    if (attempts_ < maxAttempts) state_ = SosState::ModemCheck;
    else state_ = SosState::Failed;
  }
}
