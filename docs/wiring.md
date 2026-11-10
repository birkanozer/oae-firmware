# Wiring — MKS Robin E3 v1.1

Single source for pins and headers. Sources: Marlin `pins_MKS_ROBIN_E3_common.h` /
`pins_MKS_ROBIN_E3_V1_1_common.h`, Klipper `generic-mks-robin-e3.cfg`, makerbase schematic
`MKS Robin E3 V1.1_004`. **Our board is `_0003`**: rows marked ⚠ are unverified on it.

## MCU

STM32F103RCT6, Cortex-M3 72 MHz, 256 KB flash, 48 KB RAM (device id `0x10036414`, read over
SWD). Firmware at `0x08000000`, flashed with ST-Link V2 over SWD (J1, 2×3: SWDIO PA13, SWCLK
PA14, RESET, 3.3 V, GND ⚠ pin order). Factory image backed up in `backup/`.

Timers: stepper ISR on TIM6 at the lowest interrupt priority; TMC SoftwareSerial on TIM7
(`TIMER_SERIAL`); buzzer `tone()` on a free timer (see `platformio.ini` `TIMER_TONE`).

## Steppers (TMC2209, sense 0.11 Ω, single-wire UART per driver)

| Socket | Axis (default) | STEP | DIR | EN (low = on) | UART |
|---|---|---|---|---|---|
| X | RA | PC0 | PB2 | PC13 | PC7 |
| Y | DEC | PC2 | PB9 | PB12 | PD2 |
| Z | AZ (example config) or ALT | PC14 ⚠ | PC15 ⚠ | PB8 ⚠ | PC12 |
| E | Focus (example config) | PB4 | PB3 | PB5 | PC11 |

The Z socket carries AZ in the example config (`AZ_STEP_PIN` = `ALT_STEP_PIN` and so on) or ALT, never
both: the pins file stops the build with `#error` if both are enabled. ALT stays off. The E socket
carries the focuser in the example config.

Motor connectors XH2.54-4P (Ender-3 order ⚠).

## ULN2003 (28BYJ-48, unipolar) on EXP1 and EXP2

EXP1 (1–10): 1 PC1 · 2 PC3 · 3 PA4 · 4 PA5 · 5 PA6 · 6 PA7 · 7 PC4 · 8 PC5 · 9 GND · 10 5 V.
Pins 1–5 are **not free**: they are the same MCU pins (PC1, PC3, PA4, PA5, PA6) that the onboard
screen uses through the "Ender-3 EXP1" header (see below), so they are taken by the screen. Only
pins 6–8 (PA7, PC4, PC5) are available here, as ULN group B; 9/10 are GND and 5 V.

| Group | IN1 | IN2 | IN3 | IN4 | Power |
|---|---|---|---|---|---|
| A (AZ) | PB13 (EXP2 2) | PB14 (EXP2 1) | PB15 (EXP2 6) | PA15 (EXP2) | ULN 5 V + GND from EXP1 9/10 |
| B (focus) | PA7 (6) | PC4 (7) | PC5 (8) | PC10 (EXP2 7) | same |

The 5 V rail on EXP1 is the board's buck output (MP1584, 3 A); two 28BYJ-48 draw ~240 mA each
at 5 V. 3.3 V logic drives the ULN2003 inputs fine. **PA15 and PB3/PB4 are JTAG pins at reset:
firmware disables JTAG and keeps SWD.**

## Focus motor: bipolar-modded 28BYJ-48 on the E socket (TMC2209, UART)

Same motor mod, coil pairs (find them by continuity), current formula and safety rules as the AZ
motor below, on the E socket (PB4 step, PB3 dir, PB5 EN, PC11 UART). Peak 150 mA = about 91 mA RMS
(`FOCUS_MOTOR_CURRENT_RATING` is a peak value, RMS = rating x `FOCUS_OPERATING_CURRENT_SETTING` % / 1.414),
hold 30 % (`FOCUSER_MOTOR_HOLD_SETTING`), released when idle (`FOCUSER_ALWAYS_ON 0`). EN stays high until
the UART current is set. If the focus UART does not answer at boot, every focus move is refused
(no motion, no position change). `UART_CONNECTION_TEST_TXRX 1` is mandatory (`#error` otherwise).
2048 steps per turn, 16 microsteps, 400 full steps/s and 200 full steps/s^2.

## AZ motor: bipolar-modded 28BYJ-48 on the Z socket (TMC2209, UART)

Open the blue cap and cut the PCB trace that joins the red common wire to the coil centres, then
drop the red wire. The five wires become two separate coils, each with both halves in series.
Plug the remaining four wires into the Z socket (PC14 step, PC15 dir, PB8 EN, PC12 UART).

**Find the coils by continuity, not by wire colour.** With a multimeter on ohms, test the 6 pairs
of the four wires. Two pairs read about 2 x R_half (typically 44-100 Ω): each is one coil. The
other four pairs read open (the coils are not connected). Wire one pair to the socket's A1/A2 and
the other pair to B1/B2. If the motor only shakes or turns the wrong way, swap the two wires of
one coil (reverses direction) or recheck the pairing. Write the measured pair resistance down:
it is `AZ_COIL_RESISTANCE_OHM` (default 50 Ω for R_half of about 25 Ω).

**Current.** A current-chopping driver sets the coil current itself, so the 5 V motor on 12 or
24 V is fine once the UART current is applied. The firmware keeps EN high (driver off) until
the UART current is set. If the AZ UART does not answer at boot, AZ stays disabled and every AZ
move is refused (no motion, no position change), so it never runs on the board's VREF current.
`UART_CONNECTION_TEST_TXRX 1` is mandatory for AZ on this board (`#error` otherwise).

