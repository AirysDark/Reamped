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

bool Tas5731m::write8(uint8_t reg, uint8_t value) {
  if (address_ == 0) {
    return false;
  }

  wire_.beginTransmission(address_);
  wire_.write(reg);
  wire_.write(value);
  return wire_.endTransmission() == 0;
}

bool Tas5731m::read8(uint8_t reg, uint8_t &value) {
  if (address_ == 0) {
    return false;
  }

  wire_.beginTransmission(address_);
  wire_.write(reg);
  if (wire_.endTransmission(false) != 0) {
    return false;
  }

  if (wire_.requestFrom((int)address_, 1) != 1) {
    return false;
  }

  value = wire_.read();
  return true;
}

bool Tas5731m::begin() {
  ready_ = false;
  muted_ = true;
  address_ = 0;

  // Known state first.
  digitalWrite(resetPin_, LOW);
  digitalWrite(pdnPin_, HIGH);
  delayMicroseconds(200);
  digitalWrite(resetPin_, HIGH);

  // Allow the device to complete reset before I2C access.
  delay(15);

  address_ = detectAddress();
  if (address_ == 0) {
    Serial.println("[AMP] TAS5731M not found at I2C 0x34 or 0x36.");
    hardSafeState();
    return false;
  }

  Serial.printf("[AMP] TAS5731M responding at I2C 0x%02X\n", address_);

  if (!write8(REG_OSC_TRIM, 0x00)) {
    Serial.println("[AMP] Oscillator trim write failed.");
    hardSafeState();
    return false;
  }
  delay(55);

  if (!write8(REG_SERIAL_IF, SERIAL_I2S_16BIT)) {
    Serial.println("[AMP] Serial interface setup failed.");
    hardSafeState();
    return false;
  }

  // Keep outputs muted while leaving shutdown.
  write8(REG_MASTER_VOLUME, 0xFF);
  write8(REG_SOFT_MUTE, MUTE_CH1_CH2);

  if (!write8(REG_SYS_CTRL_2, EXIT_SHUTDOWN)) {
    Serial.println("[AMP] Failed to exit shutdown.");
    hardSafeState();
    return false;
  }

  delay(20);

  if (!write8(REG_MASTER_VOLUME, masterVolume_)) {
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

bool Tas5731m::setMute(bool mute) {
  if (!ready_) {
    return false;
  }

  if (!write8(REG_SOFT_MUTE, mute ? MUTE_CH1_CH2 : UNMUTE_ALL)) {
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

  if (ready_ && !write8(REG_MASTER_VOLUME, masterVolume_)) {
    return false;
  }

  const float db = 24.0f - (0.5f * masterVolume_);
  Serial.printf("[AMP] master volume reg=0x%02X (%.1f dB)\n", masterVolume_, db);
  return true;
}

bool Tas5731m::readErrorStatus(uint8_t &value) {
  return read8(REG_ERROR_STATUS, value);
}

bool Tas5731m::readSystemControl2(uint8_t &value) {
  return read8(REG_SYS_CTRL_2, value);
}

bool Tas5731m::readDeviceId(uint8_t &value) {
  return read8(REG_DEVICE_ID, value);
}
