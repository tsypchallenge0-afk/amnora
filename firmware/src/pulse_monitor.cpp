#include "pulse_monitor.h"

#include <Wire.h>
#include <algorithm>
#include <cmath>

#include "config.h"

namespace {
constexpr uint32_t kMinimumIntervalMs = 300;   // 200 BPM upper plausibility bound
constexpr uint32_t kMaximumIntervalMs = 2000;  // 30 BPM lower plausibility bound
constexpr uint32_t kSignalStaleMs = 2500;
constexpr float kBaselineAlpha = 0.01f;
constexpr float kEnvelopeAlpha = 0.05f;
}

bool PulseMonitor::begin() {
  available_ = sensor_.begin(Wire, I2C_SPEED_STANDARD);
  if (!available_) return false;
  sensor_.setup(0x1F, 4, 2, 100, 411, 4096);
  sensor_.setPulseAmplitudeRed(0x0A);
  sensor_.setPulseAmplitudeIR(0x1F);
  return true;
}

ScalarMeasurement PulseMonitor::update(uint32_t nowMs) {
  if (!available_) return {};
  const uint16_t sampleCount = sensor_.check();
  uint16_t processed = 0;
  while (processed < sampleCount && sensor_.available()) {
    const uint32_t ir = sensor_.getIR();
    sensor_.nextSample();
    const uint32_t sampleTimeMs = nowMs -
        static_cast<uint32_t>(sampleCount - 1 - processed) * 10U;
    ++processed;
    if (ir >= config::PULSE_CONTACT_IR_MIN &&
        ir < config::PULSE_IR_SATURATION_MAX) {
      lastContactMs_ = sampleTimeMs;
      contactSeen_ = true;
      processIrSample(ir, sampleTimeMs);
      const float acDcRatio = amplitudeEnvelope_ / baseline_;
      signalQualityGood_ = isfinite(acDcRatio) &&
                           acDcRatio <= config::PULSE_MAX_AC_DC_RATIO;
      if (!signalQualityGood_) resetIntervals();
    } else {
      resetIntervals();
      hasBaseline_ = false;
      contactSeen_ = false;
      signalQualityGood_ = false;
    }
  }

  if (!contactSeen_ || static_cast<uint32_t>(nowMs - lastContactMs_) > kSignalStaleMs ||
      !signalQualityGood_ || !validBeatSeen_ ||
      static_cast<uint32_t>(nowMs - lastValidBeatMs_) > kSignalStaleMs ||
      intervalCount_ < 3 || !intervalsAreConsistent()) {
    return {0.0f, nowMs, false};
  }
  return {medianBpm(), nowMs, true};
}

void PulseMonitor::processIrSample(uint32_t ir, uint32_t nowMs) {
  const float value = static_cast<float>(ir);
  if (!hasBaseline_) {
    baseline_ = value;
    hasBaseline_ = true;
  } else {
    baseline_ += kBaselineAlpha * (value - baseline_);
  }

  const float signal = value - baseline_;
  amplitudeEnvelope_ += kEnvelopeAlpha * (fabsf(signal) - amplitudeEnvelope_);
  const float threshold = std::max(config::PULSE_MIN_AC_AMPLITUDE,
                                   amplitudeEnvelope_ * 0.35f);
  const bool risingCrossing = !aboveBeatThreshold_ && signal >= threshold &&
                              previousSignal_ < threshold;

  if (risingCrossing) {
    if (previousBeatMs_ != 0) {
      const uint32_t interval = nowMs - previousBeatMs_;
      if (interval >= kMinimumIntervalMs && interval <= kMaximumIntervalMs) {
        intervalsMs_[intervalNext_] = interval;
        intervalNext_ = (intervalNext_ + 1) % 5;
        if (intervalCount_ < 5) ++intervalCount_;
        lastValidBeatMs_ = nowMs;
        validBeatSeen_ = true;
      }
    }
    previousBeatMs_ = nowMs;
  }

  aboveBeatThreshold_ = signal >= threshold;
  previousSignal_ = signal;
}

void PulseMonitor::resetIntervals() {
  intervalCount_ = 0;
  intervalNext_ = 0;
  previousBeatMs_ = 0;
  lastValidBeatMs_ = 0;
  validBeatSeen_ = false;
  aboveBeatThreshold_ = false;
  previousSignal_ = 0.0f;
  amplitudeEnvelope_ = 0.0f;
}

float PulseMonitor::medianBpm() const {
  uint32_t sorted[5];
  for (uint8_t i = 0; i < intervalCount_; ++i) sorted[i] = intervalsMs_[i];
  std::sort(sorted, sorted + intervalCount_);
  const uint32_t medianInterval = sorted[intervalCount_ / 2];
  return 60000.0f / static_cast<float>(medianInterval);
}

bool PulseMonitor::intervalsAreConsistent() const {
  if (intervalCount_ < 3) return false;
  uint32_t minInterval = intervalsMs_[0];
  uint32_t maxInterval = intervalsMs_[0];
  uint32_t sum = 0;
  for (uint8_t i = 0; i < intervalCount_; ++i) {
    minInterval = std::min(minInterval, intervalsMs_[i]);
    maxInterval = std::max(maxInterval, intervalsMs_[i]);
    sum += intervalsMs_[i];
  }
  const float mean = static_cast<float>(sum) / intervalCount_;
  return mean > 0.0f &&
         static_cast<float>(maxInterval - minInterval) / mean <=
             config::PULSE_MAX_INTERVAL_VARIATION;
}