- Rated half-coil current = 5 V / R_half. In bipolar the two halves are in series, so equal
  torque needs about 0.5x that current and equal heat about 0.7x.
- For intermittent duty: **Irms = 0.7 x 5 / R_half** (R_half 25 Ω gives 140 mA, 50 Ω gives 70 mA).
- `AZ_MOTOR_CURRENT_RATING` is a **peak** value: RMS = rating x `AZ_OPERATING_CURRENT_SETTING` % / 1.414.
  Set rating = Irms x 1.414 at 100 %.
- The TMC2209 library programs the current in coarse steps. With R_SENSE 0.11 Ω (+0.02 Ω internal)
  at this low current it uses vsense = 1: IRUN 1 = 61 mA, **IRUN 2 = 91 mA**, IRUN 3 = 122 mA,
  IRUN 4 = 153 mA RMS. Any request of 92-122 mA gives IRUN 2. The default (150 mA peak, 106 mA
  RMS requested) therefore lands on **91 mA RMS**. Pick a request that falls inside the bin you want.
- Hold is 30 % (`AZ_MOTOR_HOLD_SETTING`, rounds to IHOLD 0 here) and `AZ_ALWAYS_ON 0` releases EN
  when the axis is idle, so nothing is powered at rest. The motor can still run warm during moves.

**ULN2003 alternative** (the stock board, no cut; firmware driver is a later slice): use the
EXP1/EXP2 table above.

**SoftwareSerial and the E socket's UART (PC11).** PC11 shares EXTI line 11 with knob A (PB11),
but STM32duino SoftwareSerial does not use `attachInterrupt` on the RX pin: it samples the pin
from the `TIMER_SERIAL` (TIM7) tick at 3x oversampling, and with RX == TX it runs half-duplex. No
pin EXTI line is used by TMC UART, so there is no conflict with the knob. The AZ UART (PC12) is on
line 12 with the RA endstop PA12, and also does not use EXTI.

## Endstops (microswitch, NC or NO set in firmware, active state configurable)

| Header | Pin | Use |
|---|---|---|
| X- | PA12 | RA home |
| Y- | PA11 | DEC home |
| Z- | PC6 | AZ home |
| Z+ / probe | PB1 | spare (ALT home or RA hard limit) |

Wire switch between signal and GND. Treat inputs as 3.3 V.

Pins are defined as upstream `*_DIAG_PIN`s and are not read yet. Today `b_setup` only sets them
`INPUT` (no internal pull-up), so a switch needs an external pull-up until the homing slice sets
INPUT_PULLUP. PA11/PA12 are also the MCU's USB pins: unused, USB goes via the CH340.

## ESP32-S3 bridge (USART2)

USART2 (`Serial2`) at 115200 baud.

| Robin E3 | Header | ESP32-S3-DevKitC-1 |
|---|---|---|
| PA2 (USART2 TX) | NeoPixel header | RX (GPIO 18, set in `bridge/`) |
| PA3 (USART2 RX) | J24 BLTouch servo (GND, VCC, PA3 ⚠ order) | TX (GPIO 17) |
| 5 V | EXP1 pin 10 | 5V pin |
| GND | EXP1 pin 9 | GND |

3.3 V logic both sides, no level shifter.

## Onboard screen (stock Ender 3 LCD ENH12864Z-1) on the "Ender-3 EXP1" header

The display's EXP3 connector plugs into the header labelled **"Ender-3 EXP1"** (third 10-pin
header; Marlin `EXP3_*`, Klipper `CR_EXP3`) with the stock cable. PSU off, connector key matched.
The ST7920 is 128×64 monochrome, driven by U8g2 (BSD-2-Clause) with software SPI, so the LCD pins
do not line up with SPI1. The pins are driven by direct register writes, and only the changed
8-pixel rows of each frame are sent. `LCD_SPI_HALF_PERIOD_NS` (default 500, a 1 �s clock) sets the
clock half period: raise it (for example 1000) if the screen shows garbage. `LCD_SHOW_FRAME_MS=1`
draws the last flush time in ms in the bottom-right corner, for timing on the bench (default off).

Pin map of the "Ender-3 EXP1" header (checked against Marlin `pins_MKS_ROBIN_E3_common.h`):

| Header pin | Signal | Function | Notes |
|---|---|---|---|
| 1 | PC1 | Buzzer | `tone()`, works with active and passive buzzers |
| 2 | PC3 | Click | active low, pull-up |
| 3 | PB11 | Encoder A | EXTI, internal pull-up; swap via `ENCODER_INVERT` |
| 4 | RESET | LCD reset | not driven by firmware |
| 5 | PB0 | Encoder B | EXTI, internal pull-up |
| 6 | PA6 | LCD SCK (E) | software SPI |
| 7 | PA5 | LCD CS (RS) | software SPI |
| 8 | PA4 | LCD data (SID / RW) | software SPI |
| 9 | GND | | |
| 10 | 5 V | | |

EXTI lines 0 and 11 are taken by the knob (PB0, PB11); no other pin numbered 0 or 11 may use
`attachInterrupt`. SoftwareSerial (TMC UART) samples from a timer and does not clash.

PB10 and PB11 are no longer reserved for a screen: PB11 is now encoder A, and PB10 is free (the
filament-runout input is unused).

## Other

- USB (mini-B) → CH340 → USART1 PA9/PA10 (`Serial`, 19200): LX200 for ASCOM / Stellarium desktop / debug.
- AT24C32 EEPROM on I²C PB6 SCL / PB7 SDA ⚠ (verify by I²C scan).
- Power input 12–24 V. Unused: heaters PC8/PC9, fan PA8, thermistors PA0/PA1, runout PB10,
  SD on SPI2 (EXP2).
