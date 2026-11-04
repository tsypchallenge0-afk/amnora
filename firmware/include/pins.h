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

// Assign after validating the actual board schematic and connected peripherals.
// Keep -1 until a verified, exposed, unused GPIO is selected for each signal.
constexpr int IMU_I2C_SDA = -1;
constexpr int IMU_I2C_SCL = -1;
constexpr int HX711_LEFT_DOUT = -1;
constexpr int HX711_LEFT_SCK = -1;
constexpr int HX711_RIGHT_DOUT = -1;
constexpr int HX711_RIGHT_SCK = -1;
constexpr int VIBRATION_DRIVER = -1;
constexpr int BUZZER_DRIVER = -1;
constexpr int SOS_BUTTON = -1;
constexpr int CANCEL_BUTTON = -1;
}  // namespace pins
