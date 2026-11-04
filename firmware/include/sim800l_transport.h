#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

enum class ModemEvent : uint8_t {
  None,
  NetworkReady,
  NetworkUnavailable,
  SmsAccepted,
  SmsFailed
};

class Sim800lTransport {
 public:
  void begin(uint32_t nowMs);
  void update(uint32_t nowMs);
  bool requestNetworkCheck(uint32_t nowMs);
  bool sendSms(const char* recipient, const char* message, uint32_t nowMs);
  ModemEvent takeEvent();
  bool idle() const;

 private:
  enum class State : uint8_t {
    Off,
    PowerSettle,
    PowerKeyLow,
    BootWait,
    Idle,
    WaitAt,
    WaitRegistration,
    WaitTextMode,
    WaitPrompt,
    WaitSmsSubmit
  };

  void sendLine(const char* command);
  void handleLine(uint32_t nowMs);
  void failCurrentOperation();
  void emit(ModemEvent event);
  bool copyBounded(char* destination, size_t capacity, const char* source);

  HardwareSerial serial_{1};
  State state_ = State::Off;
  ModemEvent event_ = ModemEvent::None;
  uint32_t deadlineMs_ = 0;
  char line_[128] = {};
  size_t lineLength_ = 0;
  char recipient_[24] = {};
  char message_[144] = {};
  bool registrationSeen_ = false;
  bool registered_ = false;
  bool smsReferenceSeen_ = false;
};
