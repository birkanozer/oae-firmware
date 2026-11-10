# point-adjust Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the LCD's Jog/GoTo/Sync/Home-Park/Setup menus with Point, Adjust and Settings, make long press mean back everywhere, keep home fixed at the OAE markers, and add the focus motor on the E socket.

**Architecture:** The pure core UI (`firmware/src/core/ui/`, host-tested with FakeCanvas) gets the new screens and two new interface items (`UiAction::Kind::MoveSteps`, snapshot axis fields). The STM32 adapter (`UiMountAdapter`) maps them to existing `Mount` calls (`moveStepperBy`, `setHome`, `startSlewingToHome`, `syncPosition`). Small board-guarded `Mount` changes cover clock-less Set home, park→home over LX200, and the focus driver guard.

**Tech Stack:** C++ (ETL, no STL), STM32duino on STM32F103RC, PlatformIO, googletest (native env), U8g2.

**Spec:** `plans/point-adjust/2026-11-10-point-adjust-design.md` (read it; its "Calls made after the builder check" section is binding).

## Global Constraints

- Gate: `bash tools/gate.sh` (native tests + `robin_e3` build) must exit 0 before every commit.
- Upstream files: small diffs guarded `#if BOARD == BOARD_STM32_MKS_ROBIN_E3`; clang-format v12 style (140 cols, Allman, 4 spaces), formatted by hand.
- Text: ASCII, EN and TR (no diacritics); menu rows ≤ 19 chars, full lines ≤ 21, header state ≤ 12 beside the clock, editors ≤ 12. Every new string goes through `uiText` and the AllScreensFit walk.
- Safety: any click or long press while the mount moves (slew, jog, Adjust step move, homing) issues STOP first, on every screen (M1). A driver fault locks motion. Never run a driver whose UART test failed.
- Home moves only through Set home here. No new path may call `setCurrentPosition` on RA/DEC except `Mount::setHome`.
- Commits: Conventional Commits, trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. CHANGELOG `[Unreleased]` bullet per task. Don't edit `plans/point-adjust/progress.md` (coordinator's ledger).
- Never flash in a task; the coordinator flashes after review.

## Review Focus

1. Long press while an Adjust step move is still running → STOP, not "back" (test in Task 4).
2. Fast turning at step size 10 000 → detents in one tick coalesce into one `MoveSteps` (steps × detents); the action queue never drops a Stop (test in Task 4).
3. Adjust on AZ/Focus whose driver failed → refused with a double beep and a footer message, no action (test in Task 4).
4. Set home here or Go home while moving or tracking → Set home needs idle (locked otherwise); Go home allowed from idle or tracking (test in Task 4).
5. Point GO/SYNC before date and time are set, or with a driver fault → locked, as today (test in Task 3).

---

### Task 1: Mount and adapter support for Adjust, clock-less home and park→home

**Files:**
- Modify: `firmware/src/core/ui/UiController.hpp` (structs only: append fields/kinds below)
- Modify: `firmware/src/adapters/UiMountAdapter.cpp`, `.hpp`
- Modify: `firmware/src/Mount.cpp`, `firmware/src/Mount.hpp` (guarded)
- Modify: `firmware/src/MeadeCommandProcessor.cpp` (guarded `:hP` / `:hU`)
- Modify: `docs/architecture.md` (LX200 command list for the app), `CHANGELOG.md`
- Test: `firmware/unit_tests/test_core/ui/test_ui_controller.cpp` (struct defaults only)

**Interfaces:**
- Produces (core, appended at the end of the existing structs/enums so existing values keep their numbers):
  - `UiAction::Kind::MoveSteps` — `a` = signed step count, `b` = axis (`0` RA, `1` DEC, `2` AZ, `3` Focus).
  - `MountSnapshot` fields: `bool hasAz, hasFocus;` `bool azFault, focusFault;` `int32_t axisPos[4];` (current step position per axis, slew-mode steps, RA/DEC/AZ/Focus) and `float stepsPerDeg[3];` (RA slew, DEC slew, AZ; 0 when unknown).
