#pragma once

#include <MAX30105.h>

#include "types.h"

class PulseMonitor {
 public:
  bool begin();
  ScalarMeasurement update(uint32_t nowMs);
  bool available() const { return available_; }

 private:
  void processIrSample(uint32_t ir, uint32_t nowMs);
  void resetIntervals();
  float medianBpm() const;
  bool intervalsAreConsistent() const;

  MAX30105 sensor_;
  uint32_t intervalsMs_[5] = {};
  uint8_t intervalCount_ = 0;
  uint8_t intervalNext_ = 0;
  float baseline_ = 0.0f;
  float amplitudeEnvelope_ = 0.0f;
  float previousSignal_ = 0.0f;
  uint32_t previousBeatMs_ = 0;
  uint32_t lastValidBeatMs_ = 0;
  uint32_t lastContactMs_ = 0;
  bool hasBaseline_ = false;
  bool aboveBeatThreshold_ = false;
  bool available_ = false;
  bool contactSeen_ = false;
  bool signalQualityGood_ = false;
  bool validBeatSeen_ = false;
};
