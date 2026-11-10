# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.2.1] - 2026-11-10

### Changed

- RA and DEC TMC2209 are quiet at standstill and while tracking on the Robin E3: stealthChop (spreadCycle only above `TMC_TPWMTHRS`), PWM at 47 kHz (`TMC_PWM_FREQ` 2), hold current 50 % of run (`RA_HOLD_PERCENT`, `DEC_HOLD_PERCENT`) after 0.44 s of standstill, and TOFF 3 with TBL 2 instead of TOFF 1.
- LCD beeps are about a third as long and quieter: tone moved off the buzzer resonance to 6 kHz (`BEEP_TONE_HZ`) and all durations scaled by `BEEP_SCALE_PERCENT` (default 35: tick 7 ms, error 2x21 ms, long 140 ms).

### Fixed

- Inverted LCD text (selected rows, alert bars) drew as solid blocks; glyphs are now transparent over the XORed box.
- The LCD redraws within about 30 ms of knob input, sends only the changed rows, and drives its pins with direct register writes (`LCD_SPI_HALF_PERIOD_NS`, optional `LCD_SHOW_FRAME_MS` timing readout).
- Knob direction inverted by default for the stock Ender 3 LCD on the Robin E3 (`ENCODER_INVERT 1`).
- TMC2209 UART reads never got a reply on the Robin E3, so every driver failed its connection test. With the board's 20 kΩ pull-down on PDN_UART, `SoftwareSerial::listen()` before the request glitched the line low ahead of the sync byte and the driver dropped the request. Now the line stays driven high for the whole request and the code listens only after it has gone out (`src/TmcHalfDuplex.cpp`).

## [v0.2.0] - 2026-11-10

### Added

- LCD safety behaviour: STOP from the screen stops all motion including tracking and leaves the mount idle (no tracker-off catch-up move); a driver fault locks GoTo, Jog, Sync and Home/Park moves (only "Stop tracking" stays live) and names the failing axis (RA, DEC or DRIVER); GO re-checks min altitude, Sun distance and input at the click; Sync confirms are armed after 600 ms like GO; Unpark needs date and time; UI "Go home" shows HOMING and ends idle. A failed AZ driver refuses every AZ move without changing its position, and AZ on TMC2209 UART requires `UART_CONNECTION_TEST_TXRX`.

- AZ axis on the Robin E3 Z socket for a bipolar-modded 5 V 28BYJ-48 on a TMC2209 in UART mode (peak 150 mA, 91 mA RMS actual, 2048 SPR at 16 microsteps, hold 30 %, released when idle): `AZ_*` pins, `AZ_COIL_RESISTANCE_OHM` for the 12/24 V PWM_OFS, and AZ stays disabled when its UART does not answer (`enableAzMotor()` no longer re-enables it on VREF current); ULN2003 stays selectable.
- LCD screens: GoTo (RA/Dec editors prefilled with the last target, Alt/Az/Sun confirm armed after 600 ms, refused below min altitude, near the Sun or on invalid input), Sync and Sync on target (offset, BIG JUMP frame above 5 deg, SYNCED), Setup (date then time fast path, hh:mm at :00, UTC offset, latitude/longitude with N/S E/W letters, "Clears Sync. Sure?" before date/time/UTC/longitude changes after a Sync), Jog (submenu, 300 ms auto-stop, stale turns dropped, SLOW/CENTER/FIND/MAX rates), Home/Park (Unpark only when parked, Set home confirm, PARKING/HOMING with STOP), Settings from a row table (language, min altitude, beep, 12/24 V supply), Turkish throughout and a width lint over every screen in both languages; host-tested.
- Onboard LCD and knob on the Robin E3 (`USE_LCD_UI`): U8g2 ST7920 canvas, EXTI encoder with click, long press and queued buzzer, `UiMountAdapter` mapping UI actions to Mount calls, non-blocking `Mount::startPark()`, `isDateSet()`/`isTimeSet()`, boot UART-test fault code, `SUPPLY_VOLTAGE` 12/24 V (`Mount::applySupplyVoltage`: 12 V speed factor and TMC2209 PWM_OFS), `TIMER_TONE=TIM4`.
- `core::UiController` for the 128x64 LCD: status screen with alert bar and transient headers, main menu with a fixed row-0 motion slot, locked rows until date and time are set, STOP on any click or long press while the mount moves, 500 ms click ignore after the motion slot changes, fault and reboot pages, screen saver; host-tested.
- `core::Canvas` drawing interface with font metrics (128x64 LCD), and `core::FieldEditor` extensions: back (previous group), hh:mm kind, hemisphere letters for latitude/longitude. The DWIN frame encoder is removed.
- EN/TR UI text table `core::uiText` (ASCII, no diacritics) and `core::FieldEditor` for time, RA, Dec, latitude, longitude, date and integer fields with range clamping, host-tested.
- `core::SkyMath`: alt/az conversion, low-precision Sun position, angular separation and `checkTarget` (refuses targets below the minimum altitude or within 15 deg of the Sun), host-tested.