- Produces (Mount, guarded): `bool Mount::setHomeHere();` (true when done; zeroes RA/DEC counters at the current position; if date+time are not set yet, marks the sky reference pending), `void Mount::applyPendingHomeReference();` (called by the adapter after SetDate/SetTime once both are set), `bool Mount::azDriverFailed() const;`, `bool Mount::focusDriverFailed() const;` (Task 2 fills the focus flag; until then return false).
- Adapter mapping: `MoveSteps` → `Mount::moveStepperBy(axis, steps)` (RA_STEPS, DEC_STEPS, AZIMUTH_STEPS, FOCUS_STEPS); `SetHome` → `setHomeHere()`; `GoHome` unchanged (startSlewingToHome + suppress tracking on arrival); `Park`/`Unpark` kinds stay in the enum but the UI no longer emits them.

- [ ] **Step 1: Write the failing test** — `TEST(UiControllerTest, SnapshotDefaultsHaveNoAzFocus)`: a value-initialised `MountSnapshot{}` has `hasAz == false`, `hasFocus == false`, `axisPos[0..3] == 0`; `static_cast<int>(UiAction::Kind::MoveSteps) > static_cast<int>(UiAction::Kind::SetSupplyVoltage)`.
- [ ] **Step 2: Run** `bash tools/gate.sh` — expected: native build fails (fields missing).
- [ ] **Step 3: Add the struct fields and kind** in `UiController.hpp` as listed above; update the comment block above `int32_t a`.
- [ ] **Step 4: Verify RA step moves keep tracking.** Read `Mount::moveStepperBy` (RA case) and the arrival block in `Mount::loop`. If tracking that was on before an RA `moveStepperBy` does not restart on arrival, or the tracker-off time is not compensated, add a guarded fix in `Mount.cpp`. Write the finding in the commit body.
- [ ] **Step 5: Implement `setHomeHere()` / `applyPendingHomeReference()`** in `Mount.cpp` (guarded): when `isDateSet() && isTimeSet()` call the existing `setHome(true)`; otherwise set both stepper positions to 0 the same way `setHome` does, leave the zero-position sky reference to `applyPendingHomeReference()`, and set a pending flag that function clears after recomputing `_zeroPosRA` from LST and `_zeroPosDEC = 0` exactly as `setHome` does. Must not move any motor.
- [ ] **Step 6: Adapter** — map `MoveSteps` and `SetHome` as above; call `applyPendingHomeReference()` after SetDate/SetTime when both are set; fill the new snapshot fields (`hasAz` = `AZ_STEPPER_TYPE != STEPPER_TYPE_NONE`, `hasFocus` likewise, faults from the new getters, positions from the steppers, steps/deg from `getStepsPerDegree`).
- [ ] **Step 7: LX200 park** — guarded in `MeadeCommandProcessor.cpp`: `:hP` calls the same path as the UI's GoHome (go home, tracking off after arrival); `:hU` replies success and does nothing.
- [ ] **Step 8: Docs** — `docs/architecture.md`: list the exact LX200 strings the phone uses for move-by-steps per axis, sync (`:CM`), set home, go home, park. CHANGELOG bullet.
- [ ] **Step 9: Run** `bash tools/gate.sh` — expected: exit 0.
- [ ] **Step 10: Commit** `feat(firmware): move-by-steps action, clock-less set home, park goes home`.

### Task 2: Focus motor on the E socket

Independent of Tasks 1, 3, 4 (run in a worktree; the coordinator merges).

**Files:**
- Modify: `firmware/boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp`
- Modify: `firmware/Configuration_local_examples/Configuration_local_robin_e3.hpp`, `firmware/Configuration_adv.hpp`, `firmware/ConfigurationValidation.hpp`
- Modify: `firmware/src/Mount.cpp`, `firmware/src/Mount.hpp`, `firmware/src/b_setup.hpp` (guarded)
- Modify: `docs/wiring.md`, `CHANGELOG.md`

**Interfaces:**
- Produces: `bool Mount::focusDriverFailed() const;` backed by `_focusDriverFailed` (set by `connectToDriver("FOC")` on failure, exactly like `_azDriverFailed`). If Task 1 already declared the getter, keep one definition.

