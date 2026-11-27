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
| Z | AZ | PC14 ⚠ | PC15 ⚠ | PB8 ⚠ | PC12 |
| E | ALT | PB4 | PB3 | PB5 | PC11 |

The Z socket carries AZ and the E socket carries ALT, both bipolar-modded 28BYJ-48 (below). The focuser
is not on a socket: it is a stock 28BYJ-48 on a ULN2003 board. Since 2026-11-26 that board lives in the
camera module and is driven by the ESP32 (see "Focus motor in the camera module"); ULN group B on the
Robin is kept as the fallback.

Motor connectors XH2.54-4P (Ender-3 order ⚠).

## ULN2003 (28BYJ-48, unipolar) on EXP1 and EXP2

EXP1 (1–10): 1 PC1 · 2 PC3 · 3 PA4 · 4 PA5 · 5 PA6 · 6 PA7 · 7 PC4 · 8 PC5 · 9 GND · 10 5 V.
Pins 1–5 are **not free**: they are the same MCU pins (PC1, PC3, PA4, PA5, PA6) that the onboard
screen uses through the "Ender-3 EXP1" header (see below), so they are taken by the screen. Only
pins 6–8 (PA7, PC4, PC5) are available here, as ULN group B; 9/10 are GND and 5 V.

| Group | IN1 | IN2 | IN3 | IN4 | Power |
|---|---|---|---|---|---|
| A (unused) | PB13 (EXP2 2) | PB14 (EXP2 1) | PB15 (EXP2 6) | PA15 (EXP2) | ULN 5 V + GND from EXP1 9/10 |
| B (focus, fallback) | PA7 (6) | PC4 (7) | PC5 (8) | PC10 (EXP2 7) | same |

The 5 V rail on EXP1 is the board's buck output (MP1584, 3 A); two 28BYJ-48 draw ~240 mA each
at 5 V. 3.3 V logic drives the ULN2003 inputs fine. **PA15 and PB3/PB4 are JTAG pins at reset:
firmware disables JTAG and keeps SWD.**

## Focus motor in the camera module (decision 2026-11-26, power amended 2026-11-27)

The focus 28BYJ-48 (stock, unipolar, five-wire, 5 V) plugs into the ULN2003 board **inside the camera
module** (`hardware/camera-cap/`), and the ESP32-S3 CAM drives that board with 4 GPIOs:

- Motor plug: a JST-XH 5-pin side-entry header in the module's end wall (cradle and window in the model);
  its five pins are wired to the ULN board's motor socket with a short pigtail.
- ESP32 pins: IN1..IN4 on four free GPIOs, chosen per board in the bridge slice (the camera, the SD slot
  and the bridge UART take many; never the board's own USB pins). ULN 5 V and GND come from the module's
  own 5.1 V rail (the Mini-360 buck converter, below), not from the Robin.
- Power budget: ESP32 with camera and WiFi about 0.5 A + 28BYJ-48 about 0.24 A = about 0.75 A at 5.1 V
  (3.8 W), which is about 0.3 A at 15 V in the cable (see the bridge section).
- Firmware: the Robin's focus code stays as a fallback. The ESP32 will serve the LX200 focus commands
  (`:F...`) and the LCD Focus axis (the Robin queues LCD focus moves and the ESP32 picks them up in its
  poll); the example config switches the Robin's focus off then. None of this is implemented yet
  (bridge slice, BACKLOG).

## Focus motor fallback: stock 28BYJ-48 on a ULN2003 board on the Robin (ULN group B)

Still built and tested on the Robin side. The unmodified (unipolar, five-wire) 28BYJ-48 plugs into
its ULN2003 board as shipped. Wire the board to ULN group B:

| ULN2003 board | Robin E3 | Header pin |
|---|---|---|
| IN1 | PA7 | EXP1 6 |
| IN2 | PC4 | EXP1 7 |
| IN3 | PC5 | EXP1 8 |
| IN4 | PC10 | EXP2 7 |
| + (5-12 V) | 5 V | EXP1 10 |
| - | GND | EXP1 9 |

`FOCUS_DRIVER_TYPE DRIVER_TYPE_ULN2003`: half steps (AccelStepper `HALF4WIRE`, coils driven in the
order IN1, IN3, IN2, IN4), 4096 half steps per output turn, 800 half steps/s, 1000/s^2 (the motor at
5 V stalls near 1000/s). All four pins go low when the focuser is idle (`FOCUSER_ALWAYS_ON 0`). There
is no UART, no current setting and no fault detection: a focus move is never refused. If the motor
only hums, check the IN1..IN4 order first; if it turns the wrong way, swap IN1 with IN4 and IN2 with
IN3.

## ALT motor: bipolar-modded 28BYJ-48 on the E socket (TMC2209, UART)

Same motor mod, coil pairs (find them by continuity), current formula and safety rules as the AZ
motor below, on the E socket (PB4 step, PB3 dir, PB5 EN, PC11 UART, address 0). Peak 150 mA = about
91 mA RMS (`ALT_MOTOR_CURRENT_RATING` is a peak value, RMS = rating x `ALT_OPERATING_CURRENT_SETTING` % / 1.414),
hold 30 % (`ALT_MOTOR_HOLD_SETTING`), released when idle (`ALT_ALWAYS_ON 0`), coil resistance
`ALT_COIL_RESISTANCE_OHM` (default 50 ohm). EN stays high until the UART current is set. If the ALT
UART does not answer at boot, every ALT move is refused (no motion, no position change).
`UART_CONNECTION_TEST_TXRX 1` is mandatory (`#error` otherwise). 2048 steps per turn, 16 microsteps,
400 full steps/s and 200 full steps/s^2; steps per degree from upstream's OAE ALT formula (130 mm
radius, 1.25 mm rod pitch). Same quiet TMC settings as AZ.

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

