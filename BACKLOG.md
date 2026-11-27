# Backlog

Ideas, plans, discussion topics and bugs. Birkan adds items at any time; each development
cycle picks a set of them. Done items move to the cycle's `CHANGELOG.md` entry and leave this
file. Slices and their order: `docs/architecture.md` §8.

## Next

- **Camera module fit test.** Print the 12.5 mm ring and the module base (PETG), glue the balls and magnets (polarity rule in `hardware/camera-cap/README.md`), check the module sits on the ring without rocking, the three magnets hold it upside down, the ESP32 / ULN / USB-C boards and the motor plug fit their seats, then measure the real eye relief per eyepiece and edit the table.
- **v0.5.0 bench test.** Power-cycle: language, min altitude, beep, supply and Own WiFi come back; blank store / `:XFR` loads defaults with no write-back; `:XNM1,2,-60#`, `:XNI…#`, `:XNNMy iPhone#`, `:XNG#` over USART2 with a USB-UART adapter (page shows Hotspot, No bridge after 5 s); the USB-C serial cable (continuity both plug orientations, 220 ohm, ESP32 5 V current).
- **v0.4.0 bench test.** ALT on the E socket answers at boot (no DRIVER FAULT) and moves in Adjust; AZ and ALT quiet while moving (stealthChop at all speeds); Supply row 12/15/20/24 V. Before wiring the focus ULN2003: meter PC10 (EXP2 7), it may be the SD-card detect line on the board; then check coil order and direction, and that the coils go cold when idle.
- **Slice 2 bench test (needs the LCD on the Ender-3 header).** Stock 12864 LCD on "Ender-3 EXP1"
  (docs/wiring.md), PSU off while plugging in, then:
  1. Boot: status screen appears. If blank or garbled, check the cable key and try a slower U8g2
     SPI bus delay (the ST7920 at 3.3 V logic is the known risk).
  2. Knob: one detent = one step in the right direction (else `ENCODER_INVERT` or counts per detent).
  3. Buzzer beeps on click.
  4. Fast path: click the alert, set date and time, GoTo unlocks.
  5. Jog RA and DEC; let go and it stops within ~0.3 s; click while moving anywhere = STOP.
  6. GoTo a star, confirm, ARRIVED then TRACKING; Sync on target.
  7. Park, PARKING screen, click, STOP works mid-park.
- **Slice 1 bench test (needs PSU).** Board on a 12/24 V PSU, then:
  - Re-probe over SWD with the PSU on: target voltage ≈ 3.3 V; dump flash again, compare SHA-256
    with `backup/robin_e3_factory_full.bin`.
  - Flash: `cd firmware && ../.venv/Scripts/pio.exe run -e robin_e3 -t upload`. Then re-attach the ST-Link
    over SWD after the first boot (JTAG is remapped off at the top of `setup()`; SWD must still work;
    if not, recover with connect-under-reset).
  - Measure RA/DEC/Z/E EN pins and motor current at boot, before and after driver config. Z and E EN
    must read HIGH (disabled). With a TMC UART wire unplugged, that axis must stay disabled
    (`UART_CONNECTION_TEST_TXRX 1` in the example config), not run on VREF current.
  - **Release build** (the default `robin_e3`; no debug output, LX200 replies on USB):
    - USB serial at 19200 answers `:GVP#` and `:GVN#`; `:SY` sync (21 chars) gets a `1`/`0` reply.
      Blocked while USB transmit is dead (see Debate); the USART2 check below covers it meanwhile.
    - Manual slew moves RA and DEC both ways; `:Q#` stops. Watch for missed or garbled driver
      writes when a slew starts with Serial2 traffic flowing.
    - `:XSR…#` then power-cycle; `:XGR#` returns the new value.
    - Same `:GVP#` over USART2 (PA2/PA3) with a USB-UART adapter at 115200.
  - **Debug build** for store and UART evidence: copy the example config to
    `firmware/Configuration_local_robin_e3.hpp` and add
    `#define DEBUG_LEVEL (DEBUG_INFO|DEBUG_EEPROM|DEBUG_STEPPERS)` (default is `DEBUG_NONE`,
    Configuration_adv.hpp), then build/upload `robin_e3`. The log goes to USB Serial (19200);
    LX200 replies are muted on USB in this build, so run any LX200 check over Serial2
    (USB-UART adapter, 115200).
    - Boot log names the active store: `[EEPROM]: STM32: Using AT24C32 (I2C)` or `flash emulation`.
    - Boot log has `[STEPPERS]: UART connection to RA driver successful.` and the same for DEC
      (Mount.cpp:451). `:XGMS#` is not evidence: it returns compile-time strings. Cross-check that
      the motor holds noticeably softer than at VREF current.
  - Z socket pins (PC14/PC15/PB8) on the real `_0003` board.

