/*
  Reamped
  -------
  ESP32-S3 + salvaged TAS5731M amplifier bring-up sketch.

  Target:
    Arduino-ESP32 Core 2.0.17

  This .ino intentionally stays small.
  Hardware control, I2S audio, test-tone generation and serial debugging
  are split into separate .h/.cpp modules for easier fault isolation.
*/

#include <Arduino.h>
#include <Wire.h>

#include "AudioOutput.h"
#include "ReampedPins.h"
#include "SerialConsole.h"
#include "Tas5731m.h"
#include "TestTone.h"

Tas5731m amplifier(Wire, REAMPED_PIN_RESET, REAMPED_PIN_PDN);
AudioOutput audioOutput;
TestTone testTone(audioOutput, amplifier);
SerialConsole console(amplifier, testTone);

void setup() {
  Serial.begin(REAMPED_SERIAL_BAUD);
  delay(500);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" Reamped - ESP32-S3 / TAS5731M");
  Serial.println(" Arduino-ESP32 Core 2.0.17 target");
  Serial.println(" Modular debug build");
  Serial.println("======================================");

  amplifier.beginPins();

  Wire.begin(REAMPED_PIN_SDA, REAMPED_PIN_SCL);
  Wire.setClock(100000);

  if (!audioOutput.begin()) {
    Serial.println("[FATAL] I2S initialization failed. Amplifier kept off.");
    console.printHelp();
    return;
  }

  if (!amplifier.begin()) {
    Serial.println("[SAFE] Amplifier remains RESET/PDN low.");
  }

  console.printHelp();
  console.printStatus();
}

void loop() {
  console.service();
  testTone.service();
}
