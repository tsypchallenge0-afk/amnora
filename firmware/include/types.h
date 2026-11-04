#pragma once

#include <cstdint>

struct ScalarMeasurement {
  float value;
  uint32_t timestampMs;
  bool valid;

  ScalarMeasurement(float measurementValue = 0.0f, uint32_t sampleTimeMs = 0,
                    bool isValid = false)
      : value(measurementValue), timestampMs(sampleTimeMs), valid(isValid) {}
};

struct RawCountMeasurement {
  long value;
  bool valid;

  RawCountMeasurement(long rawValue = 0, bool isValid = false)
      : value(rawValue), valid(isValid) {}
};

struct SensorHealth {
  bool upperImu = false;
  bool pelvisImu = false;
  bool leftLoadCell = false;
  bool rightLoadCell = false;
  bool leftLoadCalibrated = false;
  bool rightLoadCalibrated = false;
  bool environment = false;
  bool pulse = false;
};

struct WorkloadSnapshot {
  ScalarMeasurement relativeTrunkAngleDeg;
  ScalarMeasurement leftHandleLoadKg;
  ScalarMeasurement rightHandleLoadKg;
  RawCountMeasurement leftHandleRawCounts;
  RawCountMeasurement rightHandleRawCounts;
  ScalarMeasurement pulseBpm;
  ScalarMeasurement ambientTemperatureC;
  ScalarMeasurement relativeHumidityPct;
  SensorHealth health;
  uint32_t bendingDwellMs = 0;
  bool postureReminder = false;
  bool loadReminder = false;
};
