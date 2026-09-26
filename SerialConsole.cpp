#include "SerialConsole.h"

#include <Arduino.h>

#include "Tas5731m.h"
#include "TestTone.h"

SerialConsole::SerialConsole(Tas5731m &amp, TestTone &tone)
: amp_(amp), tone_(tone) {}

void SerialConsole::printHelp() {
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

void SerialConsole::printStatus() {
  Serial.printf(
    "\n[STATUS] ampReady=%s addr=0x%02X muted=%s tone=%s\n",
    amp_.ready() ? "yes" : "no",
    amp_.address(),
    amp_.muted() ? "yes" : "no",
    tone_.enabled() ? "on" : "off"
  );

  if (!amp_.ready()) {
    return;
  }

  uint8_t error = 0;
  if (amp_.readErrorStatus(error)) {
    Serial.printf("[STATUS] TAS5731M error register 0x02 = 0x%02X\n", error);
    if (error == 0) {
      Serial.println("[STATUS] No TAS5731M error bits set.");
    }
  } else {
    Serial.println("[STATUS] Could not read error register.");
  }

  uint8_t sys = 0;
  if (amp_.readSystemControl2(sys)) {
    Serial.printf("[STATUS] system control 2 = 0x%02X\n", sys);
  }
}

void SerialConsole::service() {
  while (Serial.available()) {
    const char c = (char)Serial.read();

    switch (c) {
      case '?':
        printHelp();
        break;

      case 'i':
      case 'I':
        printStatus();
        break;

      case 't':
      case 'T':
        tone_.setEnabled(!tone_.enabled());
        break;

      case 'm':
      case 'M':
        amp_.setMute(!amp_.muted());
        break;

      case '+': {
        const uint8_t v = amp_.masterVolume();
        if (v >= 0x32) {
          amp_.setMasterVolume(v - 2);
        }
        break;
      }

      case '-': {
        const uint8_t v = amp_.masterVolume();
        if (v <= 0xFC) {
          amp_.setMasterVolume(v + 2);
        }
        break;
      }

      default:
        break;
    }
  }
}
