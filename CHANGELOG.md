# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
