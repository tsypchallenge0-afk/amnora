#pragma once

#include <cstdint>

enum class SosState : uint8_t {
  Idle,
  CancelWindow,
  ModemCheck,
  Submitting,
  RetryWait,
  Submitted,
  Failed
};

class SosController {
 public:
  void trigger(uint32_t nowMs);
  void cancel();
  void modemReady(bool ready, uint32_t nowMs);
  void smsResult(bool accepted, uint32_t nowMs);
  void tick(uint32_t nowMs, uint32_t cancelWindowMs, uint32_t retryDelayMs,
            uint32_t modemTimeoutMs, uint32_t smsTimeoutMs,
            uint8_t maxAttempts);
  SosState state() const { return state_; }
  bool shouldCheckModem() const { return state_ == SosState::ModemCheck; }
  bool shouldSubmitSms() const { return state_ == SosState::Submitting; }
  uint8_t attempts() const { return attempts_; }

 private:
  SosState state_ = SosState::Idle;
  uint32_t deadlineMs_ = 0;
  uint8_t attempts_ = 0;
  void scheduleRetryOrFail(uint32_t nowMs);
};
