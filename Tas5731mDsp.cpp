#include "Tas5731mDsp.h"

#include "Tas5731m.h"

Tas5731mDsp::Tas5731mDsp(Tas5731m &amp)
: amp_(amp) {}

bool Tas5731mDsp::writeBytes(uint8_t reg, const uint8_t *data, size_t length) {
  if (!amp_.ready()) {
    return false;
  }
  return amp_.writeBlock(reg, data, length);
}

bool Tas5731mDsp::readBytes(uint8_t reg, uint8_t *data, size_t length) {
  if (!amp_.ready()) {
    return false;
  }
  return amp_.readBlock(reg, data, length);
}
