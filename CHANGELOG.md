# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.7.1] - 2026-11-28

### Changed

- Time editor edits seconds too (hh:mm:ss): set the seconds to match your watch and click to save; the old "click at :00" footer is gone. Untouched seconds keep the value from when the editor opened.
- Setup's Time row and the screensaver clock show hh:mm:ss.

## [v0.7.0] - 2026-11-28

### Added

- Adjust options: **Set home all** sets home on every axis at once: RA and DEC as Set home here, AZ and ALT zeroed and stored like upstream, Focus zeroed. Same idle-only lock and confirm screen.

### Changed

- Adjust page: step size is kept per axis, with lists that suit each motor (RA and DEC 10, 100, 1 000, 10 000, 50 000 steps; AZ and ALT 100, 1 000, 10 000, 60 000, 300 000; Focus 1 to 10 000 as before). The JOG SLOW / CENTER / FIND / MAX options are gone from the onboard screen; LX200 jogging is unchanged.
- Adjust options end with a Main menu row; long press on the options returns to Adjust.
- ALT runs at 200 full steps/s and 100 full steps/s² (was 400 and 200): 6 400 microsteps/s was above the 5 kHz stepper interrupt.

### Fixed

- Long press on the Adjust options went to the main menu instead of back to Adjust.
- Stopping a move on the Adjust page jumped to the status screen; it now stays on Adjust, and knob turns are ignored while the stopped axis slows down.

## [v0.6.2] - 2026-11-28

### Fixed

- DEC moved only about 8 degrees of its 135 degree limits on the Robin E3: `DEC_TRANSMISSION` came out as 1/16 because `Configuration.hpp` sets `DEC_PULLEY_TEETH 16` before the board file's OAE default of 1, giving 125 instead of 2000 DEC steps per degree. The Robin E3 board file now sets `DEC_TRANSMISSION 1.0f` (OAE DEC is a geared motor, no belt). AZ (64 054 steps/deg) and ALT (59 479 steps/deg) were checked on the board and are correct.

## [v0.6.1] - 2026-11-27

### Changed

- Supply-sense divider bottom resistor 10 kΩ instead of 4.7 kΩ (`SUPPLY_SENSE_RBOT 10000`): about 38 ADC counts per volt instead of 28, and only two common parts; 30 V still reads below 3.3 V at PA1.

## [v0.6.0] - 2026-11-27

### Added

- Supply voltage auto-detect (`SUPPLY_SENSE 1`, on in the example config): a 100k / 4.7k divider plug on the unused bed-thermistor socket J8 (PA1, `docs/wiring.md`) is read every 500 ms (16 samples, smoothed, Vdd assumed 3.3 V). Settings > Supply gains **Auto** (the default with the sense on and a blank store; upgraded boards keep their manual voltage): it applies 12 / 15 / 20 / 24 V once a reading inside a window (10.5-14.2 / 14.2-16.5 / 18.5-21.5 / 22.5-25.5 V) holds for 2 s, with 0.3 V hysteresis at the 12/15 edge, and only while the mount is not moving; gaps, out-of-range or no valid reading keep the last voltage. The row shows the applied choice and the measurement, `Auto 15 15.1` / `Auto 15 --` (TR `Oto`); leaving Auto starts at the applied voltage, manual rows add the measurement (`15 V (15.1)`). `SUPPLY_SENSE_CAL` for a one-time meter calibration.
- `hardware/camera-cap/`: parametric OpenSCAD magnetic camera module (replaces the earlier cap and head) on kinematic eyepiece rings: one ring per eyepiece (25 / 12.5 / 8 mm; thumbscrew sleeve, three radial 90 degree V-grooves for 5 mm steel balls, three magnet pockets, key notch, light lip; its height carries the eye relief) and one module (island with balls, magnets, light-tight labyrinth and OV5640 nose; tray for the ESP32-S3 CAM, ULN2003 board and USB-C breakout; wall windows for USB-C and the motor plug; vents; lid). Parts: `ring`, `module_base`, `module_lid`, `assembly`, `exploded`; README with what to buy, magnet polarity and centring.
- `hardware/camera-arm/`: parametric OpenSCAD swing-away camera arm, a second camera mount next to the cap. Split collar on the Barlow body with a magnet-detented vertical hinge (0 and 90 degrees, printed hard stop at 0), an arm with a vertical dovetail rail (28 mm of Z travel, engraved marks for 25 / 12.5 / 8 mm), a cup with three nylon centring thumbscrews (about +-2 mm) and a light-shield skirt, and an ESP32-S3 CAM plate on the post. Heights and eye relief are estimates until measured.

