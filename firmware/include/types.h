#pragma once

#include <cstdint>

struct ScalarMeasurement {
  float value = 0.0f;
  uint32_t timestampMs = 0;
  bool valid = false;
};

struct WorkloadSnapshot {
  ScalarMeasurement relativeTrunkAngleDeg;
  ScalarMeasurement leftHandleLoadKg;
  ScalarMeasurement rightHandleLoadKg;
  ScalarMeasurement pulseBpm;
  ScalarMeasurement ambientTemperatureC;
  ScalarMeasurement relativeHumidityPct;
  uint32_t bendingDwellMs = 0;
  bool postureReminder = false;
  bool loadReminder = false;
};
