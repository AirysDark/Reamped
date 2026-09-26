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

  // Conservative bring-up sequence intended for salvaged hardware.
  // I2S/MCLK should already be running before calling begin().
  bool begin();

  // Graceful output shutdown before removing PVDD.
  bool shutdown();

  bool ready() const { return ready_; }
  uint8_t address() const { return address_; }

  bool setMute(bool mute);
  bool muted() const { return muted_; }

  bool setMasterVolume(uint8_t value);
  uint8_t masterVolume() const { return masterVolume_; }

  bool readErrorStatus(uint8_t &value);
  bool readSystemControl2(uint8_t &value);
  bool readDeviceId(uint8_t &value);

  // Raw I2C access for higher-level DSP/debug modules.
  bool writeRegister8(uint8_t reg, uint8_t value);
  bool readRegister8(uint8_t reg, uint8_t &value);
  bool writeBlock(uint8_t reg, const uint8_t *data, size_t length);
  bool readBlock(uint8_t reg, uint8_t *data, size_t length);

private:
  static constexpr uint8_t REG_DEVICE_ID     = 0x01;
  static constexpr uint8_t REG_ERROR_STATUS  = 0x02;
  static constexpr uint8_t REG_SERIAL_IF     = 0x04;
  static constexpr uint8_t REG_SYS_CTRL_2    = 0x05;
  static constexpr uint8_t REG_SOFT_MUTE     = 0x06;
  static constexpr uint8_t REG_MASTER_VOLUME = 0x07;
  static constexpr uint8_t REG_OSC_TRIM      = 0x1B;

  static constexpr uint8_t SERIAL_I2S_16BIT  = 0x03;
  static constexpr uint8_t ENTER_SHUTDOWN    = 0x40;
  static constexpr uint8_t EXIT_SHUTDOWN     = 0x00;
  static constexpr uint8_t MUTE_CH1_CH2      = 0x03;
  static constexpr uint8_t UNMUTE_ALL        = 0x00;

  // Conservative timing margins for first bring-up.
  static constexpr uint32_t RESET_SETTLE_MS          = 15;
  static constexpr uint32_t OSC_TRIM_SETTLE_MS       = 55;
  static constexpr uint32_t FIRST_STARTUP_GUARD_MS   = 450;
  static constexpr uint32_t POST_SHUTDOWN_EXIT_MS    = 170;
  static constexpr uint32_t POWERDOWN_TO_RESET_MS    = 2;

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
};
