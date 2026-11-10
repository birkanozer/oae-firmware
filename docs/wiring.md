# Wiring — MKS Robin E3 v1.1

Single source for pins and headers. Sources: Marlin `pins_MKS_ROBIN_E3_common.h` /
`pins_MKS_ROBIN_E3_V1_1_common.h`, Klipper `generic-mks-robin-e3.cfg`, makerbase schematic
`MKS Robin E3 V1.1_004`. **Our board is `_0003`**: rows marked ⚠ are unverified on it.

## MCU

STM32F103RCT6, Cortex-M3 72 MHz, 256 KB flash, 48 KB RAM (device id `0x10036414`, read over
SWD). Firmware at `0x08000000`, flashed with ST-Link V2 over SWD (J1, 2×3: SWDIO PA13, SWCLK
PA14, RESET, 3.3 V, GND ⚠ pin order). Factory image backed up in `backup/`.

## Steppers (TMC2209, sense 0.11 Ω, single-wire UART per driver)

| Socket | Axis (default) | STEP | DIR | EN (low = on) | UART |
|---|---|---|---|---|---|
| X | RA | PC0 | PB2 | PC13 | PC7 |
| Y | DEC | PC2 | PB9 | PB12 | PD2 |
| Z | free (ALT or bipolar AZ) | PC14 ⚠ | PC15 ⚠ | PB8 ⚠ | PC12 |
| E | free (bipolar focus) | PB4 | PB3 | PB5 | PC11 |

Motor connectors XH2.54-4P (Ender-3 order ⚠).

## ULN2003 (28BYJ-48, unipolar) on EXP1

EXP1 (1–10): 1 PC3 · 2 PC1 · 3 PA4 · 4 PA5 · 5 PA6 · 6 PA7 · 7 PC4 · 8 PC5 · 9 GND · 10 5 V.

| Group | IN1 | IN2 | IN3 | IN4 | Power |
|---|---|---|---|---|---|
| A (AZ) | PA4 (3) | PA5 (4) | PA6 (5) | PA7 (6) | ULN 5 V + GND from EXP1 9/10 |
| B (focus) | PC4 (7) | PC5 (8) | PC1 (2) | PC3 (1) | same |

The 5 V rail on EXP1 is the board's buck output (MP1584, 3 A); two 28BYJ-48 draw ~240 mA each
at 5 V. 3.3 V logic drives the ULN2003 inputs fine.

**Bipolar alternative:** open the 28BYJ-48's blue cap, cut the trace joining the red common to
the coil centre, drop the red wire; plug the remaining four wires into a free TMC2209 socket
(coil A = orange/pink, coil B = yellow/blue). Set run current ~150–200 mA.

## Endstops (microswitch, NC or NO set in firmware, active state configurable)

| Header | Pin | Use |
|---|---|---|
| X- | PA12 | RA home |
| Y- | PA11 | DEC home |
| Z- | PC6 | AZ home |
| Z+ / probe | PB1 | spare (ALT home or RA hard limit) |

Wire switch between signal and GND (internal pull-up). Treat inputs as 3.3 V.

## ESP32-S3 bridge (USART2)

| Robin E3 | Header | ESP32-S3-DevKitC-1 |
|---|---|---|
| PA2 (USART2 TX) | NeoPixel header | RX (GPIO 18, set in `bridge/`) |
| PA3 (USART2 RX) | J24 BLTouch servo (GND, VCC, PA3 ⚠ order) | TX (GPIO 17) |
| 5 V | EXP1 pin 10 | 5V pin |
| GND | EXP1 pin 9 | GND |

3.3 V logic both sides, no level shifter.

## Ender 3 V2 screen (DWIN T5UIC1) — USART3 + EXP2

The screen's 10-pin connector, holes numbered on the stock cable plug (Klipper-dwin-ui,
Marlin PR #24983, Creality V4 pins). **Sources disagree on pins 1/2 (NC vs 5 V/GND): check with
a multimeter before powering.** TX/RX naming differs between sources too; if the screen stays
blank, swap 3 and 4.

| Screen pin | Signal | Robin E3 | Header |
|---|---|---|---|
| 3 | screen RX ← board TX | PB10 (USART3 TX) | J27 filament-runout header |
| 4 | screen TX → board RX | PB11 (USART3 RX) | EXP2 pin 3 |
| 5 | ENT (knob click) | PC10 | EXP2 pin 7 |
| 6 | BEEP | PB13 | EXP2 pin 2 |
| 7 | ENC B | PA15 | EXP2 pin 4 |
| 8 | ENC A | PB0 | EXP2 pin 5 |
| 9 | GND | GND | EXP2 pin 9 |
| 10 | 5 V | 5 V | EXP1 pin 10 |

115200 baud, 3.3 V signals (measured ~3.2 V). Encoder inputs use internal pull-ups. Needs an
adapter cable (Dupont jumpers or a small adapter board); neither EXP header matches the screen
pin-for-pin. Consequences: no filament-runout input (unused), no onboard SD card (EXP2 SPI
pins PB13 shared; unused). **PA15 and PB3/PB4 are JTAG pins at reset: firmware disables JTAG and
keeps SWD.**

## Other

- USB (mini-B) → CH340 → USART1 PA9/PA10: LX200 for ASCOM / Stellarium desktop / debug.
- AT24C32 EEPROM on I²C PB6 SCL / PB7 SDA ⚠ (verify by I²C scan).
- Power input 12–24 V. Unused: heaters PC8/PC9, fan PA8, thermistors PA0/PA1, runout PB10,
  SD on SPI2 (EXP2).
