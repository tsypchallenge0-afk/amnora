#pragma once

// T-Call V1.4 SIM800L/IP5306 board resources from the LILYGO reference.
// UART constructor order is RX, TX from the ESP32 point of view.
namespace pins {
constexpr int MODEM_RX = 26;
constexpr int MODEM_TX = 27;
constexpr int MODEM_PWRKEY = 4;
constexpr int MODEM_RESET = 5;
constexpr int MODEM_POWER = 23;
constexpr int MODEM_DTR = 32;
constexpr int MODEM_RI = 33;
constexpr int POWER_I2C_SDA = 21;
constexpr int POWER_I2C_SCL = 22;
constexpr int BOARD_LED = 13;

// The T-Call power IC already uses this I2C bus. Sensor sharing is valid only
// when the assembled bus pull-ups, addresses, logic levels, and capacitance suit it.
constexpr int SENSOR_I2C_SDA = POWER_I2C_SDA;
constexpr int SENSOR_I2C_SCL = POWER_I2C_SCL;

// Assign these only after checking the exact board schematic and harness.
// -1 disables the associated input/output safely in firmware.
constexpr int HX711_LEFT_DOUT = -1;
constexpr int HX711_LEFT_SCK = -1;
constexpr int HX711_RIGHT_DOUT = -1;
constexpr int HX711_RIGHT_SCK = -1;
constexpr int VIBRATION_DRIVER = -1;
constexpr int BUZZER_DRIVER = -1;
constexpr int SOS_BUTTON = -1;
constexpr int CANCEL_BUTTON = -1;
}  // namespace pins
