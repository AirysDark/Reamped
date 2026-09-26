/*
  Reamped
  -------
  ESP32-S3 + salvaged TAS5731M amplifier bring-up sketch.

  Target:
    Arduino-ESP32 Core 2.0.17

  The sketch intentionally starts at a very low volume and soft-muted.
  Serial commands:
    ?  help
    i  print amplifier status
    t  toggle 440 Hz stereo test tone
    m  toggle mute
    +  volume up 1 dB
    -  volume down 1 dB

  IMPORTANT:
  - TAS5731M AVDD/DVDD must be supplied with 3.3 V.
  - TAS5731M PVDD is the separate amplifier power rail.
  - Speaker outputs are BTL; speaker "-" is NOT GND.
*/

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "driver/i2s.h"
#include "ReampedPins.h"

namespace Tas5731m {
  static constexpr uint8_t ADDR_LOW  = 0x34;
  static constexpr uint8_t ADDR_HIGH = 0x36;

  static constexpr uint8_t REG_DEVICE_ID      = 0x01;
  static constexpr uint8_t REG_ERROR_STATUS   = 0x02;
  static constexpr uint8_t REG_SERIAL_IF      = 0x04;
  static constexpr uint8_t REG_SYS_CTRL_2     = 0x05;
  static constexpr uint8_t REG_SOFT_MUTE      = 0x06;
  static constexpr uint8_t REG_MASTER_VOLUME  = 0x07;
  static constexpr uint8_t REG_OSC_TRIM       = 0x1B;

  static constexpr uint8_t SERIAL_I2S_16BIT   = 0x03;
  static constexpr uint8_t ENTER_SHUTDOWN     = 0x40;
  static constexpr uint8_t EXIT_SHUTDOWN      = 0x00;
  static constexpr uint8_t MUTE_CH1_CH2       = 0x03;
  static constexpr uint8_t UNMUTE_ALL         = 0x00;
}

static uint8_t g_ampAddress = 0;
static bool g_ampReady = false;
static bool g_toneEnabled = false;
static bool g_muted = true;

// TAS5731M volume register:
// 0x30 = 0 dB, each increment = -0.5 dB.
// Start deliberately quiet: 0x90 = -48 dB.
static uint8_t g_masterVolume = 0x90;

static bool i2cAddressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

static uint8_t detectAmplifierAddress() {
  if (i2cAddressResponds(Tas5731m::ADDR_LOW)) {
    return Tas5731m::ADDR_LOW;
  }
  if (i2cAddressResponds(Tas5731m::ADDR_HIGH)) {
    return Tas5731m::ADDR_HIGH;
  }
  return 0;
}

static bool ampWrite8(uint8_t reg, uint8_t value) {
  if (g_ampAddress == 0) {
    return false;
  }

  Wire.beginTransmission(g_ampAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool ampRead8(uint8_t reg, uint8_t &value) {
  if (g_ampAddress == 0) {
    return false;
  }

  Wire.beginTransmission(g_ampAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)g_ampAddress, 1) != 1) {
    return false;
  }

  value = Wire.read();
  return true;
}

static bool setupI2S() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = REAMPED_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 128;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = REAMPED_MCLK_HZ;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = REAMPED_PIN_MCLK;
  pins.bck_io_num = REAMPED_PIN_BCLK;
  pins.ws_io_num = REAMPED_PIN_LRCLK;
  pins.data_out_num = REAMPED_PIN_SDOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  esp_err_t err = i2s_driver_install((i2s_port_t)REAMPED_I2S_PORT, &cfg, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("[I2S] driver install failed: %d\n", (int)err);
    return false;
  }

  err = i2s_set_pin((i2s_port_t)REAMPED_I2S_PORT, &pins);
  if (err != ESP_OK) {
    Serial.printf("[I2S] pin setup failed: %d\n", (int)err);
    return false;
  }

  err = i2s_set_clk(
    (i2s_port_t)REAMPED_I2S_PORT,
    REAMPED_SAMPLE_RATE,
    I2S_BITS_PER_SAMPLE_16BIT,
    I2S_CHANNEL_STEREO
  );
  if (err != ESP_OK) {
    Serial.printf("[I2S] clock setup failed: %d\n", (int)err);
    return false;
  }

  i2s_zero_dma_buffer((i2s_port_t)REAMPED_I2S_PORT);

  // Prime the transmitter with silence so MCLK/BCLK/LRCLK are active.
  int16_t silence[64] = {};
  size_t written = 0;
  i2s_write(
    (i2s_port_t)REAMPED_I2S_PORT,
    silence,
    sizeof(silence),
    &written,
    portMAX_DELAY
  );

  Serial.println("[I2S] 48 kHz, stereo, 16-bit I2S, MCLK=12.288 MHz");
  return true;
}