- [ ] **Step 1: Pins** — focus on the E socket: STEP PB4, DIR PB3, EN PB5, UART PC11 (`FOCUS_SERIAL_PORT_TX` = `_RX` = PC11), address 0.
- [ ] **Step 2: Config** — example config: `FOCUS_STEPPER_TYPE STEPPER_TYPE_ENABLED`, `FOCUS_DRIVER_TYPE DRIVER_TYPE_TMC2209_UART`, `FOCUS_MOTOR_CURRENT_RATING 150`, `FOCUS_OPERATING_CURRENT_SETTING 100`, hold low, released when idle (focus "always on" off), `FOCUS_STEPPER_SPR 2048`, `FOCUS_MICROSTEPPING 16`, speed/accel modest (like AZ: 400 full steps/s, 200 full steps/s² × microstepping) — use the upstream focus define names.
- [ ] **Step 3: Safety** — EN driven HIGH in `b_setup` before `configureFocusDriver`; every focus move path (`focusMoveBy`, `moveStepperBy(FOCUS_STEPS)`, any continuous focus move) refuses without changing position when `_focusDriverFailed`; board-guarded `#error` in `ConfigurationValidation.hpp` unless `UART_CONNECTION_TEST_TXRX == 1` for focus on TMC2209 UART. Reuse the AZ pattern (`enableAzMotor()` returns bool).
- [ ] **Step 4: Docs** — `docs/wiring.md`: focus on the E socket, coil pairs by continuity, current formula (same as AZ).
- [ ] **Step 5: Run** `bash tools/gate.sh` — expected: exit 0; also build once with `UART_CONNECTION_TEST_TXRX` 0 to see the `#error`, then restore.
- [ ] **Step 6: Commit** `feat(firmware): focus on tmc2209 e socket for bipolar 28byj-48`.

### Task 3: Menu tree, Point page, long press = back

**Files:**
- Modify: `firmware/src/core/ui/UiController.hpp`, `UiController.cpp`, `UiScreens.cpp`, `UiText.hpp`, `UiText.cpp`, `UiSettings.cpp`
- Test: `firmware/unit_tests/test_core/ui/test_ui_flows.cpp`, `test_ui_controller.cpp`, `test_ui_text.cpp`

**Interfaces:**
- Consumes: Task 1 structs.
- Produces: main menu rows `RowMotion, RowPoint, RowAdjust, RowSettings` (no Back row; long press is back); `Screen::PointConfirm` replacing GotoConfirm/SyncConfirm; `Screen::Settings` list whose first row opens Setup; removed screens: JogMenu, Jog, HomePark, SetHomeConfirm (Task 4 adds Adjust screens); `Edit::PointRa`, `Edit::PointDec` replacing Goto*/Sync*. The last confirmed Point coordinates are stored as the target (`_target` RA seconds / Dec arc-sec, `_hasTarget`), used by Task 4.

- [ ] **Step 1: Write failing tests** (test_ui_flows.cpp):
  - `MainMenuIsMotionPointAdjustSettings` — rows drawn in order "Track here", "Point", "Adjust", "Settings" (EN).
  - `PointConfirmTurnSelectsGoOrSync` — after RA/Dec editors, the confirm shows GO selected; one detent selects SYNC; click after 600 ms emits `Sync` with the entered values; same path with GO emits `Goto`.
  - `PointConfirmNotArmedBefore600ms` — click at 599 ms emits nothing.
  - `PointGoRefusedBelowMinAlt` / `PointGoRefusedNearSun` — reuse today's refused tests on the new screen.
  - `PointLockedWithoutDateTime` and `PointLockedOnFault` (Review Focus 5).
  - `LongPressBacksOutOfEveryScreen` — from Settings, Setup, PointConfirm, Refused and every Settings row editor, a long press returns to the parent; on the main menu it changes nothing.
  - `LongPressInEditorGoesToPreviousField` — existing behaviour kept; on the first field it cancels.
  - `LongPressWhileMovingIsStop` — with `state = Slewing`, a long press on Settings emits `Stop` and stays.
  - `SetupIsUnderSettings` — Settings first row opens Setup; Setup rows Date, Time, UTC, Lat, Lon.
  - Update or delete the tests for removed screens (Jog menu, Home/Park, Sync row, Sync on target row).
