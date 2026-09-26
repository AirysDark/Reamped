#pragma once

#include <Arduino.h>

class Tas5731m;

// Higher-level TAS5731M DSP access.
//
// DSP coefficient/register programming uses multi-byte transactions.
// This module deliberately does not invent EQ coefficients: it provides
// a single place to add verified TI-format biquad/DRC/routing writes later.
class Tas5731mDsp {
public:
  explicit Tas5731mDsp(Tas5731m &amp);

  bool writeBytes(uint8_t reg, const uint8_t *data, size_t length);
  bool readBytes(uint8_t reg, uint8_t *data, size_t length);

private:
  Tas5731m &amp_;
};
