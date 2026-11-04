# Hardware inventory and design checks

## Baseline inventory

| Item | Count | Integration notes |
|---|---:|---|
| LILYGO T-Call V1.4, SIM800L/IP5306 variant | 1 | Confirm board silkscreen and schematic; SIM800L must be the 2G variant |
| IMU (MPU6050 or ICM-42670-P; choose one model) | 2 | Upper back and pelvis; verify address selection and bus compatibility |
| Load cell | 2 | Installed in dedicated load-bearing handle structures |
| HX711 amplifier | 2 | One per load cell; independent calibration |
| MAX30102 breakout | 1 | Wrist contact; motion-sensitive optical estimate |
| SHT31 breakout | 1 | Ventilated ambient location |
| Vibration motor + driver | 1 | MOSFET/transistor driver; never drive motor directly from GPIO |
| Buzzer + driver if needed | 1 | Check current and voltage against GPIO limits |
| Status LED + resistor (if external) | 1 | Board LED may use a reserved board pin |
| Guarded SOS and cancel buttons | 2 | Debounce in firmware; physically distinguishable |
| Protected 1-cell Li-ion/LiPo battery | 1 | Confirm connector polarity, capacity, charging and board compatibility |
| Vest, enclosure, cables, handle structures | as needed | Protect against sweat, dust, snags, and accidental activation |

## T-Call V1.4 reference pins

The official LILYGO repository lists these functions for the SIM800L/IP5306 V1.4 design. Confirm them against the exact board in hand before wiring. The UART names below refer to ESP32 UART pins as configured by the vendor example: `MODEM_RX=26`, `MODEM_TX=27`.

| GPIO | Board function | Design treatment |
|---:|---|---|
| 26 | Modem UART receive (ESP32 RX) | Reserved |
| 27 | Modem UART transmit (ESP32 TX) | Reserved |
| 4 | Modem PWRKEY | Reserved |
| 5 | Modem reset | Reserved |
| 23 | Modem power control | Reserved |
| 21 | Power IC I2C SDA | Shared bus only after address/electrical review |
| 22 | Power IC I2C SCL | Shared bus only after address/electrical review |
| 13 | Board LED | Reserved unless exact revision confirms otherwise |
| 32, 33 | V1.4 modem DTR / RI | Reserved |

The vendor's reference pin list is not a complete guarantee that every other ESP32 GPIO is exposed, free, or safe for every peripheral. The actual assembly and board schematic govern the final mapping.

## Electrical and mechanical checks

- Verify sensor breakout input voltage and logic levels; do not infer compatibility from the sensor IC name alone.
- Two MPU6050 devices need distinct I2C addresses via AD0, or another justified bus topology. Verify actual breakout configuration.
- Check I2C pull-ups and all device addresses, including the onboard power controller.
- HX711 data/clock pins must be selected from available, exposed GPIOs; keep wiring short and mechanically protected.
- Use an appropriate transistor/MOSFET and protection for the vibration motor; check buzzer driver requirements.
- Confirm SIM800L supply stability during transmit peaks; do not size battery life from nominal current alone.
- Validate load-cell capacity and mounting with a mechanically safe handle that routes the intended force through the sensing element.
- Do not state a battery runtime, precision, or load limit until measured for the assembled design.

## Official reference

LILYGO's [T-Call SIM800 repository](https://github.com/Xinyuan-LILYGO/LilyGo-T-Call-SIM800) contains revision references, example firmware, and schematic links.
