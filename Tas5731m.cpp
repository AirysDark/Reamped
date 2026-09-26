#include "Tas5731m.h"

Tas5731m::Tas5731m(TwoWire &wire, int resetPin, int pdnPin)
: wire_(wire), resetPin_(resetPin), pdnPin_(pdnPin) {}

void Tas5731m::beginPins() {
  pinMode(resetPin_, OUTPUT);
  pinMode(pdnPin_, OUTPUT);
  hardSafeState();
}

void Tas5731m::hardSafeState() {
  digitalWrite(resetPin_, LOW);
  digitalWrite(pdnPin_, LOW);
  ready_ = false;
  muted_ = true;
}

bool Tas5731m::addressResponds(uint8_t address) {
  wire_.beginTransmission(address);
  return wire_.endTransmission() == 0;
}

uint8_t Tas5731m::detectAddress() {
  if (addressResponds(ADDR_LOW)) {
    return ADDR_LOW;
  }
  if (addressResponds(ADDR_HIGH)) {
    return ADDR_HIGH;
  }
  return 0;
}

bool Tas5731m::writeRegister8(uint8_t reg, uint8_t value) {
  return writeBlock(reg, &value, 1);
}

bool Tas5731m::readRegister8(uint8_t reg, uint8_t &value) {
  return readBlock(reg, &value, 1);
}

bool Tas5731m::writeBlock(uint8_t reg, const uint8_t *data, size_t length) {
  if (address_ == 0 || data == nullptr || length == 0) {
    return false;
  }

  wire_.beginTransmission(address_);
  wire_.write(reg);

  for (size_t i = 0; i < length; ++i) {
    wire_.write(data[i]);
  }

  return wire_.endTransmission() == 0;
}

bool Tas5731m::readBlock(uint8_t reg, uint8_t *data, size_t length) {
  if (address_ == 0 || data == nullptr || length == 0) {
    return false;
  }

  wire_.beginTransmission(address_);
  wire_.write(reg);

  if (wire_.endTransmission(false) != 0) {
    return false;
  }

  const size_t received = wire_.requestFrom((int)address_, (int)length);
  if (received != length) {
    return false;
  }

  for (size_t i = 0; i < length; ++i) {
    data[i] = wire_.read();
  }

  return true;
}

bool Tas5731m::begin() {
  ready_ = false;
  muted_ = true;
  address_ = 0;

  Serial.println("[AMP] Starting conservative TAS5731M power-up sequence.");

  // MCLK/BCLK/LRCLK should already be active before this point.
  // Keep RESET asserted, then release PDN first.
  digitalWrite(resetPin_, LOW);
  digitalWrite(pdnPin_, HIGH);
  delayMicroseconds(200);

  // Release RESET and allow the digital core to settle.
  digitalWrite(resetPin_, HIGH);
  delay(RESET_SETTLE_MS);

  address_ = detectAddress();
  if (address_ == 0) {
    Serial.println("[AMP] TAS5731M not found at I2C 0x34 or 0x36.");
    hardSafeState();
    return false;
  }

  Serial.printf("[AMP] TAS5731M responding at I2C 0x%02X\n", address_);

  // Datasheet initialization requires oscillator trim after reset.
  if (!writeRegister8(REG_OSC_TRIM, 0x00)) {
    Serial.println("[AMP] Oscillator trim write failed.");
    hardSafeState();
    return false;
  }

  delay(OSC_TRIM_SETTLE_MS);

  // ESP32 stream is 16-bit Philips/I2S.
  if (!writeRegister8(REG_SERIAL_IF, SERIAL_I2S_16BIT)) {
    Serial.println("[AMP] Serial interface setup failed.");
    hardSafeState();
    return false;
  }

  // Configure the amp while it is still muted/shutdown.
  if (!writeRegister8(REG_MASTER_VOLUME, 0xFF)) {
    Serial.println("[AMP] Failed to force startup mute.");
    hardSafeState();
    return false;
  }

  if (!writeRegister8(REG_SOFT_MUTE, MUTE_CH1_CH2)) {
    Serial.println("[AMP] Failed to enable soft mute.");
    hardSafeState();
    return false;
  }

  // The first startup after power application needs a long guard period.
  // This intentionally exceeds the minimum timing to make bring-up robust
  // on a salvaged board with unknown rail ramp characteristics.
  Serial.printf("[AMP] Waiting %lu ms first-start guard...\n",
                (unsigned long)FIRST_STARTUP_GUARD_MS);
  delay(FIRST_STARTUP_GUARD_MS);

  if (!writeRegister8(REG_SYS_CTRL_2, EXIT_SHUTDOWN)) {
    Serial.println("[AMP] Failed to exit shutdown.");
    hardSafeState();
    return false;
  }

  // Allow the output stage / soft-start sequence to complete before
  // restoring the requested volume.
  delay(POST_SHUTDOWN_EXIT_MS);

  if (!writeRegister8(REG_MASTER_VOLUME, masterVolume_)) {
    Serial.println("[AMP] Failed to load startup volume.");
    hardSafeState();
    return false;
  }

  ready_ = true;
  muted_ = true;

  uint8_t id = 0;
  if (readDeviceId(id)) {
    Serial.printf("[AMP] Device-ID register: 0x%02X\n", id);
  }

  Serial.println("[AMP] Initialized and muted.");
  return true;
}

bool Tas5731m::shutdown() {
  if (address_ == 0) {
    hardSafeState();
    return false;
  }

  Serial.println("[AMP] Graceful shutdown.");

  // Mute first to minimize pops.
  writeRegister8(REG_SOFT_MUTE, MUTE_CH1_CH2);
  muted_ = true;

  // Put all channels into shutdown.
  const bool ok = writeRegister8(REG_SYS_CTRL_2, ENTER_SHUTDOWN);

  // Give the soft-stop sequence time to settle before PDN/RESET.
  delay(POST_SHUTDOWN_EXIT_MS);

  digitalWrite(pdnPin_, LOW);
  delay(POWERDOWN_TO_RESET_MS);
  digitalWrite(resetPin_, LOW);

  ready_ = false;
  return ok;
}

bool Tas5731m::setMute(bool mute) {
  if (!ready_) {
    return false;
  }

  if (!writeRegister8(REG_SOFT_MUTE, mute ? MUTE_CH1_CH2 : UNMUTE_ALL)) {
    return false;
  }

  muted_ = mute;
  Serial.printf("[AMP] %s\n", muted_ ? "MUTED" : "UNMUTED");
  return true;
}

bool Tas5731m::setMasterVolume(uint8_t value) {
  if (value < 0x30) {
    value = 0x30;
  }
  if (value > 0xFE) {
    value = 0xFE;
  }

  masterVolume_ = value;

  if (ready_ && !writeRegister8(REG_MASTER_VOLUME, masterVolume_)) {
    return false;
  }

  const float db = 24.0f - (0.5f * masterVolume_);
  Serial.printf("[AMP] master volume reg=0x%02X (%.1f dB)\n", masterVolume_, db);
  return true;
}

bool Tas5731m::readErrorStatus(uint8_t &value) {
  return readRegister8(REG_ERROR_STATUS, value);
}

bool Tas5731m::readSystemControl2(uint8_t &value) {
  return readRegister8(REG_SYS_CTRL_2, value);
}

bool Tas5731m::readDeviceId(uint8_t &value) {
  return readRegister8(REG_DEVICE_ID, value);
}