static void ampHardSafeState() {
  digitalWrite(REAMPED_PIN_RESET, LOW);
  digitalWrite(REAMPED_PIN_PDN, LOW);
}

static bool initializeAmplifier() {
  // TI startup requirement: establish known digital states, deassert PDN,
  // then hold RESET low for at least 100 us before releasing RESET.
  digitalWrite(REAMPED_PIN_RESET, LOW);
  digitalWrite(REAMPED_PIN_PDN, HIGH);
  delayMicroseconds(200);
  digitalWrite(REAMPED_PIN_RESET, HIGH);

  // Datasheet requires at least 13.5 ms before normal configuration.
  delay(15);

  g_ampAddress = detectAmplifierAddress();
  if (g_ampAddress == 0) {
    Serial.println("[AMP] TAS5731M not found at I2C 0x34 or 0x36.");
    ampHardSafeState();
    return false;
  }

  Serial.printf("[AMP] TAS5731M responding at I2C 0x%02X\n", g_ampAddress);

  // Datasheet initialization sequence: oscillator trim = 0x00, then >=50 ms.
  if (!ampWrite8(Tas5731m::REG_OSC_TRIM, 0x00)) {
    Serial.println("[AMP] Oscillator trim write failed.");
    ampHardSafeState();
    return false;
  }
  delay(55);

  // ESP32 stream is 16-bit Philips/I2S.
  if (!ampWrite8(Tas5731m::REG_SERIAL_IF, Tas5731m::SERIAL_I2S_16BIT)) {
    Serial.println("[AMP] Serial interface setup failed.");
    ampHardSafeState();
    return false;
  }

  // Keep the output muted while exiting shutdown.
  ampWrite8(Tas5731m::REG_MASTER_VOLUME, 0xFF);
  ampWrite8(Tas5731m::REG_SOFT_MUTE, Tas5731m::MUTE_CH1_CH2);

  // 2.0 BTL mode, leave all-channel shutdown.
  if (!ampWrite8(Tas5731m::REG_SYS_CTRL_2, Tas5731m::EXIT_SHUTDOWN)) {
    Serial.println("[AMP] Failed to exit shutdown.");
    ampHardSafeState();
    return false;
  }

  delay(20);

  // Load the deliberately low startup volume, but remain soft-muted.
  ampWrite8(Tas5731m::REG_MASTER_VOLUME, g_masterVolume);
  g_muted = true;

  uint8_t id = 0;
  if (ampRead8(Tas5731m::REG_DEVICE_ID, id)) {
    Serial.printf("[AMP] Device-ID register: 0x%02X\n", id);
  }

  Serial.println("[AMP] Initialized and muted.");
  return true;
}

static void setMute(bool mute) {
  if (!g_ampReady) {
    return;
  }

  ampWrite8(
    Tas5731m::REG_SOFT_MUTE,
    mute ? Tas5731m::MUTE_CH1_CH2 : Tas5731m::UNMUTE_ALL
  );

  g_muted = mute;
  Serial.printf("[AMP] %s\n", mute ? "MUTED" : "UNMUTED");
}

static void setMasterVolume(uint8_t value) {
  // Keep this bring-up firmware below 0 dB.
  if (value < 0x30) {
    value = 0x30;
  }
  if (value > 0xFE) {
    value = 0xFE;
  }

  g_masterVolume = value;

  if (g_ampReady) {
    ampWrite8(Tas5731m::REG_MASTER_VOLUME, g_masterVolume);
  }

  const float db = 24.0f - (0.5f * g_masterVolume);
  Serial.printf("[AMP] master volume reg=0x%02X (%.1f dB)\n", g_masterVolume, db);
}

