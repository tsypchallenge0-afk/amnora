#include "sim800l_transport.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "pins.h"

namespace {
constexpr uint32_t kPowerSettleMs = 100;
constexpr uint32_t kPowerKeyPulseMs = 1000;
constexpr uint32_t kModemBootMs = 3000;
constexpr uint32_t kCommandTimeoutMs = 8000;
constexpr uint32_t kSmsTimeoutMs = 30000;
constexpr char kUnconfiguredPhoneNumber[] = "+0000000000";

bool validPhoneNumber(const char* number) {
  const size_t length = std::strlen(number);
  if (length < 8 || length >= 24 || number[0] != '+') return false;
  for (size_t i = 1; i < length; ++i) {
    if (number[i] < '0' || number[i] > '9') return false;
  }
  return true;
}

bool safeSmsText(const char* text) {
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
    if (*p < 0x20 && *p != '\n' && *p != '\r') return false;
  }
  return true;
}
}

void Sim800lTransport::begin(uint32_t nowMs) {
  pinMode(pins::MODEM_RESET, OUTPUT);
  digitalWrite(pins::MODEM_RESET, HIGH);
  pinMode(pins::MODEM_POWER, OUTPUT);
  digitalWrite(pins::MODEM_POWER, HIGH);
  pinMode(pins::MODEM_PWRKEY, OUTPUT);
  digitalWrite(pins::MODEM_PWRKEY, HIGH);

  serial_.begin(115200, SERIAL_8N1, pins::MODEM_RX, pins::MODEM_TX);
  state_ = State::PowerSettle;
  deadlineMs_ = nowMs + kPowerSettleMs;
}

void Sim800lTransport::update(uint32_t nowMs) {
  if (state_ == State::PowerSettle && static_cast<int32_t>(nowMs - deadlineMs_) >= 0) {
    digitalWrite(pins::MODEM_PWRKEY, LOW);
    state_ = State::PowerKeyLow;
    deadlineMs_ = nowMs + kPowerKeyPulseMs;
  } else if (state_ == State::PowerKeyLow && static_cast<int32_t>(nowMs - deadlineMs_) >= 0) {
    digitalWrite(pins::MODEM_PWRKEY, HIGH);
    state_ = State::BootWait;
    deadlineMs_ = nowMs + kModemBootMs;
  } else if (state_ == State::BootWait && static_cast<int32_t>(nowMs - deadlineMs_) >= 0) {
    state_ = State::Idle;
  }

  while (serial_.available() > 0) {
    const char value = static_cast<char>(serial_.read());
    if (state_ == State::WaitPrompt && value == '>') {
      serial_.print(message_);
      serial_.write(static_cast<uint8_t>(0x1A));
      state_ = State::WaitSmsSubmit;
      deadlineMs_ = nowMs + kSmsTimeoutMs;
      continue;
    }
    if (value == '\r') continue;
    if (value == '\n') {
      if (lineLength_ > 0) handleLine(nowMs);
      lineLength_ = 0;
      line_[0] = '\0';
      continue;
    }
    if (lineLength_ + 1 < sizeof(line_)) {
      line_[lineLength_++] = value;
      line_[lineLength_] = '\0';
    } else {
      lineLength_ = 0;
      line_[0] = '\0';
    }
  }

  if ((state_ == State::WaitAt || state_ == State::WaitRegistration ||
       state_ == State::WaitTextMode || state_ == State::WaitPrompt ||
       state_ == State::WaitSmsSubmit) &&
      static_cast<int32_t>(nowMs - deadlineMs_) >= 0) {
    failCurrentOperation();
  }
}

bool Sim800lTransport::requestNetworkCheck(uint32_t nowMs) {
  if (state_ != State::Idle) return false;
  registrationSeen_ = false;
  registered_ = false;
  sendLine("AT");
  state_ = State::WaitAt;
  deadlineMs_ = nowMs + kCommandTimeoutMs;
  return true;
}

