#pragma once

#include <Arduino.h>

class AudioOutput;
class Tas5731m;

class TestTone {
public:
  TestTone(AudioOutput &audio, Tas5731m &amp);

  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }

  void service();

private:
  AudioOutput &audio_;
  Tas5731m &amp_;

  bool enabled_ = false;
  float phase_ = 0.0f;
};