### Changed

- LCD settings block marker 0xA1 -> 0xA2 (Supply 0 = Auto, 1..4 = 12/15/20/24 V). An older block still loads with every value kept (its supply index moves up by one); it is rewritten in the new layout on the next change. Downgrading to v0.5.0 or older resets the LCD settings (language, minimum altitude, beep, supply, Own WiFi) to their defaults.
- Camera module powered by its own Mini-360 buck converter (5.1 V) fed with 15 V over the USB-C cable's VBUS (the Robin no longer powers it); focus stays a stock 5 V 28BYJ-48 on the ULN2003 in the module. Module now 60 x 55.2 x 36.2 mm, about 51 g. Cable safety note updated ("15 V – NOT USB").
- `hardware/camera-cap/`: compact camera module centred on the optical axis (boards stacked: OV5640, ESP32-S3 CAM, ULN2003 driver; about 60 x 55 x 36 mm and 47 g, centre of mass 2.8 mm off the axis, was 103 x 60 x 27 mm side by side): folded camera ribbon, driver on posts (`driver = "board" | "dip"`), USB-C and the JST-XH motor socket on one end wall (`port_side`); rings unchanged.
- The focus 28BYJ-48 moves from the Robin's ULN2003 (group B) into the camera module: it plugs into a ULN2003 board there, driven by the ESP32 (decision and power budget in docs only: `docs/wiring.md`, `docs/architecture.md`; ESP32 firmware follows in the bridge slice). The Robin's focus code stays as a fallback.

## [v0.5.0] - 2026-11-26

### Added

- LCD Settings > Network page: the ESP32 bridge's mode (Hotspot, Own WiFi, Connecting, Off, or No bridge after 5 s without a status), WiFi name (up to 20 characters), IP address, clients and, in Hotspot mode, the signal; plus the Own WiFi choice (Auto / On / Off), changed with a click. Long press goes back; a click while the mount moves still stops it.
- LX200 `:XN` commands for the bridge (polled about once a second): `:XNM<mode>,<clients>,<rssi>#` status, `:XNI<a.b.c.d>#` address, `:XNN<name>#` WiFi name (`:XNM` and `:XNI` reply `1`, or `0` when malformed; `:XNN` always `1`), and `:XNG#` returning the Own WiFi choice (`0#` Auto, `1#` On, `2#` Off). Every value is checked strictly; the name is cut to 20 printable characters.

### Changed

