# Final review: feature/adjust-per-axis

Reviewed: commits 094f12a (docs), d26429a (config), 2ac02ae (UI) against `dev`. I read the full diff and the code around it:
UiController.cpp/.hpp, UiScreens.cpp, UiMountAdapter.cpp/.hpp, LcdUi.cpp, Knob.cpp, `Mount::moveStepperBy` / `stepMoveBase` /
`stopSlewing`, Configuration_adv.hpp, the test harness. I did not run the gate. The implementer reports 554/554 native and a
clean robin_e3 build.

## Verdict 1: spec compliance

| # | Requirement | Status |
|---|---|---|
| 1a | Long press on Adjust options → Adjust | Met (`UiController.cpp:428-430`). |
| 1b | "Main menu" row opens the main menu, never locked | Met (`UiScreens.cpp:278`, `:374-376`; `optionLock` default returns Count). |
| 1c | Main menu is the **last** row | **Changed, needs a user ruling.** The automatic Back row follows it (`UiScreens.cpp:283`). The rows are Step, Sync, Set home, Go home, Main menu, Back. Back now goes to Adjust, the same as a long press, which is consistent. Either keep it this way or drop Back on this list. |
| 1d | Long press on Set home confirm → options | Met (unchanged, `UiController.cpp:421-423`). |
| 1e | TR text | "Ana menu" in ASCII. This meets the binding ASCII/LCD-font rule; the design's "Ana menü" is not possible on this font. |
| 2 | A stop from Adjust (click or hold while moving) stays on Adjust, with STOPPED and a long beep; stops elsewhere → Status | Met (`UiController.cpp:148-157`). `renderAdjust` now draws the transient in its header (`UiScreens.cpp:624-625`); see finding M1. Adjust is still exempt from M2. Tested for click and hold × Slewing/Jogging/Homing. |
| 3a | Per-axis `_stepSel[5]` by axis id, default 2, RAM only | Met (`UiController.hpp:308`). |
| 3b | Step lists verbatim | Met: `kStepSizes[5][5]` (`UiScreens.cpp:20-24`) matches the design for RA, DEC, AZ, Focus and ALT. |
| 3c | Step option cycles 0..4 for the current axis only | Met (`UiScreens.cpp:359-360`). |
| 3d | UI jog removed; `MountState::Jogging` and LX200 jog untouched | Met on the UI side. `_jogDir`, `_jogMs`, `jogStop`, `speedText`, `kJogFirst`, `kJogStopMs`, the jog texts and the `JogStart` motion flag are all gone. The adapter side is left dead (finding M3). |
| 3e | The `MoveSteps` payload holds 127 × 300000 | Met: `int32_t` payload, `long` in `Mount::moveStepperBy` (32-bit), and a test covers it. |
| 4 | DEC_UART_STEALTH_MODE 0; ALT speed 200×µs, accel 100×µs; nothing else in the motor config | Met. The diff touches only those three defines plus comments. The local define wins over the Robin block's `#ifndef … 1` (`Configuration_adv.hpp:79-80`). `Mount.cpp:633/676/685` read it. Nothing else overrides it. |
| 5 | Style (col 140, ASCII TR, -Werror) | No added line exceeds 140 columns and none has tabs. clang-format itself could not be run (see below). |

