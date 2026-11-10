# stm32-port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** OAT firmware v1.14.0 builds for the MKS Robin E3 v1.1 (`pio run -e robin_e3`) with a
timer-driven stepper ISR, single-wire TMC2209 UART, persistent settings and LX200 on two serial
ports, and the native test gate is green on Windows.

**Architecture:** Follow the upstream RP2040 port: a new board id, a pins file, a PlatformIO env,
and `#if` branches on `ARDUINO_ARCH_STM32` in the few arch-specific files. Every upstream edit is
small and guarded. Stock OAE values are defaults in the pins file and a committed example config.

**Tech Stack:** PlatformIO 6.2 (`.venv`), platform `ststm32` + STM32duino Arduino core, board
`genericSTM32F103RC`, TMCStepper 0.7.x, AccelStepper 1.6x, googletest on the native env with
WinLibs GCC 16.2 (`.tools/mingw64`).

**Spec:** [2026-11-10-stm32-port-design.md](2026-11-10-stm32-port-design.md), pins in
[docs/wiring.md](../../docs/wiring.md).

## Global Constraints

- All paths below are relative to `firmware/` unless they start with `/` (= repo root).
- Run PlatformIO as `../.venv/Scripts/pio.exe` from `firmware/`; never a global `pio`.
- The gate is `bash /tools/gate.sh` (created in Task 1); it must pass at the end of every task.
- Upstream edits: guarded by `BOARD == BOARD_STM32_MKS_ROBIN_E3` or `defined(ARDUINO_ARCH_STM32)`;
  no reformatting of untouched lines; clang-format style of the file.
- Board id `BOARD_STM32_MKS_ROBIN_E3` = `3001`. Env name `robin_e3`. Pins file
  `boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp`.
- Pins exactly as `/docs/wiring.md` (X STEP PC0 DIR PB2 EN PC13 UART PC7; Y PC2/PB9/PB12/PD2;
  Z PC14/PC15/PB8/PC12; E PB4/PB3/PB5/PC11; endstops PA12, PA11, PC6, PB1; I²C PB6 SCL/PB7 SDA;
  USART2 PA2 TX/PA3 RX).
- Timers: stepper ISR on `TIM6`; SoftwareSerial on `TIM7` (`-D TIMER_SERIAL=TIM7`).
- TMC2209: `R_SENSE` 0.11, software serial 19200 baud, RX pin == TX pin per driver, address 0.
- Serial: USB `Serial` (USART1) at `SERIAL_BAUDRATE` (upstream 19200); bridge `Serial2`
  (USART2) at `BRIDGE_SERIAL_BAUDRATE` = 115200.
- No display, no GPS, no WiFi, no gyro on this board (`DISPLAY_TYPE_NONE`,
  `INFO_DISPLAY_TYPE_NONE`, `USE_GPS 0`, `USE_GYRO_LEVEL 0`).
- Commits: Conventional Commits, trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Root `/CHANGELOG.md` `## [Unreleased]` gets one bullet per task.

## Review Focus

- LX200 bytes arriving on both ports at once must not interleave into one command buffer (per-port
  buffers, Task 5).
- An AT24C32 that is absent or NACKs must not hang boot: probe with a timeout and fall back to
  flash EEPROM (Task 4 test `FallsBackWhenProbeFails`).
- Over-long commands (> buffer) on either port must be dropped, not overflow (Task 5).
- `:SC` with month 13 / day 0 / day 32 and the GPS command with a negative or huge timeout are
  rejected or clamped (Task 6 tests).
- The stepper ISR period must equal `STEPPER_CONTROL_INTERVAL_MS` within one timer tick
  (Task 2 `static_assert` on the computed overflow).

---

### Task 1: Toolchain and native baseline

**Files:**
- Create: `/tools/gate.sh`
- Modify: `platformio.ini` (`[platformio]` section)
- Modify: `/CLAUDE.md` (`## Workflow` gate line), `/CHANGELOG.md`

