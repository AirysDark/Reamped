# Reamped

Reamped is an Arduino IDE project for rebuilding the salvaged Bluetooth speaker amplifier around an **ESP32-S3** and the original **Texas Instruments TAS5731M** Class-D amplifier section.

## Target

- MCU: ESP32-S3
- Arduino IDE
- Arduino-ESP32 Core: **2.0.17**
- Amplifier: **TAS5731M**
- Audio link: I2S from ESP32-S3 to TAS5731M
- Control link: I2C from ESP32-S3 to TAS5731M
- Amplifier power stage: original salvaged PCB section with the TAS5731M, four output inductors, bulk capacitors and speaker connector retained.

## Important electrical notes

- TAS5731M PVDD power stage: **8 V to 26.4 V absolute operating range; 24 V is the normal high-power target**.
- TAS5731M AVDD and DVDD: **3.3 V nominal**.
- ESP32-S3 GPIO is **3.3 V only**.
- The speaker outputs are BTL. **Do not connect L- or R- to GND.**
- Keep the original four output inductors, bootstrap capacitors and local decoupling parts on the salvaged amplifier PCB.
- The TAS5731M is an I2S slave, so the ESP32-S3 must supply MCLK, BCLK/SCLK and LRCLK.
- TAS5731M I2C address is **0x34** when ADR/FAULT is pulled LOW and **0x36** when ADR/FAULT is pulled HIGH.
- The cut donor amplifier board may no longer contain its original 3.3 V regulator. **Verify AVDD pin 13 and DVDD pin 27 actually receive 3.3 V before applying PVDD.**

## Suggested ESP32-S3 wiring

These GPIO numbers are the project defaults and can be changed in `ReampedPins.h`.

| ESP32-S3 GPIO | TAS5731M pin | Signal |
|---:|---:|---|
| GPIO1 | 15 | MCLK |
| GPIO2 | 21 | SCLK / I2S BCLK |
| GPIO3 | 20 | LRCLK / WS |
| GPIO4 | 22 | SDIN / I2S data |
| GPIO8 | 23 | SDA |
| GPIO9 | 24 | SCL |
| GPIO10 | 25 | RESET, active LOW |
| GPIO11 | 19 | PDN, active LOW |
| 3V3 | 13 | AVDD |
| 3V3 | 27 | DVDD |
| GND | 9 | AVSS |
| GND | 17 | DVSSO |
| GND | 28 | DVSS |
| GND | 29 | GND |
| GND | 30 | AGND |
| GND | 37,38 | PGND_CD |
| GND | 47,48 | PGND_AB |

The salvaged board's existing high-current power connector should feed PVDD through its existing PCB traces. Do not run speaker-current power through the ESP32-S3 board.

## TAS5731M full 48-pin pinout

The table below follows the TI TAS5731M datasheet for the 48-pin HTQFP package.

| Pin | Name | Function |
|---:|---|---|
| 1 | OUT_A | Half-bridge output A |
| 2 | PVDD_AB | Power supply for half-bridges A/B |
| 3 | PVDD_AB | Power supply for half-bridges A/B |
| 4 | BST_A | Bootstrap A |
| 5 | NC | No connect |
| 6 | SSTIMER | Soft-start/ramp timing |
| 7 | NC | No connect |
| 8 | PBTL | BTL/PBTL mode select; LOW/default = BTL |
| 9 | AVSS | Analog 3.3 V supply ground |
| 10 | PLL_FLTM | PLL loop-filter negative |
| 11 | PLL_FLTP | PLL loop-filter positive |
| 12 | VR_ANA | Internal 1.8 V analog regulator output; do not use externally |
| 13 | AVDD | 3.3 V analog supply |
| 14 | ADR/FAULT | I2C address select at power-up / optional fault output |
| 15 | MCLK | Master clock input |
| 16 | OSC_RES | Oscillator trim resistor connection |
| 17 | DVSSO | Oscillator ground |
| 18 | VR_DIG | Internal 1.8 V digital regulator output; do not use externally |
| 19 | PDN | Power-down input, active LOW |
| 20 | LRCLK | I2S left/right clock |
| 21 | SCLK | I2S bit clock |
| 22 | SDIN | I2S serial audio data input |
| 23 | SDA | I2C data |
| 24 | SCL | I2C clock |
| 25 | RESET | Reset input, active LOW |
| 26 | STEST | Factory test; connect to DVSS |
| 27 | DVDD | 3.3 V digital supply |
| 28 | DVSS | Digital ground |
| 29 | GND | Analog/power-stage ground |
| 30 | AGND | Local analog ground for power stage |
| 31 | VREG | Internal digital regulator output; do not power external circuitry |
| 32 | GVDD_OUT | Internal gate-drive regulator output |
| 33 | BST_D | Bootstrap D |
| 34 | PVDD_CD | Power supply for half-bridges C/D |
| 35 | PVDD_CD | Power supply for half-bridges C/D |
| 36 | OUT_D | Half-bridge output D |
| 37 | PGND_CD | Power ground C/D |
| 38 | PGND_CD | Power ground C/D |
| 39 | OUT_C | Half-bridge output C |
| 40 | NC | No connect |
| 41 | NC | No connect |
| 42 | BST_C | Bootstrap C |
| 43 | BST_B | Bootstrap B |
| 44 | NC | No connect |
| 45 | NC | No connect |
| 46 | OUT_B | Half-bridge output B |
| 47 | PGND_AB | Power ground A/B |
| 48 | PGND_AB | Power ground A/B |

The exposed PowerPAD underneath the IC must remain connected to system ground through the original PCB.

## Pins we need for the ESP32-S3 rebuild

