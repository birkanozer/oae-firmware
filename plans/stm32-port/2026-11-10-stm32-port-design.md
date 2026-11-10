# stm32-port — design

Slice 1 of [docs/architecture.md](../../docs/architecture.md) §8. Port OpenAstroTracker-Firmware
v1.14.0 to the MKS Robin E3 v1.1 so that, on a powered board, LX200 over USB and over USART2
drives RA (X) and DEC (Y) with TMC2209s configured over UART, and settings survive a reboot.

## Scope

In:
- Host toolchain (portable MinGW in `.tools/`) and `core_dir` so every tool stays in the repo
  folder; green `pio test -e native` baseline on untouched upstream first.
- `BOARD_STM32_MKS_ROBIN_E3` board id, `boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp`,
  `[env:robin_e3]` (platform `ststm32`, board `genericSTM32F103RC`, STM32duino core, upload and
  debug over `stlink`, app at `0x08000000`), a committed board configuration with stock OAE
  values (`#define OAE`, RA/DEC TMC2209_UART, AZ/ALT/FOCUS none).
- Stepper interrupt backend on STM32duino `HardwareTimer` behind upstream's
  `InterruptCallback` API.
- TMC2209 UART per driver on its own single-wire pin (X PC7, Y PD2, Z PC12, E PC11) via
  STM32duino `SoftwareSerial` in half-duplex (RX pin == TX pin), at 19200 baud.
- Persistent store: onboard AT24C32 over `Wire` (PB6/PB7), probed at boot; if it does not answer,
  STM32duino flash-emulated `EEPROM`. Same `EPROMStore` interface either way.
- LX200 on USB (`Serial`, USART1) and on USART2 (PA2 TX / PA3 RX) for the ESP32 bridge; one
  parser, replies go back to the port the command came from.
- Upstream parser hardening from the security review (`:SC` ranges, GPS timeout clamp), with
  native tests.

Out (later slices): runtime motor/gear settings, ULN2003 driver type, homing, AZ/ALT/focus,
bridge, app.

## Approach

Follow the RP2040 port (upstream PR #298): arch guards on `ARDUINO_ARCH_STM32`, new board id,
pins file, env, per-arch branches in `InterruptCallback.cpp` and `EPROMStore.cpp`. Keep every
upstream diff small and guarded so upstream merges stay possible.

Order by risk: toolchain baseline → env that compiles with steppers off → timer ISR → TMC
single-wire UART → store → second serial port → hardening. Everything compiles and links in CI
terms (`pio run -e robin_e3`) without hardware; hardware checks are a separate bench checklist
run once a 12/24 V supply is on.

## Risks

- **Single-wire TMC UART next to a fast stepper ISR.** SoftwareSerial bit timing is interrupt
  driven; the stepper timer can jitter it. Mitigation: configure drivers at boot before the
  stepper timer starts; later reads (`:XGMS`) briefly tolerate retries. Fallback: drive PDN pins
  from USART3/UART4/UART5 in half-duplex if pins allow (PC10/PC11 = UART4 TX/RX, PC12/PD2 =
  UART5 TX/RX — E and Z/Y share those).
- **Board `_0003` vs schematic `_004`.** Z pins and the AT24C32 may differ; the boot probe and
  a flash fallback cover the EEPROM, the bench checklist covers Z.
- **RAM.** 48 KB; keep displays, GPS and WiFi off for this board (config).

## Bench checklist (needs PSU, run after the slice merges)

1. Re-probe over SWD: target voltage ≈ 3.3 V; dump flash again and compare SHA-256 with the
   backup.
2. `pio run -e robin_e3 -t upload`; USB serial at the configured baud answers `:GVP#` and
   `:GVN#`.
3. `:XGMS#` (stepper info) reports TMC2209 version `0x21` for X and Y.
4. `:MXr…`/manual slew moves RA and DEC both ways; `:Q#` stops.
5. Change steps/deg with `:XSR…#`, power-cycle, `:XGR#` returns the new value.
6. Same `:GVP#` over USART2 with a USB-UART adapter on PA2/PA3.
