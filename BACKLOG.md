# Backlog

Ideas, plans, discussion topics and bugs. Birkan adds items at any time; each development
cycle picks a set of them. Done items move to the cycle's `CHANGELOG.md` entry and leave this
file. Slices and their order: `docs/architecture.md` §8.

## Next

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

- Slice 3 `runtime-config` (settings struct, `:XC` commands, ULN2003 driver type).
- Persist screen settings (slice 3 settings table): language, minimum altitude, beep, supply.
  RAM-only until then.
- Slice 4 `homing` (microswitch homing, startup sequence).
- Slice 5 `esp32-bridge` (BLE + hotspot HTTPS/WSS). Pair with BLE bonding/whitelist: the LX200 parser is a trust boundary.
- Slice 6 `pwa`, slice 7 `planes`, slice 8 `stellarium`.
- Small built-in object list on the screen (Messier + bright stars in flash) for GoTo without
  a phone.
- Focuser 28BYJ-48 + ULN2003 (hardware arriving later).
- Optional ALT motor on a free TMC2209 socket.

## Plans

## Bugs

## Debate

- USB transmit dead on our board (2026-11-10): USART1 configured and receiving (PC → board works), but
  nothing reaches COM6, even bytes written straight to USART1 DR over SWD; PA9 idles high as AF push-pull.
  Suspect the PA9 → CH340C RXD (pin 3) trace/resistor. Skipped for now: bench LX200 runs over USART2 via the
  ESP32-S3. Continuity check later (PSU and USB off).
- Other boards (ramps, esp32, rp2040) are not compiled locally after the STM32 changes to shared files
  (`f_serial.hpp`, `MeadeParserSet.cpp`, `SiderealClock.cpp`, `Configuration_adv.hpp`); only native tests and
  `robin_e3` run in the gate. Run them once those toolchains are installed, or rely on upstream CI.
- iPhone hotspot with no cellular signal in the field: does it stay up? Test before slice 4.