**Current.** A current-chopping driver sets the coil current itself, so the 5 V motor on 12 to
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
- Quiet settings like RA and DEC: stealthChop, PWM at 47 kHz (`TMC_PWM_FREQ`), TOFF 3 with TBL 2
  (`TMC_TOFF`), current ramp-down `TMC_IHOLDDELAY` / `TMC_TPOWERDOWN`, stealthChop at all
  speeds (no `TMC_TPWMTHRS`).

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
| Z+ / probe | PB1 | spare (ALT home or RA hard limit; `ALT_DIAG_PIN`) |

Wire switch between signal and GND. Treat inputs as 3.3 V.

Pins are defined as upstream `*_DIAG_PIN`s and are not read yet. Today `b_setup` only sets them
`INPUT` (no internal pull-up), so a switch needs an external pull-up until the homing slice sets
INPUT_PULLUP. PA11/PA12 are also the MCU's USB pins: unused, USB goes via the CH340.

## ESP32-S3 CAM bridge (USART2, USB-C serial cable)

USART2 (`Serial2`) at 115200 baud, 3.3 V logic both sides, no level shifter. The ESP32-S3 CAM sits
on the telescope and is linked to the Robin by a **standard USB-C (USB 2.0) cable** between two
USB-C breakout boards that join both contact rows (the plug works either way round). The cable
carries serial and **15 V** (power for the camera module), **not USB**: see the safety note in the
[README](../README.md).

