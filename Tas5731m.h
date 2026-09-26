#pragma once

#include <Arduino.h>
#include <Wire.h>

class Tas5731m {
public:
  static constexpr uint8_t ADDR_LOW  = 0x34;
  static constexpr uint8_t ADDR_HIGH = 0x36;

  Tas5731m(TwoWire &wire, int resetPin, int pdnPin);

  void beginPins();
  void hardSafeState();

  bool begin();
  bool ready() const { return ready_; }
  uint8_t address() const { return address_; }

  bool setMute(bool mute);
  bool muted() const { return muted_; }

  bool setMasterVolume(uint8_t value);
  uint8_t masterVolume() const { return masterVolume_; }

  bool readErrorStatus(uint8_t &value);
  bool readSystemControl2(uint8_t &value);
  bool readDeviceId(uint8_t &value);

private:
  static constexpr uint8_t REG_DEVICE_ID     = 0x01;
  static constexpr uint8_t REG_ERROR_STATUS  = 0x02;
  static constexpr uint8_t REG_SERIAL_IF     = 0x04;
  static constexpr uint8_t REG_SYS_CTRL_2    = 0x05;
  static constexpr uint8_t REG_SOFT_MUTE     = 0x06;
  static constexpr uint8_t REG_MASTER_VOLUME = 0x07;
  static constexpr uint8_t REG_OSC_TRIM      = 0x1B;

  static constexpr uint8_t SERIAL_I2S_16BIT  = 0x03;
  static constexpr uint8_t EXIT_SHUTDOWN     = 0x00;
  static constexpr uint8_t MUTE_CH1_CH2      = 0x03;
  static constexpr uint8_t UNMUTE_ALL        = 0x00;

  TwoWire &wire_;
  int resetPin_;
  int pdnPin_;

  uint8_t address_ = 0;
  bool ready_ = false;
  bool muted_ = true;

  // 0x30 = 0 dB, then -0.5 dB per register step.
  // Start at -48 dB for safe bring-up.
  uint8_t masterVolume_ = 0x90;

  bool addressResponds(uint8_t address);
  uint8_t detectAddress();

  bool write8(uint8_t reg, uint8_t value);
  bool read8(uint8_t reg, uint8_t &value);
};