static void printAmpStatus() {
  Serial.printf("\n[STATUS] ampReady=%s addr=0x%02X muted=%s tone=%s\n",
                g_ampReady ? "yes" : "no",
                g_ampAddress,
                g_muted ? "yes" : "no",
                g_toneEnabled ? "on" : "off");

  if (!g_ampReady) {
    return;
  }

  uint8_t error = 0;
  if (ampRead8(Tas5731m::REG_ERROR_STATUS, error)) {
    Serial.printf("[STATUS] TAS5731M error register 0x02 = 0x%02X\n", error);
    if (error == 0) {
      Serial.println("[STATUS] No TAS5731M error bits set.");
    }
  } else {
    Serial.println("[STATUS] Could not read error register.");
  }

  uint8_t sys = 0;
  if (ampRead8(Tas5731m::REG_SYS_CTRL_2, sys)) {
    Serial.printf("[STATUS] system control 2 = 0x%02X\n", sys);
  }
}

static void printHelp() {
  Serial.println();
  Serial.println("Reamped serial commands:");
  Serial.println("  ?  help");
  Serial.println("  i  amplifier status");
  Serial.println("  t  toggle low-level 440 Hz test tone");
  Serial.println("  m  toggle mute");
  Serial.println("  +  volume up 1 dB");
  Serial.println("  -  volume down 1 dB");
  Serial.println();
}

static void serviceSerial() {
  while (Serial.available()) {
    const char c = (char)Serial.read();

    switch (c) {
      case '?':
        printHelp();
        break;

      case 'i':
      case 'I':
        printAmpStatus();
        break;

      case 't':
      case 'T':
        if (!g_ampReady) {
          Serial.println("[AMP] Amplifier is not ready.");
          break;
        }
        g_toneEnabled = !g_toneEnabled;
        if (g_toneEnabled) {
          setMute(false);
          Serial.println("[AUDIO] Test tone ON.");
        } else {
          setMute(true);
          Serial.println("[AUDIO] Test tone OFF.");
        }
        break;

      case 'm':
      case 'M':
        setMute(!g_muted);
        break;

      case '+':
        // Register steps are 0.5 dB, so 1 dB louder = subtract 2.
        if (g_masterVolume >= 0x32) {
          setMasterVolume(g_masterVolume - 2);
        }
        break;

      case '-':
        // 1 dB quieter = add 2.
        if (g_masterVolume <= 0xFC) {
          setMasterVolume(g_masterVolume + 2);
        }
        break;

      default:
        break;
    }
  }
}

static void serviceTestTone() {
  static float phase = 0.0f;

  constexpr float frequency = 440.0f;
  constexpr float amplitude = 1200.0f; // intentionally low level
  constexpr size_t frames = 128;

  int16_t samples[frames * 2];

  if (!g_toneEnabled || !g_ampReady || g_muted) {
    memset(samples, 0, sizeof(samples));
  } else {
    const float phaseStep = 2.0f * PI * frequency / (float)REAMPED_SAMPLE_RATE;

    for (size_t i = 0; i < frames; ++i) {
      const int16_t s = (int16_t)(sinf(phase) * amplitude);
      phase += phaseStep;
      if (phase >= 2.0f * PI) {
        phase -= 2.0f * PI;
      }

      samples[(i * 2) + 0] = s;
      samples[(i * 2) + 1] = s;
    }
  }

  size_t written = 0;
  i2s_write(
    (i2s_port_t)REAMPED_I2S_PORT,
    samples,
    sizeof(samples),
    &written,
    portMAX_DELAY
  );
}

void setup() {
  Serial.begin(REAMPED_SERIAL_BAUD);
  delay(500);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" Reamped - ESP32-S3 / TAS5731M");
  Serial.println(" Arduino-ESP32 Core 2.0.17 target");
  Serial.println("======================================");

  pinMode(REAMPED_PIN_RESET, OUTPUT);
  pinMode(REAMPED_PIN_PDN, OUTPUT);
  ampHardSafeState();

  Wire.begin(REAMPED_PIN_SDA, REAMPED_PIN_SCL);
  Wire.setClock(100000);

  if (!setupI2S()) {
    Serial.println("[FATAL] I2S initialization failed. Amplifier kept off.");
    printHelp();
    return;
  }

  g_ampReady = initializeAmplifier();

  if (!g_ampReady) {
    Serial.println("[SAFE] Amplifier remains RESET/PDN low.");
  }

  printHelp();
  printAmpStatus();
}

void loop() {
  serviceSerial();
  serviceTestTone();
}