### Fixed

- docs/wiring.md: Ender-3 EXP1 header pin map for the screen cable (encoder on pins 3 and 5), EXP1 pins 1 and 2 swap, EXP2 positions of PB13/PB14/PB15, EXTI lines 0 and 11 reserved by the knob; architecture diagram and bench checklist (time `canvas.flush()`) updated.

## [v0.1.1] - 2026-11-10

### Fixed

- `robin_e3` USB serial (`Serial`) now runs on USART1 PA9/PA10 as intended; it had been built on USART2
  (PA2/PA3), shared with the bridge `Serial2`, so the USB port never came up. Receive verified on the
  bench; transmit to the PC still shows nothing on our board (board-level PA9 → CH340 path, see BACKLOG).

## [v0.1.0] - 2026-11-10

### Added

- Project scaffold: README (unofficial notice), CLAUDE.md, BACKLOG.md, architecture and wiring
  docs, plans folder.
- OpenAstroTracker-Firmware v1.14.0 imported into `firmware/` (upstream commit `6a40302`).
- Local toolchain and native test gate: `tools/gate.sh` (puts the bundled MinGW on PATH, runs
  `pio test -e native`, then `pio run -e robin_e3` once that env exists); PlatformIO core dir
  pinned to `.pio-core`. Native baseline: 342/342 tests pass on GCC 16.2, no warning
  suppressions needed.
- MKS Robin E3 board `robin_e3` (STM32F103RC): `ststm32@20.0.0` (STM32duino core 4.30000.0, arduino-stm32 3.0.0), board id 3001, pins file and committed example config, stepper ISR on TIM6 at 5 kHz (`static_assert` on the period), TMC2209 single-wire UART pins for SoftwareSerial on TIM7, JTAG off / SWD on at the top of `setup()`, serial polled each loop. Builds clean: RAM 5220 B (10.6 % of 48 KB), flash 97304 B (37.1 % of 256 KB) at `-O2`.
- TMC2209 single-wire UART verified on `robin_e3`: SoftwareSerial (STM32duino core 4.30000.0) treats RX pin == TX pin as half-duplex, and RA/DEC drivers are configured before the stepper timer starts. The core gives TIM6 and TIM7 the same NVIC priority (14, no sub-priority), so the stepper timer is now set to priority 15 and cannot delay SoftwareSerial bit timing.
- Persistent store on `robin_e3`: header-only `core::I2cEeprom` (AT24C32, big-endian address, write-on-change, 10 ms ack-poll, bounded probe) behind `EEPROMStore`, with STM32duino flash emulation as fallback when the chip does not answer at boot (I2C PB7 SDA / PB6 SCL). Host-tested with a fake bus.
- LX200 on both serial ports of `robin_e3`: USB `Serial` and the bridge `Serial2` (USART2, 115200) are polled every loop, each with its own command buffer (`SerialBuffer`), so partial commands never mix. An over-long command (more than 31 characters before `#`; the longest real command, `:SY`, is 21) is now dropped up to its `#` on every board instead of its tail being run as a command; the bridge port never carries debug output.
- `robin_e3` example config enables `UART_CONNECTION_TEST_TXRX`: an RA/DEC driver whose TMC UART does not answer is disabled (EN high) instead of running on the board's VREF current. Unused Z/E driver EN pins are driven HIGH at boot, and the I2C pins come from `I2C_SDA_PIN`/`I2C_SCL_PIN`.

### Security

- LX200 `:SC` rejects month outside 1..12 and day outside 1..31 (reply `0`); the date-to-JD helper returns 0 for a month outside 1..12 instead of indexing out of bounds.
- LX200 set commands (`:Sr`, `:Sd`, `:SY`, `:St`, `:SL`, `:SH`, `:SHL`) reject hours >= 24, degrees > 90 (DEC, latitude) and minutes/seconds > 59 (reply `0`).
- GPS acquire command timeout is parsed with `strtol`; negative or unparsable values use the 2 min default and anything longer is clamped to 120000 ms.
- Serial polling is bounded to 64 bytes per port per loop, and an over-long LX200 command is ignored in full (including any ACK inside it) until its `#`.
- Settings store: a failed AT24C32 read latches a bus-failure state that blocks writes until reboot, so a transient I²C error can no longer wipe stored settings; flash fallback writes only changed bytes. A failed AT24C32 write latches the same state (a half-written marker is never rewritten), and once latched reads return immediately instead of timing out per byte.
