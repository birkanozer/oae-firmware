# SDD ledger — plan: plans/stm32-port/2026-11-10-stm32-port-plan.md

Spec: plans/stm32-port/2026-11-10-stm32-port-design.md. Branch feature/stm32-port from dev b680bf8.
Workspace (briefs, reports, packages): .superpowers/sdd/2026-11-10-stm32-port-plan/

## Pre-flight scan

| Pair / task | Shared file or interface | Finding |
|---|---|---|
| T1–T2 | platformio.ini; gate.sh builds robin_e3 only once env exists | consistent |
| T2–T3 | pins file: T2 creates with all pins (Global Constraints list UART PC7/PD2/PC12/PC11), T3 "sets" UART pins | CONFLICT: T2 sets RA/DEC TMC2209_UART; ConfigurationValidation requires *_SERIAL_PORT_TX/_RX → T2 build cannot succeed without T3's pins |
| T2–T5 | f_serial.hpp: T2 adds STM32 polling, T5 refactors to per-port | sequential, consistent |
| T2–T4 | EPROMStore: T2 leaves AVR `#else` path (STM32duino EEPROM emulation compiles); T4 adds STM32 branch | consistent |
| T5–T2 | BRIDGE_SERIAL_PORT / BRIDGE_SERIAL_BAUDRATE defined in T2 pins file, used in T5 | consistent |
| JTAG-off constraint | no task owns it | GAP |
| T1 | gate.sh, CLAUDE.md gate key | self-consistent |
| T2 | static_assert ≥ 50 µs vs 0.2 ms interval | self-consistent |
| T3 | build-only verification | self-consistent after ruling 1 |
| T4 | test dir unit_tests/test_core/storage inside test_core suite; header in src/core (native build_src_filter +<./core>) | self-consistent |
| T5 | no native test possible (Arduino Stream) — verified by build | self-consistent |
| T6 | TDD in native meade tests | self-consistent |
| T7 | docs only | self-consistent |

Ruling: TMC per-axis UART pins (`*_SERIAL_PORT_TX` = `*_SERIAL_PORT_RX` = PDN pin, `*_DRIVER_ADDRESS 0`) move into Task 2's pins file; Task 3 becomes verification + boot-order check of the SW-serial path — the T2 build cannot pass validation otherwise — cost if wrong: none, same code lands one task earlier.
Ruling: JTAG-off (Global Constraints) is owned by Task 2 (first lines of STM32 setup) — it is board bring-up and T2 is the board task — cost if wrong: E stepper / screen pins dead until fixed, caught on the bench.

