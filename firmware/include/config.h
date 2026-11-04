#pragma once

#include <cstdint>

namespace config {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SAMPLE_PERIOD_MS = 50;
constexpr uint32_t SOS_CANCEL_WINDOW_MS = 5000;
constexpr uint32_t SOS_RETRY_DELAY_MS = 15000;
constexpr uint32_t SOS_MODEM_TIMEOUT_MS = 15000;
constexpr uint32_t SOS_SMS_TIMEOUT_MS = 30000;
constexpr uint8_t SOS_MAX_ATTEMPTS = 2;
constexpr char EMERGENCY_PHONE_NUMBER[] = "+0000000000";  // Replace locally; do not commit personal data.

// Initial configurable reminders, not universal safety thresholds.
constexpr float POSTURE_WARNING_DEG = 35.0f;
constexpr float POSTURE_RESET_DEG = 30.0f;
constexpr uint32_t POSTURE_WARNING_DWELL_MS = 8000;
constexpr float HANDLE_LOAD_WARNING_KG = 10.0f;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
constexpr uint32_t ENVIRONMENT_SAMPLE_PERIOD_MS = 1000;
constexpr float IMU_COMPLEMENTARY_ACCEL_WEIGHT = 0.02f;
constexpr uint8_t NEUTRAL_CALIBRATION_SAMPLES = 40;  // About 2 seconds at the main sample rate.
constexpr float NEUTRAL_CALIBRATION_MAX_STDDEV_DEG = 2.5f;
constexpr uint8_t LOAD_TARE_SAMPLES = 16;
constexpr uint32_t CALIBRATION_TIMEOUT_MS = 10000;
constexpr float LOAD_COUNTS_PER_KG_LEFT = 0.0f;  // Calibrate independently with known loads.
constexpr float LOAD_COUNTS_PER_KG_RIGHT = 0.0f;
constexpr uint32_t PULSE_CONTACT_IR_MIN = 20000;
constexpr uint32_t PULSE_IR_SATURATION_MAX = 260000;
constexpr float PULSE_MIN_AC_AMPLITUDE = 500.0f;
constexpr float PULSE_MAX_AC_DC_RATIO = 0.15f;  // Heuristic gate; tune for the selected breakout and fit.
constexpr float PULSE_MAX_INTERVAL_VARIATION = 0.25f;
}  // namespace config
