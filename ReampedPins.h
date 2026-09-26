#pragma once

// Reamped default ESP32-S3 pin assignment.
// Target: Arduino-ESP32 Core 2.0.17
//
// Change these values to suit the final bare-module PCB routing.

static constexpr int REAMPED_PIN_MCLK  = 1;   // TAS5731M pin 15
static constexpr int REAMPED_PIN_BCLK  = 2;   // TAS5731M pin 21 (SCLK)
static constexpr int REAMPED_PIN_LRCLK = 3;   // TAS5731M pin 20
static constexpr int REAMPED_PIN_SDOUT = 4;   // ESP32 TX audio -> TAS5731M pin 22 (SDIN)

static constexpr int REAMPED_PIN_SDA   = 8;   // TAS5731M pin 23
static constexpr int REAMPED_PIN_SCL   = 9;   // TAS5731M pin 24

static constexpr int REAMPED_PIN_RESET = 10;  // TAS5731M pin 25, active LOW
static constexpr int REAMPED_PIN_PDN   = 11;  // TAS5731M pin 19, active LOW

static constexpr int REAMPED_I2S_PORT = 0;
static constexpr unsigned long REAMPED_SERIAL_BAUD = 115200;
static constexpr uint32_t REAMPED_SAMPLE_RATE = 48000;
static constexpr uint32_t REAMPED_MCLK_HZ = 12288000; // 256 * 48 kHz