## Progress
Task 1: base eb76235
Task 2: base 97eb800 (dispatched while Task 1 review runs; Task 1 diff is toolchain-only, no overlap)
Task 1: cannot-verify resolved by controller — trailer Opus 5.5 present, gate.sh stored LF (eol=lf attr), .tools/ git-ignored; 342/342 per report.
Task 1: minor (deferred): report lacks raw output tail / other-cwd evidence.
Task 1: minor (deferred): gate.sh silently falls back to a global gcc if .tools/mingw64/bin is missing — add a dir check.
Task 1: minor (deferred): CLAUDE.md Commands section omits .tools/.
Task 1: complete (commits eb76235..97eb800, review clean)
Task 2: minor (deferred): InterruptCallback start()/stop() deref null timer if called before setInterval — guard or document.
Task 2: minor (deferred): static_assert checks macro but runtime uses intervalMs argument.
Task 2: minor (deferred): JTAG block guarded ARDUINO_ARCH_STM32; AFIO macros are F1-only.
Task 2: minor (deferred): endstops under *_DIAG_PIN get pinMode INPUT (no pull-up); homing slice must set INPUT_PULLUP; PA11/PA12 = USB pins note; AZ needs step/dir overrides before enabling.
Task 2: minor (deferred): SERIAL_UART_INSTANCE / ENABLE_HWSERIAL2 flags uncommented.
Task 2: minor (deferred): matrix_build.py -b robin_e3 not run.
Task 2: cannot-verify carried to Task 3: SoftwareSerial TIM7 IRQ priority vs stepper TIM6.
Task 2: complete (commits 97eb800..930e25f, review clean)
Task 3: base 930e25f
Task 4: base afe578f (dispatched while Task 3 review runs; no shared files)
Task 3: minor (deferred): InterruptCallback priority comment overclaims "never held up"; reword.
Task 3: minor (deferred): literal priority 15 lacks ceiling note (lowest F1 level; later timer ISRs unordered vs stepper).
Task 3: minor (deferred): report lacks pasted gate output tail.
Task 3: cannot-verify → bench checklist: TMC UART on hardware, stepper jitter under TIM7 load, stepper ISR duration vs 17 us TIM7 tick. Build re-verified by the final gate run.
Task 3: complete (commits 930e25f..afe578f, review clean)
Task 5: base 9c606ee (dispatched while Task 4 review runs; no shared files)
Task 4: fix round 1/5 dispatched (3 Important: read failure reads as blank → flag wipe; silent update failure; EEPROM.write wears flash) — FIX_BASE 9c606ee
Task 4: fix round 1/5 (3 addressed, 0 open; commits 9c606ee..8cef344)
Task 4: minor (deferred): busFailed latch is sticky until reboot; failure LOG only under DEBUG_EEPROM (silent in prod) — surface on screen/status later.
Task 4: minor (deferred): STM32 wrapper latch has no unit test (STM32-only code), verified by reading.
Task 4: minor (deferred): check STM32duino EEPROM page-per-byte flush vs buffered API with commit() (pre-existing behaviour, update() limits it to changed bytes).
Task 4: minor (deferred): read() returns 0xFF on failure with no error channel (ponytail-marked).
Task 4: note for Task 7: CHANGELOG bullet for the fix round (bus-failure latch, EEPROM.update) not yet written — fixer was told not to touch CHANGELOG.
Task 4: complete (commits afe578f..8cef344, review clean after 1 fix round)
Task 6: base 49b29c6 (dispatched while Task 5 review runs; told not to touch CHANGELOG — Task 7 writes its bullet)
Task 5: Ruling: over-long command now dropped up to its '#' on ALL boards (upstream ran the tail as a new command) — Review Focus requires drop-not-overflow and the old tail-execution is a command-injection shape at a BLE-reachable boundary — cost if wrong: a client relying on >19-char commands breaks (none exist; longest LX200 command fits).
Task 5: minor (deferred): STM32 reply gate duplicated in two places.
Task 5: minor (deferred): report lacked pasted gate output (requested in fix round).
Task 5: cannot-verify → bench: Serial2 on PA2/PA3; other boards' compile (ramps/esp32/rp2040 toolchains not installed).
Task 5: fix round 1/5 dispatched (1 Important: unbounded read loop starves USB under Serial2 flood; + ACK-in-discard minor) — FIX_BASE 49b29c6
Task 5: fix round 1/5 (3 addressed, 0 open; commits 49b29c6..527278d)
Task 5: minor (deferred): Serial2 HW RX ring (SERIAL_RX_BUFFER_SIZE, default 64) may overrun on a slow loop + BLE burst — check in slice 5 (bridge), consider -D SERIAL_RX_BUFFER_SIZE=256.
Task 5: minor (deferred): processSerialData has no native test (Arduino Stream); gate is compile proof only.
Task 5: note for Task 7 CHANGELOG: "Serial polling bounded to 64 bytes per port per loop; an over-long LX200 command is ignored in full (including any ACK inside it) until its #."
Task 5: complete (commits 8cef344..527278d, review clean after 1 fix round)
Task 6: Ruling: missing CHANGELOG Security bullet is a controller-caused gap (implementer was told not to touch CHANGELOG while Task 5 ran) — no concurrent editors remain, so Task 6 writes it in fix round 1 — cost if wrong: none.
Task 6: fix round 1/5 dispatched (Important: min/sec >59 accepted → DayTime >24h into sync/setLST; Important: CHANGELOG Security bullet; folded minors: boundary-accept tests, JD sentinel comment, real test output) — FIX_BASE ebac93e
Task 7: base d3115cb (dispatched while Task 6 re-review runs; docs only)
Task 6: fix round 1/5 (2 Important + minors addressed, 0 open; commits ebac93e..d3115cb)
Task 6: minor (deferred): :SG UTC-offset hours has no range check.
Task 6: minor (deferred): accept tests don't hit mm/ss=59 on :SY/:SH/:SHL paths.
Task 6: minor (deferred): GPS timeout clamp under USE_GPS==1 is never compiled by any env (eye-checked twice: const char*, strtol, clamp).
Task 6: complete (commits 49b29c6..d3115cb excluding 527278d, review clean after 1 fix round)
Task 7: cannot-verify resolved by controller — SERIAL_BAUDRATE on STM32 = SERIAL_BAUDRATE_ASCOM (19200), Configuration.hpp:180-186.
Task 7: minor (deferred): docs/wiring.md "Other" lists runout PB10 as unused, but PB10 is reserved as screen USART3 TX.
Task 7: complete (commits d3115cb..67c2f15, review clean)
Final review: 6 Important, 8 Minor, 0 Critical (final-review.md). Deferred triage: must-fix none; can-wait 23.
Ruling: CORRECTION of the Task 5 ruling — "longest LX200 command fits in 19 chars" was wrong: :SY sync is 21 chars; buffer goes to 32 in the final fix wave — cost if wrong: none, bounded either way.
Final fix wave dispatched (I1-I6 + M1-M6) — FIX_BASE 6d476d6
Final fix wave: all 12 addressed (commits 6d476d6..1ee1cba), re-review clean. Deferred: flash-fallback page-per-byte wear; other boards not compiled locally; dead TMC driver adds up to 5x500 ms boot delay.
Gate (controller): 376/376 native, robin_e3 SUCCESS, RAM 10.6%, flash 37.1%.
