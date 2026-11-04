#include "sos_controller.h"

void SosController::trigger(uint32_t nowMs) {
  if (state_ != SosState::Idle && state_ != SosState::Submitted && state_ != SosState::Failed) return;
  state_ = SosState::CancelWindow;
  deadlineMs_ = nowMs;
  attempts_ = 0;
}

void SosController::cancel() {
  if (state_ == SosState::CancelWindow) state_ = SosState::Idle;
}

void SosController::modemReady(bool ready, uint32_t nowMs) {
  if (state_ != SosState::ModemCheck) return;
  if (ready) {
    state_ = SosState::Submitting;
  } else {
    scheduleRetryOrFail(nowMs);
  }
  deadlineMs_ = nowMs;
}

void SosController::smsResult(bool accepted, uint32_t nowMs) {
  if (state_ != SosState::Submitting) return;
  if (accepted) {
    // Means modem/network accepted submission, not confirmed recipient delivery.
    state_ = SosState::Submitted;
  } else {
    scheduleRetryOrFail(nowMs);
  }
}

void SosController::tick(uint32_t nowMs, uint32_t cancelWindowMs,
                         uint32_t retryDelayMs, uint32_t modemTimeoutMs,
                         uint32_t smsTimeoutMs, uint8_t maxAttempts) {
  if (state_ == SosState::CancelWindow &&
      static_cast<uint32_t>(nowMs - deadlineMs_) >= cancelWindowMs) {
    state_ = SosState::ModemCheck;
    deadlineMs_ = nowMs;
    attempts_ = 1;
  } else if (state_ == SosState::ModemCheck &&
             static_cast<uint32_t>(nowMs - deadlineMs_) >= modemTimeoutMs) {
    scheduleRetryOrFail(nowMs);
  } else if (state_ == SosState::Submitting &&
             static_cast<uint32_t>(nowMs - deadlineMs_) >= smsTimeoutMs) {
    scheduleRetryOrFail(nowMs);
  } else if (state_ == SosState::RetryWait &&
             static_cast<uint32_t>(nowMs - deadlineMs_) >= retryDelayMs) {
    if (attempts_ < maxAttempts) {
      ++attempts_;
      state_ = SosState::ModemCheck;
      deadlineMs_ = nowMs;
    } else {
      state_ = SosState::Failed;
    }
  }
}

void SosController::scheduleRetryOrFail(uint32_t nowMs) {
  state_ = SosState::RetryWait;
  deadlineMs_ = nowMs;
}
