#include "TestTone.h"

#include <math.h>

#include "AudioOutput.h"
#include "ReampedPins.h"
#include "Tas5731m.h"

TestTone::TestTone(AudioOutput &audio, Tas5731m &amp)
: audio_(audio), amp_(amp) {}

void TestTone::setEnabled(bool enabled) {
  if (!amp_.ready()) {
    Serial.println("[AMP] Amplifier is not ready.");
    enabled_ = false;
    return;
  }

  enabled_ = enabled;

  if (enabled_) {
    amp_.setMute(false);
    Serial.println("[AUDIO] Test tone ON.");
  } else {
    amp_.setMute(true);
    Serial.println("[AUDIO] Test tone OFF.");
  }
}

void TestTone::service() {
  constexpr float frequency = 440.0f;
  constexpr float amplitude = 1200.0f;
  constexpr size_t frames = 128;

  int16_t samples[frames * 2];

  if (!enabled_ || !amp_.ready() || amp_.muted()) {
    memset(samples, 0, sizeof(samples));
  } else {
    const float phaseStep =
      2.0f * PI * frequency / (float)REAMPED_SAMPLE_RATE;

    for (size_t i = 0; i < frames; ++i) {
      const int16_t s = (int16_t)(sinf(phase_) * amplitude);

      phase_ += phaseStep;
      if (phase_ >= 2.0f * PI) {
        phase_ -= 2.0f * PI;
      }

      samples[(i * 2) + 0] = s;
      samples[(i * 2) + 1] = s;
    }
  }

  audio_.write(samples, frames * 2);
}