The minimum control/audio set is:

- Pin 13 AVDD -> 3.3 V
- Pin 27 DVDD -> 3.3 V
- Pin 15 MCLK <- ESP32-S3
- Pin 20 LRCLK <- ESP32-S3
- Pin 21 SCLK/BCLK <- ESP32-S3
- Pin 22 SDIN <- ESP32-S3
- Pin 23 SDA <-> ESP32-S3
- Pin 24 SCL <- ESP32-S3
- Pin 25 RESET <- ESP32-S3
- Pin 19 PDN <- ESP32-S3
- Common GND between ESP32-S3 and amplifier

For the cut donor board, do not solder to the TAS5731M legs unless necessary. Continuity-trace each required pin to a larger resistor pad, capacitor pad or via and solder there.


## Code layout

The firmware is intentionally split across multiple `.h` / `.cpp` files so each subsystem can be debugged independently in Arduino IDE:

| File | Purpose |
|---|---|
| `Reamped.ino` | Minimal application entry point; creates objects and calls setup/service methods |
| `ReampedPins.h` | ESP32-S3 GPIO assignments and project-wide clock/sample-rate constants |
| `Tas5731m.h` / `Tas5731m.cpp` | TAS5731M I2C driver, conservative startup/shutdown sequencing, mute, volume, raw register access and status |
| `Tas5731mDsp.h` / `Tas5731mDsp.cpp` | Dedicated multi-byte DSP register access for later EQ, biquad, routing and DRC work |
| `AudioOutput.h` / `AudioOutput.cpp` | ESP32-S3 I2S setup and audio sample transmission |
| `TestTone.h` / `TestTone.cpp` | Low-level 440 Hz diagnostic tone generator |
| `SerialConsole.h` / `SerialConsole.cpp` | Serial debug commands and amplifier status reporting |

This keeps amplifier faults, I2S faults and command/UI faults separate instead of putting the whole project in one large `.ino` file.

## Arduino IDE setup

1. Install Espressif Arduino-ESP32 **2.0.17**.
2. Select an ESP32-S3 board matching the hardware being used.
3. Open `Reamped.ino`.
4. Edit `ReampedPins.h` if different GPIOs are required.
5. Start with the amplifier power supply disconnected and verify the 3.3 V/control wiring first.
6. Power the salvaged amplifier section only after checking PVDD-to-GND for a hard short.

## Current firmware

The sketch currently provides a safe bring-up path:

- holds TAS5731M in reset/power-down during boot
- starts I2C at 100 kHz
- checks both valid TAS5731M addresses: 0x34 and 0x36
- configures ESP32-S3 I2S for 48 kHz stereo
- supplies MCLK/BCLK/LRCLK/SDIN
- follows the TAS5731M reset timing
- performs oscillator trim
- uses a conservative **450 ms first-start guard** before leaving shutdown
- waits **170 ms after leaving shutdown** before restoring the requested volume
- selects 16-bit I2S mode
- starts from muted volume
- exits shutdown only when the amplifier responds on I2C
- generates an optional low-level stereo test tone
- reports TAS5731M error status over Serial
- supports graceful shutdown/restart from the Serial console
- exposes block register I/O for future TAS5731M DSP programming

The code deliberately starts quietly. Increase volume only after the speaker wiring and supply rails have been verified.


## Power-up sequence used by Reamped

The salvaged board no longer has the original lower control/power section, so Reamped deliberately uses conservative timing during first bring-up:

```text
1. ESP32-S3 starts I2S first so MCLK/BCLK/LRCLK are present.
2. TAS5731M RESET = LOW, PDN = HIGH.
3. Wait >100 us.
4. RESET = HIGH.
5. Wait 15 ms.
6. Detect I2C address (0x34 or 0x36).
7. Write oscillator trim register 0x1B = 0x00.
8. Wait 55 ms.
9. Configure 16-bit I2S and force mute.
10. Wait 450 ms first-start guard.
11. Exit all-channel shutdown.
12. Wait 170 ms for output-stage soft start.
13. Restore the requested low startup volume (-48 dB).
14. Remain muted until explicitly unmuted/tested.
```

This is intentionally slower than the minimum path because the donor PCB rail ramp characteristics are currently unknown.

Before first hardware power-up, verify:

- AVDD pin 13 -> approximately 3.3 V
- DVDD pin 27 -> approximately 3.3 V
- PVDD-to-GND is not a hard short
- ESP32-S3 GND and amplifier GND are common
- speaker outputs are not tied to GND

## Serial debug commands

```text
?  help
i  amplifier status
t  toggle low-level 440 Hz test tone
m  toggle mute
p  graceful amplifier shutdown
r  rerun amplifier bring-up sequence
+  volume up 1 dB
-  volume down 1 dB
```

## DSP expansion

The TAS5731M contains internal DSP resources, so future Reamped firmware can move tone shaping and protection into the amplifier instead of spending ESP32-S3 CPU time on every audio sample. The dedicated `Tas5731mDsp.*` module is reserved for verified TI-format multi-byte transactions such as:

- biquad EQ
- bass/mid/treble presets
- channel routing/mixing
- dynamic-range compression
- limiter/protection tuning

No guessed coefficients are written by the current firmware.

## Salvaged speaker connector

The original speaker connector should remain connected to the four LC-filter outputs on the donor PCB:

```text
L+ ---- left speaker ---- L-
R+ ---- right speaker --- R-
```

Neither negative speaker terminal is chassis/system ground.

## Reference

Texas Instruments: TAS5731M, “2 × 30-W Digital Audio Power Amplifier With DSP and 2.1 Mode”, Rev. C.