- The LCD settings (language, minimum altitude, beep, supply voltage, Own WiFi) are kept across power cycles in their own block of the EEPROM store (AT24C32, flash fallback), written only when a value changes. The stored supply voltage is applied at boot; a blank or invalid store keeps the defaults and `SUPPLY_VOLTAGE`.
- Both phones link over WiFi (phone hotspot first, the bridge's own access point as fallback); Bluetooth is dropped. One ESP32-S3 CAM on the telescope does camera and bridge, linked to the Robin by a USB-C cable that carries serial, not USB (docs/wiring.md, README safety section).

## [v0.4.0] - 2026-11-25

### Added

- ALT motor on the Robin E3 E socket (bipolar-modded 28BYJ-48, TMC2209 UART on PC11), with the AZ numbers: 91 mA RMS, hold 30 %, released when idle, 2048 steps x 16 microsteps, steps per degree from upstream's OAE ALT formula. EN stays high until the UART current is set, a failed UART test refuses every ALT move without changing position, and the build stops with `#error` unless `UART_CONNECTION_TEST_TXRX` is 1.
- Focus motor on a ULN2003 board (stock unipolar 28BYJ-48 on ULN group B: PA7, PC4, PC5, PC10, 5 V and GND from EXP1): new `DRIVER_TYPE_ULN2003`, AccelStepper half steps with the 28BYJ-48 coil order, 4096 half steps per turn, 800 half steps/s, 1000/s^2. The coils are released when idle unless `FOCUSER_ALWAYS_ON` is 1. No UART, current setting or fault detection; the build refuses ULN2003 on RA, DEC, AZ and ALT and on other boards.
- LCD Adjust: ALT axis between AZ and Focus (only when fitted), with its position and the step in degrees. A failed ALT driver refuses only ALT moves ("ALT DRIVER FAULT"), ALT step moves count as moving and STOP stops them. LX200 `:MXl<n>#` (upstream's ALT letter) moves ALT by steps, now through the same driver check.

### Changed

- The supply setting (LCD Settings and `SUPPLY_VOLTAGE`) offers 12, 15, 20 and 24 V instead of 12 or 24 V (15 V for USB-PD power banks). Every TMC2209 UART axis gets its stealthChop PWM offset for the real voltage, and the slew speed cap is `min(1, SUPPLY_12V_SPEED_FACTOR * V / 12)`: 70 % at 12 V, 87.5 % at 15 V, full from 20 V.
- The E socket carries ALT instead of the focuser, and AZ has its own pin defines on the Z socket (it no longer borrows ALT's). The focuser moves from the E socket's TMC2209 to a ULN2003 board, and the example config enables AZ, ALT and the ULN2003 focuser.

### Fixed

- AZ TMC2209 gets the same quiet settings as RA and DEC: stealthChop at all speeds, PWM at 47 kHz, TOFF 3 with TBL 2, and hold-current ramp-down.

## [v0.3.0] - 2026-11-10

### Added

- LCD Adjust page: turn moves RA, DEC, AZ or Focus by the chosen step size (1 to 10 000 steps, coalesced per tick) or jogs at SLOW to MAX; click picks the axis, hold opens Step size, Sync on target, Set home here (idle only, confirm) and Go home. Shows the axis position and the step in degrees; a failed AZ/Focus driver refuses the move; click or hold while moving still stops.
- Firmware groundwork for the Adjust page: a move-by-steps UI action (same `Mount::moveStepperBy` as LX200 `:MX`), AZ/Focus/position fields in the UI snapshot, and Set home here that works before date and time are set.
- Focus motor on the Robin E3 E socket (bipolar-modded 28BYJ-48, TMC2209 UART, 91 mA RMS, 16 microsteps). EN stays high until the UART current is set, a failed UART test refuses every focus move without changing position, and the build stops with `#error` unless `UART_CONNECTION_TEST_TXRX` is 1.
- LCD: AZ and Focus moves count as moving (the bar shows STOP and any click or hold stops them), and step moves accumulate instead of restarting.

### Changed

- RA and DEC step moves (`:MX`) resume tracking only if it was on before; from idle they stay idle (upstream started tracking on arrival).
- LX200 park `:hP#` goes home with tracking off and unpark `:hU#` is a no-op on the Robin E3 (there is no parked state).
- LCD menus are Track here, Point, Adjust and Settings. Point merges GoTo, Sync and Sync on target: RA, Dec, then a confirm screen where the knob picks GO or SYNC. Setup (date, time, UTC offset, position) moved under Settings, Jog and Home/Park rows are gone, and a long press goes back one level on every screen (it still stops a moving mount).

### Fixed

- Go home (LCD) and park `:hP#` now end idle at the markers: RA and DEC stop exactly at step 0, tracking stays off and the tracker-off catch-up move (which blocked the main loop and ran RA past the markers) is skipped.
- Adjust moves exactly the step size per detent (the knob's fast-turn x5 now only applies in menus and editors), and a click in the same tick as a step move is STOP instead of cycling the axis.
- Focus TMC2209 uses the same TOFF/TBL, PWM frequency and supply-voltage PWM offset as the other axes, and its EN pin never goes low after a failed UART test, also with `FOCUSER_ALWAYS_ON 1`.

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