## Later

- Slice 3 `runtime-config` (settings struct, `:XC` commands).
- Slice 4 `homing` (microswitch homing, startup sequence).
- Bridge slice, focus in the camera module (decision 2026-11-26, `docs/wiring.md`): the ESP32 drives the
  module's ULN2003 (4 GPIOs, pins chosen per board) and serves the LX200 focus commands (`:F...`); LCD
  focus moves go through a queue on the Robin that the ESP32 picks up in its poll (new `:XN` command). The
  example config then switches the Robin's focus (ULN group B) off; the Robin focus code stays as a fallback.
- Telescope: National Geographic (Bresser) 76/700 EQ Newtonian, f/9.2, 1.25" focuser; eyepieces 25,
  12.5, 8 mm; 3x Barlow. Camera plan: OV5640 afocal behind the 12.5 mm (exit pupil 1.4 mm < ~1.5 mm
  module lens opening: full light, ~f/2.6 effective, ~1 deg field in a ~0.9 deg circle, 1.4"/px) or 8 mm
  (f/4.1, 0.57 deg circle); 25 mm wastes ~70 %. 12.5 mm + 3x Barlow for Moon/planets. Needs a 3D-printed
  clamp on the 1.25" eyepiece barrel holding the board centred at eye relief, lens at infinity. Focus
  presets "Eyepiece"/"Camera" on the focus motor.
- ALT auto-levelling with an MPU-6050 on the polar axis (I2C, shares PB6/PB7 with the AT24C32;
  upstream OAT already supports the MPU-6050 and stores its pitch/roll offsets). The phone sends
  latitude; the firmware moves ALT until the measured polar-axis tilt equals it. Needs a one-time
  calibration (flip test or the phone's level app), stored on the board. Expected 0.1-0.3 deg after
  calibration, drifting with temperature: coarse setting only; final polar alignment from the stars
  (later: camera plate solving). Upgrade path if not enough: Murata SCL3300 inclinometer (SPI, new
  driver). Azimuth stays manual (magnetometers are useless near steel and motors).
- Slice 5 `esp32-bridge` (ESP32-S3 CAM: phone hotspot STA or own access point, HTTPS/WSS, `:XN` status polling; Robin side and protocol done). The LX200 parser is a trust boundary.
- Slice 6 `pwa`, slice 7 `planes`, slice 8 `stellarium`.
- Small built-in object list on the screen (Messier + bright stars in flash) for GoTo without
  a phone.

## Plans

## Bugs

- Step move during a GoTo started from idle makes the GoTo end idle (accepted in point-adjust).

- `robin_e3` core runs at 64 MHz (measured over SWD) while `F_CPU` says 72 MHz, likely HSI instead of the 8 MHz HSE crystal. LCD DWT delays use `F_CPU` (12.5 % long, harmless); switch them to `SystemCoreClock` and consider running the PLL from HSE at 72 MHz.

## Debate

- Slew torque with `TMC_TPWMTHRS` 1500 (spreadCycle above ~31 full steps/s) not checked on the bench.

- USB transmit dead on our board (2026-11-10): USART1 configured and receiving (PC → board works), but
  nothing reaches COM6, even bytes written straight to USART1 DR over SWD; PA9 idles high as AF push-pull.
  Suspect the PA9 → CH340C RXD (pin 3) trace/resistor. Skipped for now: bench LX200 runs over USART2 via the
  ESP32-S3. Continuity check later (PSU and USB off).
- Other boards (ramps, esp32, rp2040) are not compiled locally after the STM32 changes to shared files
  (`f_serial.hpp`, `MeadeParserSet.cpp`, `SiderealClock.cpp`, `Configuration_adv.hpp`); only native tests and
  `robin_e3` run in the gate. Run them once those toolchains are installed, or rely on upstream CI.
- Camera (2026-11-25): wanted as a live viewer and for long exposures, placed at the eyepiece.
  ESP32 OV-series sensors (OV2640/OV5640) are webcam parts: small pixels, noisy, IR-cut filter, JPEG,
  short maximum exposure (exact limit unchecked, depends on the sensor driver), so live view of
  Moon/planets only. Afocal (over the eyepiece) needs a lens focused at infinity and is fiddly;
  prime focus (bare sensor with a 1.25" nosepiece, focus motor does the focusing) is better.
  Options: (a) ESP32-S3 camera board as viewer only, mounted at the camera with a short ribbon and a
  4-wire UART (TX, RX, GND, 5 V) to the Robin; (b) Raspberry Pi Zero 2 W + Pi HQ (IMX477) or
  IMX462 at prime focus for real RAW long exposures, serving images to the PWA over WiFi, with the
  ESP32-S3 kept as the LX200 bridge (~2 W more). Video needs WiFi on Android too (BLE is too slow).
  Decide before the bridge/PWA slices.
  2026-11-25: goal is faint objects; user rules out a Raspberry Pi (software upkeep), ESP32 only.
  Images go to the camera board's own SD card, never through the Robin (UART 11-200 KB/s, 48 KB RAM,
  step timer). XIAO ESP32S3 Sense not sold in Turkey: use any ESP32-S3 camera board with 8 MB octal
  PSRAM (N8R8/N16R8), a 24-pin DVP camera socket, a microSD slot and USB-C (e.g. Freenove
  ESP32-S3-WROOM CAM or a generic "ESP32-S3 CAM N16R8"). Next: camera spike on that board + OV5640 (fixed focus): measure the longest
  exposure the esp32-camera driver allows (low XCLK/PCLK, long VTS), RAW noise, and live-stack ~100
  frames on a bright DSO (M42, M13). Also try a UVC low-light camera (IMX291/IMX462) on the S3's USB
  host (full-speed only, may not enumerate). Decide the camera from the measurements.
  Camera features to plan (2026-11-25): three modes switched from the app. (1) Live view: JPEG stream,
  ~15-30 fps at 640x480, slower at higher resolution; at night frame rate = 1/exposure. (2) Focus aid:
  full-resolution crop around a star with a star-width (HFD/FWHM) number, driving the Focus motor
  from the app. (3) Capture: RAW (driver `PIXFORMAT_RAW`, 8-bit over the DVP bus; confirm the Bayer
  order) to the camera's SD card, independent of the phone; the phone live-stacks binned/cropped frames
  (WebGL/WebGPU, star alignment) and fetches missed frames when it wakes (PWAs and iOS native apps are
  suspended in the background). Full RAWs stacked later in Siril. Optional: running stack on the
  camera in PSRAM. Spike also measures: WiFi throughput (~1-2 MB/s expected) and live-view rate over
  HTTPS/WSS with the project CA (an HTTPS page cannot load a plain-HTTP stream).
  Network: everything joins the phone's hotspot. ESP32 is 2.4 GHz only (iPhone: "Maximize
  Compatibility" on); iPhone hotspot switches off with no clients, so bridge and camera auto-reconnect;
  Android Chrome resolves .local poorly, so discovery needs a fallback. Web Bluetooth is dropped (both
  phones over WiFi, one ESP32-S3 CAM does camera and bridge: CLAUDE.md Decisions). Stellarium (mobile or desktop on
  the same hotspot) talks LX200 over TCP to the bridge; the bridge must serialise commands from several
  clients (app + Stellarium), one command in flight.