- [ ] **Step 2: Run** `bash tools/gate.sh` — expected: the new tests fail.
- [ ] **Step 3: Implement** the menu rows, the PointConfirm screen (reuse `evalGoto()` for GO; SYNC keeps today's big-jump frame and "Clears Sync" rule), the Settings list with Setup as its first row, and the uniform long-press → parent handling in `holdScreen` (editors keep previous-field). Texts: "Point"/"Hedef", "Adjust"/"Ayar", "GO"/"GIT", "SYNC"/"ESITLE" (builder may adjust TR wording within limits).
- [ ] **Step 4: Run** `bash tools/gate.sh` — expected: exit 0, AllScreensFit passes in EN and TR.
- [ ] **Step 5: Commit** `feat(ui): point page, settings holds setup, long press is back`.

### Task 4: Adjust page

**Files:**
- Modify: `firmware/src/core/ui/UiController.hpp`, `UiController.cpp`, `UiScreens.cpp`, `UiText.hpp`, `UiText.cpp`
- Modify: `firmware/src/adapters/UiMountAdapter.cpp` only if Task 1's mapping needs a fix
- Modify: `README.md` (LCD usage), `CHANGELOG.md`
- Test: `firmware/unit_tests/test_core/ui/test_ui_flows.cpp`

**Interfaces:**
- Consumes: Task 1 `MoveSteps`, snapshot `hasAz/hasFocus/azFault/focusFault/axisPos/stepsPerDeg`; Task 3 `_target`/`_hasTarget`, menu `RowAdjust`.
- Produces: `Screen::Adjust`, `Screen::AdjustOptions`, `Screen::SetHomeConfirm` (re-added under Adjust options), step-size list `{1, 10, 100, 1000, 10000, JOG SLOW, JOG CENTER, JOG FIND, JOG MAX}`.

- [ ] **Step 1: Write failing tests** (test_ui_flows.cpp):
  - `AdjustTurnEmitsMoveSteps` — step 100, axis RA, one detent clockwise → `MoveSteps a=100 b=0`; counter-clockwise → `a=-100`.
  - `AdjustDetentsCoalescePerTick` — 7 detents queued before one tick at step 10000 → one `MoveSteps a=70000` (Review Focus 2).
  - `AdjustClickCyclesAxes` — RA → DEC → AZ → Focus → RA with `hasAz = hasFocus = true`; with both false, RA ↔ DEC only.
  - `AdjustJogUsesJogStartStop` — step JOG CENTER on RA: a detent emits `SetSlewRate 2` then `JogStart` (E/W bits); 300 ms without detents emits `JogStop`. JOG sizes are offered only for RA/DEC.
  - `AdjustLongPressOpensOptions` — options list: Step size, Sync on target, Set home here, Go home.
  - `AdjustLongPressWhileMovingIsStop` — `state = Jogging` or `Slewing`: long press emits `Stop`, options not opened (Review Focus 1).
  - `AdjustRefusesFailedDriver` — axis AZ with `azFault = true`: a detent emits `BeepError`, no `MoveSteps`, footer message shown 2 s (Review Focus 3).
  - `SyncOnTargetNeedsTargetAndClock` — locked without a Point target or without date+time; otherwise emits `Sync` with the stored target.
  - `SetHomeNeedsIdle` — locked while Tracking/Slewing; confirm then emits `SetHome` when Idle. `GoHomeAllowedFromTracking` emits `GoHome` (Review Focus 4).
  - `AdjustWorksWithoutDateTime` — MoveSteps and SetHome do not need date+time.
  - `AdjustShowsDegreesForStep` — step 1000 with `stepsPerDeg[0] = 1000.0` draws a "1.00" degree hint; Focus shows steps only.
- [ ] **Step 2: Run** `bash tools/gate.sh` — expected: the new tests fail.
- [ ] **Step 3: Implement** the Adjust screen (axis, step size, position from `axisPos`, degree hint), the options list, Set home confirm, Go home (HOMING screen as today), using the existing jog timing for JOG sizes. Fault lock as for other motion rows.
- [ ] **Step 4: README** — short LCD usage: Point, Adjust (aligning to the OAE markers, then Set home here), Settings.
- [ ] **Step 5: Run** `bash tools/gate.sh` — expected: exit 0, AllScreensFit passes.
- [ ] **Step 6: Commit** `feat(ui): adjust page with step moves, sync on target and home`.