**Interfaces:**
- Produces: `bash /tools/gate.sh` — prepends `<repo>/.tools/mingw64/bin` to `PATH` (absolute,
  from the script's own location), then runs `pio test -e native` and, once Task 2 lands,
  `pio run -e robin_e3` (skip the build when `[env:robin_e3]` is absent). Exits non-zero on any
  failure.

- [ ] **Step 1:** Add `core_dir = ../.pio-core` under `[platformio]` in `platformio.ini`.
- [ ] **Step 2:** Write `/tools/gate.sh` per Interfaces (`set -euo pipefail`).
- [ ] **Step 3:** Run `bash /tools/gate.sh` on untouched upstream code.
  Expected: googletest summary, all tests PASSED. If a failure is Windows-specific (path, MinGW
  quirk), fix it in the narrowest place and note it in `/CHANGELOG.md`; a real upstream test
  failure stops the task and is reported.
- [ ] **Step 4:** Set the `gate:` key in `/CLAUDE.md` to `bash tools/gate.sh`.
- [ ] **Step 5:** Commit `chore: local toolchain and native test gate`.

### Task 2: Board, env and stepper timer — `robin_e3` builds

**Files:**
- Modify: `Constants.hpp` (board id), `LocalConfiguration.hpp` (elif: include
  `Configuration_local_robin_e3.hpp` if present, else
  `Configuration_local_examples/Configuration_local_robin_e3.hpp`), `Configuration.hpp:295-298`
  (pins include), `Configuration_adv.hpp:43-48` (`STEPPER_CONTROL_INTERVAL_MS` 0.2f on STM32),
  `ConfigurationValidation.hpp:20,27-36,86,107` (allow STM32 like RP2040),
  `src/InterruptCallback.cpp:40-59,84-123` (STM32 branch), `src/Mount.cpp:1166-1171`
  (hardware info `"STM32F103,"`), `src/c_buttons.hpp:101-106` and `src/f_serial.hpp:22,37`
  (poll `processSerialData()` each loop on STM32, as RP2040/ESP32), `platformio.ini`,
  `matrix_build.py:28-34,182-240` (add the board).
- Create: `boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp`,
  `Configuration_local_examples/Configuration_local_robin_e3.hpp`.

**Interfaces:**
- Produces: `[env:robin_e3]` — `extends = common_embedded`, `platform = ststm32@<exact version
  installed>`, `board = genericSTM32F103RC`, `upload_protocol = stlink`, `debug_tool = stlink`,
  `monitor_speed = 19200`, `lib_deps` = TinyGPSPlus, TMCStepper, AccelStepper,
  esp8266-oled-ssd1306#4.6.2 (no U8g2 / LiquidCrystal / LiquidTWI2), `lib_ignore =
  TimerInterrupt`, `build_flags = ${env.build_flags} -D BOARD=BOARD_STM32_MKS_ROBIN_E3
  -D TIMER_SERIAL=TIM7 -D ENABLE_HWSERIAL2 -D SERIAL_UART_INSTANCE=1`. App at `0x08000000`
  (platform default; no offset).
- Produces: pins file with every pin from Global Constraints as `#ifndef` defaults, plus stock
  OAE mechanics copied from `boards/ESP32_ESP32DEV/pins_OAE_V1.hpp` (RA_WHEEL_CIRCUMFERENCE,
  RA_STEPPER_SPR 400*9, RA limits, DEC_STEPPER_SPR 200*50*4.5, DEC wheel/pulley 1, AZ/ALT
  values), `SW_SERIAL_UART 1`, `DEBUG_SERIAL_PORT Serial`, `BRIDGE_SERIAL_PORT Serial2`,
  `BRIDGE_SERIAL_BAUDRATE 115200`.
- Produces: example config — `#define OAE`, `NORTHERN_HEMISPHERE 1`, displays none, RA/DEC
  `STEPPER_TYPE_ENABLED` + `DRIVER_TYPE_TMC2209_UART`, AZ/ALT/FOCUS `STEPPER_TYPE_NONE`,
  `USE_GPS 0`, `USE_GYRO_LEVEL 0`, `TRACK_ON_BOOT 0`, RA/DEC motor current rating as in the RP2040
  example (470 mA rating / 75 %).
- Produces: STM32 `InterruptCallback` — `setInterval(float ms, cb, payload)` creates a
  `HardwareTimer(TIM6)`, `setOverflow(round(ms*1000), MICROSEC_FORMAT)`, attaches a handler
  calling `cb(payload)`, `resume()`, returns true; `start()` = `resume()`, `stop()` = `pause()`.
  `static_assert` that `STEPPER_CONTROL_INTERVAL_MS * 1000` rounds to an integer ≥ 50 µs.

- [ ] **Step 1:** Add board id, pins file, example config, LocalConfiguration/Configuration
  includes, validation whitelist entries.
- [ ] **Step 2:** Add `[env:robin_e3]`; run `../.venv/Scripts/pio.exe run -e robin_e3`.
  Expected: fails at `InterruptCallback.cpp` `#error` (proves the board is picked up).
- [ ] **Step 3:** Implement the STM32 `InterruptCallback` branch, the loop polling, hardware info.
- [ ] **Step 4:** Run `pio run -e robin_e3`. Expected: SUCCESS; record RAM/flash usage from the
  size report in `/CHANGELOG.md` (must be < 90 % of 48 KB RAM and 256 KB flash; if not, switch
  this env to `-Os` and re-check).
- [ ] **Step 5:** Run `bash /tools/gate.sh`. Expected: native tests pass and `robin_e3` builds.
- [ ] **Step 6:** Commit `feat(firmware): mks robin e3 board, env and stm32 stepper timer`.

### Task 3: Single-wire TMC2209 UART

**Files:**
- Modify: `boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp` (per-axis
  `*_SERIAL_PORT_TX` = `*_SERIAL_PORT_RX` = PDN pin, `*_DRIVER_ADDRESS 0`),
  `src/b_setup.hpp` / `src/Mount.cpp` only if the SW-serial path needs an STM32 guard.

**Interfaces:**
- Consumes: Task 2 env (`TIMER_SERIAL=TIM7`), upstream SW-serial ctor
  `TMC2209Stepper(uint16_t RX, uint16_t TX, float rsense, uint8_t addr)` + `beginSerial(19200)`.
- Produces: RA and DEC drivers configured over UART at boot, **before** `InterruptCallback::
  setInterval` starts the stepper timer (assert the order in `b_setup.hpp`; move only if wrong).

- [ ] **Step 1:** Set the pins; build. Expected: SUCCESS with TMCStepper's SoftwareSerial path
  (confirm in the build log that `SoftwareSerial.cpp` from the STM32 core is compiled).
- [ ] **Step 2:** Confirm `SoftwareSerial(pin, pin)` is half-duplex in the installed STM32duino
  core (read `SoftwareSerial.cpp`: RX == TX → half-duplex); note the core version in
  `/CHANGELOG.md`.
- [ ] **Step 3:** Run `bash /tools/gate.sh`. Expected: pass.
- [ ] **Step 4:** Commit `feat(firmware): tmc2209 single-wire uart on robin e3`.

### Task 4: Persistent store — AT24C32 with flash fallback

**Files:**
- Create: `src/core/storage/I2cEeprom.hpp` (header-only, no Arduino includes),
  `unit_tests/test_core/storage/test_i2c_eeprom.cpp`.
- Modify: `src/EPROMStore.cpp` (STM32 branch before the AVR `#else`, `<EEPROM.h>` include kept),
  `platformio.ini` (`[env:native]` test sources pick up the new test dir if the filter needs it).

**Interfaces:**
- Produces: `template <class Bus> class I2cEeprom` with
  `I2cEeprom(Bus& bus, uint8_t devAddr = 0x50)`, `bool probe()`, `uint8_t read(uint16_t addr)`,
  `void update(uint16_t addr, uint8_t value)` (read-compare, write only on change, then
  ack-poll until the device answers or 10 ms pass). `Bus` provides
  `bool write(uint8_t dev, const uint8_t* data, size_t n)` (true = ACK) and
  `bool read(uint8_t dev, uint8_t* out, size_t n)`. Memory address sent big-endian, 2 bytes.
- Produces: STM32 `EEPROMStore` backend — `initialize()` runs `Wire.setSDA(PB7); Wire.setSCL(PB6);
  Wire.begin()`, probes; on success uses `I2cEeprom` through a `Wire` adapter, otherwise
  STM32duino `EEPROM` (flash emulation) and logs which backend is active. `commit()` no-op.

- [ ] **Step 1: Write failing tests** with a fake bus recording traffic:
  `ReadSendsBigEndianAddress` (read(0x0123) writes `{0x01,0x23}` then reads 1 byte);
  `UpdateSkipsWriteWhenUnchanged`; `UpdateWritesAddressAndValue` (`{hi,lo,value}`);
  `UpdatePollsUntilAck` (bus NACKs twice then ACKs → returns, 3 polls);
  `ProbeFalseOnNack`; `FallsBackWhenProbeFails` is covered by `probe()` returning false.
- [ ] **Step 2:** Run `bash /tools/gate.sh`. Expected: FAIL (`I2cEeprom` undefined).
- [ ] **Step 3:** Implement `I2cEeprom`; inject the clock for the 10 ms poll limit as a
  `Bus::millis()` member so the test controls time.
- [ ] **Step 4:** Run the gate. Expected: new tests PASS.
- [ ] **Step 5:** Implement the STM32 `EEPROMStore` branch; build `robin_e3`. Expected: SUCCESS.
- [ ] **Step 6:** Commit `feat(firmware): at24c32 settings store with flash fallback`.

### Task 5: LX200 on USB and USART2

**Files:**
- Modify: `src/f_serial.hpp:104-161` (and the `TEST_VERIFY_MODE` copy at :46-107 only if it
  must compile), `src/b_setup.hpp` (begin `BRIDGE_SERIAL_PORT` after the debug port, ~:141).

**Interfaces:**
- Produces: `void processSerialData(Stream& port, SerialBuffer& buf)` with
  `struct SerialBuffer { char data[20]; uint8_t len; }` — one static `SerialBuffer` per port;
  bytes beyond 19 before `#` discard the partial command and reset `len`; ACK `0x06` answers `P`
  on the same port; replies are printed to the port the command came from. On STM32 the loop
  polls `Serial` and `BRIDGE_SERIAL_PORT` every iteration; other boards keep calling the
  upstream behaviour with `Serial` only.

- [ ] **Step 1:** Refactor to the per-port function; other boards pass `Serial`.
- [ ] **Step 2:** Begin `Serial2` at `BRIDGE_SERIAL_BAUDRATE` on STM32 only; poll both ports.
- [ ] **Step 3:** Build every env the local toolchain can build without new downloads failing
  (`robin_e3` required; `native` via gate). Expected: SUCCESS.
- [ ] **Step 4:** Commit `feat(firmware): lx200 on usb and bridge uart`.

### Task 6: Parser hardening

**Files:**
- Modify: `src/core/meade/MeadeParserSet.cpp` (`'C'` case and RA/DEC/lat/HA/time fields),
  the date→JD helper that indexes by month (find via `calculateDeltaJd`),
  `src/MeadeCommandProcessor.cpp` (GPS timeout parse).
- Test: `unit_tests/test_core/meade/` (extend the existing Set-parser test file).

**Interfaces:**
- Produces: `:SC mm/dd/yy#` replies `0` unless 1 ≤ mm ≤ 12 and 1 ≤ dd ≤ 31; JD helper rejects
  month outside 1..12. GPS timeout: `strtol`, negative or unparsable → default, clamp to
  120000 ms.

- [ ] **Step 1: Write failing tests:** `SetDateRejectsMonth13`, `SetDateRejectsMonth0`,
  `SetDateRejectsDay0`, `SetDateRejectsDay32`, `SetDateAcceptsValid` (`:SC12/31/26#` → ack as
  upstream), plus one out-of-range test each for RA hours ≥ 24, DEC |deg| > 90, latitude
  |deg| > 90, time hh ≥ 24 — each expecting the upstream "invalid" reply.
- [ ] **Step 2:** Run the gate. Expected: the new tests FAIL.
- [ ] **Step 3:** Add the range checks; clamp the GPS timeout.
- [ ] **Step 4:** Run the gate. Expected: all PASS.
- [ ] **Step 5:** Move the `/BACKLOG.md` "Upstream parser input validation" bug out (into
  `/CHANGELOG.md` `### Security`).
- [ ] **Step 6:** Commit `fix(firmware): validate lx200 set-command ranges and gps timeout`.

### Task 7: Docs

**Files:**
- Modify: `/README.md` (build/flash commands match the env), `/docs/wiring.md` (anything the
  tasks changed), `/BACKLOG.md` (slice 1 → bench checklist item under Next).

- [ ] **Step 1:** Update the three files; the bench checklist from the design doc goes into
  `/BACKLOG.md` Next as "Slice 1 bench test (needs PSU)".
- [ ] **Step 2:** Run the gate. Expected: pass.
- [ ] **Step 3:** Commit `docs: stm32 port build, flash and bench checklist`.
