#pragma once

#include <cstdint>

namespace config {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SAMPLE_PERIOD_MS = 50;
constexpr uint32_t SOS_CANCEL_WINDOW_MS = 5000;
constexpr uint32_t SOS_RETRY_DELAY_MS = 15000;
constexpr uint8_t SOS_MAX_ATTEMPTS = 2;
constexpr char EMERGENCY_PHONE_NUMBER[] = "+0000000000";  // Replace locally; do not commit personal data.

// Initial engineering parameters only. Tune for the task and validate with users.
constexpr float POSTURE_WARNING_DEG = 35.0f;
constexpr uint32_t POSTURE_WARNING_DWELL_MS = 8000;
constexpr float HANDLE_LOAD_WARNING_KG = 10.0f;
constexpr float GRAVITY_M_S2 = 9.80665f;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
}  // namespace config