Nothing was built beyond the brief except the header transient in `renderAdjust`, which the brief invited ("check renderAdjust still shows
the STOPPED header").

## Verdict 2: correctness and quality

### Important

**I1. A detent after a STOP on Adjust restarts motion right away.** Location: `UiController.cpp:239` with `:155`.
- **Before this branch:** `stop()` always switched to Status, where turning while moving does nothing. A STOP was final until the user deliberately went back into Adjust, and the menu locks Adjust while moving.
- **Now:** the screen stays on Adjust, and the `applyTurn` exception (Adjust, not Homing/Parking) still passes turns while the snapshot shows Slewing or Jogging. That lasts for the whole deceleration after the Stop, and for the rest of the tick in which the click arrived.
- **What happens on the board:** a turn calls `MoveSteps`, then `Mount::moveStepperBy` runs `moveTo(stepMoveBase + steps)`. `AccelStepper::stop()` set the target to the stopping point, so `stepMoveBase` returns that point and the axis sets off on a fresh move.
- **Nothing filters the stray detent:** `Knob.cpp` has no detent suppression around a press. The click fires on release (`Knob.cpp:85-89`), so the detent a finger makes when it lets go, or when it releases after a long-press STOP, lands in the next poll while the axis is still decelerating.
- **Concrete case:** an ALT move at 300000 steps (≈94 s at the new 3200 µsteps/s). The user clicks STOP, the knob ticks one detent on release, and ALT starts another 300000-step move. The header reads STOPPED while the axis moves.
- **Fix:** don't let a turn through the moving exception while a STOP is still settling. One line: add `&& _transient != TextId::HdrStopped` to the exception at `:239`. The STOPPED transient lasts 2 s, which outlasts the deceleration, and turns are fine again once `mountMoving()` is false. Add a test: click to STOP on Adjust while the state is Slewing, turn while it is still Slewing, and expect no `MoveSteps`.

### Minor

**M1. `renderAdjust` shows any transient, not only STOPPED.** Location: `UiScreens.cpp:624-625`. ARRIVED (3 s), SYNCED and HOME SET (2 s) are set and then the screen goes to Status. Status → click → turn → click reaches Adjust within that window, and the Adjust header then reads "ARRIVED" or "SYNCED" instead of "ADJUST". Fix: show only `HdrStopped` on Adjust (`_transient == TextId::HdrStopped && within(...)`).

**M2. A test that cannot fail.** Location: `test_ui_flows.cpp:1984` `AdjustOptionsHasNoJogSizes`. The "JOG" text and all the code that drew it are deleted, so this assertion is vacuous. `AdjustStepListsPerAxisDefaultMiddle` already proves the wrap at 5. Delete it.

**M3. The UI jog plumbing is now dead in the adapter and the snapshot. Remove it in this branch.**
- **Evidence:** a grep of `src/` and `unit_tests/` finds no emitter of `Kind::JogStart`, `JogStop` or `SetSlewRate`. LX200 jogging goes through `MeadeCommandProcessor` to `Mount` directly, never through `UiAction`. The brief's condition for keeping them ("if other code still uses those action kinds") is not met.
- **What goes:**
  - `UiAction::Kind::JogStart/JogStop/SetSlewRate` and their payload comment (`UiController.hpp:85-87`, `:106-107`).
  - The adapter cases (`UiMountAdapter.cpp:166-187`).
  - `_jogMask`, `_jogTail`, `_jogResumeTrack`, `_jogStoppedAt` (`UiMountAdapter.hpp:34-37`) and every reset of them in the other cases.
  - `kJogResumeMs` (`UiMountAdapter.cpp:10`) and the jog-tail block plus its comment in `state()` (`:47-61`).
  - `MountSnapshot.slewRate` (`UiController.hpp:61`), its fill (`UiMountAdapter.cpp:107`), the harness default (`UiHarness.hpp:33`) and the stale walk-test line (`test_ui_flows.cpp:1483`, "CENTER / ORTALA, the longest").
- **What must stay** in `state()`: the `STATUS_SLEWING_MANUAL` branch (LX200 manual slew → Jogging) and the `isAxisRunning(AZ/ALT/FOCUS)` branch (Adjust step moves → Jogging).
- **Size and risk:** about 45 lines. The robin_e3 build checks it. Behaviour cannot change, because nothing sends these kinds.
- **Severity:** Minor, because there is no runtime failure. But it is a state machine (`_jogTail` in `state()`) that can never run, and it misleads the next reader.

**M4. A test comment claims more than the test checks.** Location: `test_ui_flows.cpp:2008`. "Main menu is the row before it" is never checked there. `AdjustOptionsMainMenuRow` covers the row, so either add `u.turn(-1); EXPECT_TRUE(u.inv("Main menu"));` or drop the comment.

**M5. Format leftover.** Location: `UiDraw.hpp:16`. The `kArmMs` trailing comment is still padded to the column of the deleted `kJogStopMs` line. The block was not clang-format clean on dev either (its `=` signs are not aligned despite `AlignConsecutiveAssignments: true`). Cosmetic: re-pad it to two spaces or leave it.

### Checked, no issue

- **`_stepLatch` after a stop:** cleared in `stop()`. A turn followed by a click in the same tick still means STOP, not an axis cycle.
- **M2 window after a stop on Adjust:** row 0 flips Stop → Track when the snapshot goes idle, which arms the ignore window. Adjust is exempt, so the next axis click works (tested). A click on the options list within 500 ms of the flip is swallowed, as on Status before.
- **Arrived logic in `tick()`:** it is already suppressed on Adjust (`UiController.cpp:614`), so STOPPED cannot be overwritten there.
- **Axis cycle after a stop:** unchanged, and it reads only `axisFitted`. `_stepSel[_axis]` cannot go out of range, because `_axis` always comes from `kAxisOrder` and the index is kept modulo 5.
- **Fault page:** if a fault arrives on Adjust while the axes decelerate, a click stops and keeps `_screen` = Adjust with `_faultPage` still on top. The next click clears the page and goes to Status, the same as before. Turns are blocked by `_faultPage`.
- **Back vs Main menu rows:** Back means "up one level" (Adjust) and Main menu jumps to the menu. Both work, and long press equals Back.
- **Turning during a running move:** `stepMoveBase` returns the target while the stepper runs, so detents add onto the current target and are not lost. Moves run at the axis's `setMaxSpeed`/acceleration from configure.
- **`kItems` (9):** holds the 6 option rows.

### Out of scope, noted

- **Unused texts on dev already:** `TextId::Speed`, `TurnLeftWest`, `TurnRightEast` and `Moving` (under `// Jog`, `UiText.hpp:58-62`) have no uses on dev either. This branch did not cause them; delete them in a later cleanup.
- **Move duration:** at the design's ALT top step of 300000 steps and 3200 µsteps/s, one detent is a ≈94 s move. AZ is ≈47 s. This is by design, but worth knowing on the bench.

## Cannot verify

- clang-format v12. No binary is available. The only format workflow is upstream's `firmware/.github/workflows/code_format.yml`, which GitHub does not run from a subfolder, and it pins v18.
- The gate result (taken from the implementer's report, not re-run).
- The bench effect of DEC spreadCycle and the slower ALT.
- The ST7920 rendering of the new row and header.
