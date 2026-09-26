#pragma once

#include <Arduino.h>

class AudioOutput {
public:
  bool begin();
  bool write(const int16_t *samples, size_t sampleCount);
  void writeSilence(size_t frames);

private:
  bool started_ = false;
};