bool Sim800lTransport::sendSms(const char* recipient, const char* message,
                               uint32_t nowMs) {
  if (state_ != State::Idle || !copyBounded(recipient_, sizeof(recipient_), recipient) ||
      !copyBounded(message_, sizeof(message_), message) || !validPhoneNumber(recipient_) ||
      !safeSmsText(message_) ||
      std::strcmp(recipient_, kUnconfiguredPhoneNumber) == 0) {
    emit(ModemEvent::SmsFailed);
    return false;
  }
  smsReferenceSeen_ = false;
  sendLine("AT+CMGF=1");
  state_ = State::WaitTextMode;
  deadlineMs_ = nowMs + kCommandTimeoutMs;
  return true;
}

ModemEvent Sim800lTransport::takeEvent() {
  const ModemEvent result = event_;
  event_ = ModemEvent::None;
  return result;
}

bool Sim800lTransport::idle() const { return state_ == State::Idle; }

void Sim800lTransport::handleLine(uint32_t nowMs) {
  if (std::strcmp(line_, "ERROR") == 0 || std::strncmp(line_, "+CMS ERROR", 10) == 0) {
    if (state_ == State::WaitAt || state_ == State::WaitRegistration ||
        state_ == State::WaitTextMode || state_ == State::WaitPrompt ||
        state_ == State::WaitSmsSubmit) {
      failCurrentOperation();
    }
    return;
  }

  if (state_ == State::WaitRegistration && std::strncmp(line_, "+CREG:", 6) == 0) {
    registrationSeen_ = true;
    const char* finalComma = std::strrchr(line_, ',');
    if (finalComma != nullptr) {
      const int status = std::atoi(finalComma + 1);
      registered_ = status == 1 || status == 5;
    }
    return;
  }

  if (state_ == State::WaitSmsSubmit && std::strncmp(line_, "+CMGS:", 6) == 0) {
    smsReferenceSeen_ = true;
    return;
  }

  if (std::strcmp(line_, "OK") != 0) return;
  switch (state_) {
    case State::WaitAt:
      sendLine("AT+CREG?");
      state_ = State::WaitRegistration;
      deadlineMs_ = nowMs + kCommandTimeoutMs;
      break;
    case State::WaitRegistration:
      state_ = State::Idle;
      emit(registrationSeen_ && registered_ ? ModemEvent::NetworkReady
                                            : ModemEvent::NetworkUnavailable);
      break;
    case State::WaitTextMode: {
      char command[48];
      const int written = std::snprintf(command, sizeof(command), "AT+CMGS=\"%s\"", recipient_);
      if (written <= 0 || static_cast<size_t>(written) >= sizeof(command)) {
        failCurrentOperation();
        break;
      }
      sendLine(command);
      state_ = State::WaitPrompt;
      deadlineMs_ = nowMs + kCommandTimeoutMs;
      break;
    }
    case State::WaitSmsSubmit:
      state_ = State::Idle;
      emit(smsReferenceSeen_ ? ModemEvent::SmsAccepted : ModemEvent::SmsFailed);
      break;
    default:
      break;
  }
}

void Sim800lTransport::failCurrentOperation() {
  const bool wasNetworkCheck = state_ == State::WaitAt || state_ == State::WaitRegistration;
  const bool wasSms = state_ == State::WaitTextMode || state_ == State::WaitPrompt ||
                      state_ == State::WaitSmsSubmit;
  state_ = State::Idle;
  if (wasNetworkCheck) emit(ModemEvent::NetworkUnavailable);
  else if (wasSms) emit(ModemEvent::SmsFailed);
}

void Sim800lTransport::sendLine(const char* command) {
  serial_.print(command);
  serial_.print("\r\n");
}

void Sim800lTransport::emit(ModemEvent event) { event_ = event; }

bool Sim800lTransport::copyBounded(char* destination, size_t capacity,
                                   const char* source) {
  if (source == nullptr || capacity == 0) return false;
  const size_t length = std::strlen(source);
  if (length == 0 || length >= capacity) return false;
  std::memcpy(destination, source, length + 1);
  return true;
}
