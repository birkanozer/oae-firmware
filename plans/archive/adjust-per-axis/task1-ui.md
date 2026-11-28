# Task 1: Adjust page UI fixes and per-axis step sizes

Spec: `plans/adjust-per-axis/design.md` § UI (items 1-3). Use its values verbatim.

## Code

- `firmware/src/core/ui/UiController.cpp` / `.hpp`: `stop()`, `holdScreen()`, `clickScreen()` (Adjust axis cycle),
  `applyTurn`, `tick()` jog expiry, `row0Label()`, `moving()`.
- `firmware/src/core/ui/UiScreens.cpp`: `kStepSizes`, `kJogFirst`, `clickAdjustOptions`, `turnAdjust`, `stepText`,
  `renderAdjust`, the AdjustOptions `items()` list and its renderer, `optionLock` (row numbers shift if needed).
- `firmware/src/core/ui/UiText.hpp` / `UiText.cpp`: add `Main menu` / `Ana menü`; remove texts only the removed
  jog path used (`JogWord`, jog speed names) if nothing else uses them.
- `firmware/src/adapters/UiMountAdapter.cpp`: leave `JogStart`/`JogStop`/`SetSlewRate` handling if other code
  (LX200 path, other screens) still uses those action kinds; remove only what becomes dead in the UI.
- Tests: `firmware/unit_tests/test_core/ui/` (existing UiController tests). Update tests that exercised Adjust
  jog sizes; add tests for each item below.

## Requirements

1. Long press on `Screen::AdjustOptions` → `Screen::Adjust`. New last row "Main menu" in AdjustOptions; click
   opens the main menu (`openMenu()`). The row is never locked.
2. `stop()` keeps `_screen` when it is `Screen::Adjust`; otherwise Status as today. Covers click-while-moving and
   hold-while-moving on Adjust. The STOPPED transient and BeepLong unchanged. Check `renderAdjust` still shows
   the STOPPED header (or whatever the header rendering does on Adjust) and the M2 ignore window still
   behaves (the next axis click after a stop on Adjust must not be swallowed wrongly; current code exempts
   Adjust from M2).
3. Per-axis step size: `_stepSel` becomes one index per axis id (0 RA, 1 DEC, 2 AZ, 3 Focus, 4 ALT), default 2.
   Step lists per axis exactly as in the design. Step size option cycles 0..4 for the current axis only.
   Remove the jog sizes, `_jogDir`/`_jogMs`/`jogStop()` and everything only they need, so `moving()` and
   `row0Label()` no longer reference a UI jog. Keep `MountState::Jogging` and the mount-side jog.
   Adjust text shows the step count and its degrees as today (uses `_m.stepsPerDeg[axis]`).
   Check `MoveSteps` payload type holds `detents * 300000` for |detents| ≤ 127 without overflow.
4. clang-format v12 style (`firmware/.clang-format`), `-Werror`: build must stay warning-free.

## Gate

`bash tools/gate.sh` from the repo root (native tests, then `robin_e3` build). Both must pass. Run focused
tests while iterating: `cd firmware && ../.venv/Scripts/pio.exe test -e native -f test_core` with
`.tools/mingw64/bin` on PATH (see `tools/gate.sh`).

## Out of scope

Motor speed/current config (Task 2 runs in parallel on `Configuration_local_examples/Configuration_local_robin_e3.hpp`;
do not touch that file). No CHANGELOG/BACKLOG/README edits: the controller does the docs pass.