| USB-C line | Robin E3 end | ESP32-S3 CAM end |
|---|---|---|
| VBUS | **15 V** from the fused PD-trigger output (after the 3 A fuse), not the Robin's EXP1 pin 10 | Mini-360 IN+ (never the ESP32's 5V pin) |
| GND | GND (the Robin's GND, EXP1 pin 9; common with the 15 V return) | Mini-360 IN-, which is the module's ground |
| D+ | PA2 (USART2 TX), NeoPixel header, through 220 Ω | RX: a free GPIO |
| D- | PA3 (USART2 RX), J24 BLTouch servo (GND, VCC, PA3 ⚠ order), through 220 Ω | TX: a free GPIO |

- One 220 Ω series resistor on each data line, at the Robin end.
- ESP32 end: two free GPIOs, never the board's own USB-C port. They are chosen per board in the
  bridge slice: on Freenove-style ESP32-S3 CAM boards GPIO 17 and 18 are camera data lines.
- Power: the Robin no longer powers the module. A **Mini-360 buck converter** (MP2307 type, about 17 x 11 x 4 mm,
  input up to about 23 V, set to **5.1 V**) sits inside the camera module (`hardware/camera-cap/`). It is
  fed with the system's 15 V (USB-PD trigger output, after the 3 A fuse) over the cable's VBUS and makes 5.1 V
  for the ESP32-S3 CAM (about 0.5 A) and the ULN2003 + 28BYJ-48 (about 0.24 A): about 0.75 A at 5 V, about 0.3 A at
  15 V in the cable (3.8 W, about 88 % efficient, about 0.5 W heat in the box, behind the wall vents).
- Camera end: cable VBUS -> Mini-360 IN+; Mini-360 OUT+ (5.1 V) -> ESP32 5V pin and ULN2003 +; grounds
  join at the Mini-360 (IN- and OUT- are one node).
- Filter at the output: **100 uF electrolytic (16 V or more, 8 x 12 mm) plus 10 uF ceramic** across OUT+ / OUT-,
  soldered at the Mini-360's output pads.
- **Set 5.1 V before connecting anything**: the Mini-360's factory output can be anywhere up to the input
  voltage and would destroy the ESP32. On the bench, power the bare board from the 15 V (nothing on its output
  but the meter), turn the trim pot until a meter reads 5.10 V (5.0 to 5.2 V is fine), then **seal the pot**
  (nail varnish or a drop of hot glue). The module has no access hole to the pot.
- One ground path: the module's ground reaches the Robin only through the cable's GND. Do not tie the
  module's ground to the telescope, the mount or a second cable; with the ESP32's own USB-C port unplugged there is
  no other path.
- Use **22 AWG** for the 15 V tap (supply to the cable's Robin-end breakout) and keep it short (it is
  already behind the 3 A fuse). The Mini-360's input limit is about 23 V: use it only with the 15 V supply
  setting, never tap a 20 or 24 V supply.
- The CC1/CC2 pins carry nothing in this link (the breakouts only join the contact rows), so a breakout's own CC resistors do not matter for the serial link. They can make a USB-C charger switch VBUS on if the cable is plugged into one by mistake, which is why the README says never to.

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

## Supply sense (J8, bed thermistor, PA1)

The board has no input-voltage sense, so a plug on the unused bed-thermistor socket **J8 (TB)** carries a
divider (`SUPPLY_SENSE 1`, Settings > Supply **Auto**):

| Part | From | To |
|---|---|---|
| R_top 100 kΩ 1 % | supply + **after the 3 A fuse** | J8 pin 1 (PA1 side) |
| R_bot 10 kΩ 1 % | J8 pin 1 | J8 pin 2 (GND) |

On the board: R20 4.7 kΩ pull-up to 3.3 V on the node, R21 100 Ω node → PA1, C20 to GND. The node reads
about 2.55 V at 12 V, 2.64 V at 15 V, 2.80 V at 20 V, 2.92 V at 24 V and 3.10 V at 30 V (still below 3.3 V). Without
the divider the pull-up holds PA1 at 3.3 V, read as "not fitted" (`Auto --`).

- Use 1 % resistors and take the supply after the fuse. Fit **R_top at the supply end** (at the fuse tap), so the
  lead to J8 carries only a 100 kΩ-limited signal: a bare supply lead touching J8 pin 1 would push 12-24 V through
  R21 (100 Ω) into PA1. Polarity: supply + to R_top only; J8 pin 2 is the board's ground, never the supply. A
  plug fitted the wrong way round is harmless: it reads invalid (`Auto --`).
- Measure R_top and R_bot with a meter before fitting and check R20's value and tolerance on your board (cheap
  boards may fit 5 % parts): the divider is sensitive (1 % on R_bot is about 0.4 V), so use the calibration below.
- The firmware averages 16 reads every 500 ms, assumes the board's 3.3 V rail (no VREFINT) and only applies a
  reading inside a window around 12 V (10.5–14.2 V), 15 V (14.2–16.5 V), 20 V (18.5–21.5 V) or 24 V
  (22.5–25.5 V), once it has held for 2 s and never during a move. Readings in the gaps or out of range show
  but change nothing. Moving from the applied choice to the other side of the 14.2 V edge needs 0.3 V more
  (14.5 V up, 13.9 V down), so a 12 V battery on charge (13.2–14.4 V) stays on 12 V.
- Calibrate once, at about 20 V (the wall adapter, not the power bank: the error is smallest around there): with
  `SUPPLY_SENSE_CAL 1.0` read the voltage the row shows, measure the supply at the J8 plug's source with a
  meter, then set `SUPPLY_SENSE_CAL` = meter volts / shown volts (a new value = old x meter / shown if it was not 1.0)
  and rebuild and flash; it is a compile-time constant. Without a meter the windows still hold with 1 %
  resistors, but check the row against the supply you plug in.

## Other

- USB (mini-B) → CH340 → USART1 PA9/PA10 (`Serial`, 19200): LX200 for ASCOM / Stellarium desktop / debug.
- AT24C32 EEPROM on I²C PB6 SCL / PB7 SDA ⚠ (verify by I²C scan).
- Power input 12–24 V; set Settings > Supply (Auto with the J8 divider, or 12, 15, 20 or 24 V) to match. Unused: heaters PC8/PC9, fan PA8, thermistor PA0 (PA1 = supply sense), runout PB10,
  SD on SPI2 (EXP2).
