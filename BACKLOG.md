# Backlog

Ideas, plans, discussion topics and bugs. Birkan adds items at any time; each development
cycle picks a set of them. Done items move to the cycle's `CHANGELOG.md` entry and leave this
file. Slices and their order: `docs/architecture.md` §8.

## Next

- **Slice 0 — toolchain + baseline.** Portable MinGW in `.tools/` for `pio test -e native`;
  `core_dir` in `firmware/platformio.ini`; green native test run on untouched v1.14.0.
- **Slice 1 — `stm32-port`.** Board env + pins, HardwareTimer stepper ISR, TMC2209 single-wire
  UART, AT24C32 EEPROM, LX200 on USB + USART2. Needs the board on a 12/24 V PSU.

## Later

- Slice 2 `dwin-ui` (Ender 3 V2 screen: driver from the DWIN T5UIC1 guide, status, setup wizard,
  jog, Track here / GoTo RA/Dec / Sync). Needs the adapter cable (docs/wiring.md).
- Slice 3 `runtime-config` (settings struct, `:XC` commands, ULN2003 driver type).
- Slice 4 `homing` (microswitch homing, startup sequence).
- Slice 5 `esp32-bridge` (BLE + hotspot HTTPS/WSS).
- Slice 6 `pwa`, slice 7 `planes`, slice 8 `stellarium`.
- Small built-in object list on the screen (Messier + bright stars in flash) for GoTo without
  a phone.
- Focuser 28BYJ-48 + ULN2003 (hardware arriving later).
- Optional ALT motor on a free TMC2209 socket.

## Plans

## Bugs

- Upstream parser input validation (security review, 2026-11-10): `:SC` date (MeadeParserSet.cpp)
  accepts month/day out of range → bad index in `calculateDeltaJd`; range-check RA/DEC/lat/HA/time
  fields too. GPS command in MeadeCommandProcessor.cpp takes an unbounded, unchecked timeout and
  blocks the main loop → strtol, reject negatives, clamp to 2 min. Must land before the bridge
  exposes the parser over BLE/WiFi (slice 4); pair with BLE bonding/whitelist.

## Debate

- Board reports 1.99 V target voltage on ST-Link alone. Re-probe and re-dump on PSU; compare
  SHA-256 with `backup/robin_e3_factory_full.bin` (`409c2a5b6cfdf5c3…`).
- Board revision is `_0003`; public schematics are `_004`. Verify the AT24C32 by I²C scan and the
  Z pins (PC14/PC15/PB8) on the real board.
- iPhone hotspot with no cellular signal in the field: does it stay up? Test before slice 4.
