# Patch: Set home here = active axis only; home confirms return to Adjust (approved in chat 2026-11-28)

## UI (`firmware/src/core/ui/`)

1. **Row label names the axis.** The Adjust options row "Set home here" becomes "Set <AXIS> home" for the axis selected
   on Adjust (`_axis`: 0 RA, 1 DEC, 2 AZ, 3 Focus, 4 ALT), e.g. "Set DEC home"; TR "<AXIS> evini ayarla", e.g.
   "DEC evini ayarla". Focus uses the existing focus axis name the Adjust page shows. ASCII only. Build the text
   from the axis name the Adjust page already uses plus one format text per language (fewest lines; no five
   separate TextIds unless the text system forces it). Check it fits the options list width (128 px, F6x10).
2. **Confirm names the axis too:** "Set <AXIS> home?" / "<AXIS> evini ayarla?". "Set home all?" unchanged.
3. **Both confirms return to Adjust.** After a confirmed Set home (axis) or Set home all: push the action, show
   `HdrHomeSet` transient, `_screen = Screen::Adjust` (today: Status). The idle re-check failing at the click still
   beeps and goes back to the options (unchanged). `renderAdjust` header today shows only the `HdrStopped`
   transient: let it show `HdrHomeSet` too (other transients still keep the ADJUST header).
4. Lock unchanged: both rows only while idle (`MsgStopFirst`).
5. The `SetHome` action carries the axis id (use the existing action payload field, as `MoveSteps` carries its axis).

## Mount / adapter

6. `UiMountAdapter.cpp` `Kind::SetHome` → new guarded (`#if BOARD == BOARD_STM32_MKS_ROBIN_E3`) Mount method, e.g.
   `setHomeAxis(axis)`, next to `setHomeHere()`/`setHomeAll()` in `firmware/src/Mount.cpp/.hpp`:
   - RA: zero `_stepperRA` and `_stepperTRK`; RA sky reference as `setHome()`/`applyPendingHomeReference()` do for RA
     (`_zeroPosRA = LST + 6 h` when date and time are set, else pending), `_targetRA = currentRA()`. DEC untouched.
   - DEC: zero `_stepperDEC` and `_stepperGUIDE`, `_zeroPosDEC = 0`. RA untouched.
   - AZ, ALT: zero and store to EEPROM the same way `setAZALTHome()` does, but only that axis (split or parameterise
     it with the fewest lines; keep `setAZALTHome()` behaviour for Set home all and the LX200 path).
   - Focus: zero `_stepperFocus` (RAM only).
   Stop/wait/tracking handling as `setHome()` does for the axis being zeroed (the UI only offers it while idle).
   Read `setHome()`, `setHomeHere()`, `applyPendingHomeReference()`, `_homeReferencePending` end to end first: the
   pending-reference logic must stay right when only RA or only DEC is set. If setting RA or DEC alone cannot be
   made consistent with the sky reference (e.g. the pending flag covers both), stop and report.
   `setHomeHere()` stays (LX200 `MeadeCommandProcessor.cpp:317` and `setHomeAll()` use it).
7. `_synced = false` in the adapter only when RA or DEC was set (AZ/ALT/Focus do not touch the sky sync).

## Tests (native, test-first) in `firmware/unit_tests/test_core/ui/`

Row label per axis (RA, DEC, AZ, ALT, Focus) in EN and TR; confirm text; confirm pushes `SetHome` with the axis id;
both confirms land on Adjust with HOME SET in the header; idle re-check failure → options; Set home all unchanged
apart from returning to Adjust. Update existing tests that expected Status.

## Gate and commit

`bash tools/gate.sh` from the repo root (robin_e3 compiles the Mount/adapter part). clang-format v12 style, col 140,
`-Werror`. Commit `fix(ui): set home sets the selected axis, home confirms return to adjust` with trailer
`Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. No CHANGELOG/README edits (controller does docs).

## Addendum (user, same day): long press on Adjust = main menu; options via an Options stop and an Axis row

This supersedes item 3's "return to Adjust" target axis and the earlier Main menu row (v0.7.0).

8. **Long press on the Adjust page opens the main menu** (`openMenu()`), on every axis page and on the Options stop.
9. **Options stop.** Click on Adjust still cycles the fitted axes in display order (RA, DEC, AZ, ALT, Focus), and
   after the last fitted axis comes one more stop, **Options** (TR "Secenekler"), then back to RA. On the Options stop
   the Adjust page shows "OPTIONS" where the axis and position are (keep it simple, same layout/fonts), footer
   "click: options" (TR ASCII); turning does nothing there (no move, no beep); click opens the options list.
10. **Axis row.** The options list starts with a row **Axis: <AXIS>** (TR "Eksen: <AXIS>"); click cycles the fitted
    axes (not the Options stop). Its initial value when the options open: the axis last moved by a turn on Adjust
    since boot (RA if none). Step size and "Set <AXIS> home" act on the Axis row's axis (label and confirm follow it).
    Rows: Axis, Step size, Sync on target, Set <AXIS> home, Set home all, Go home, then the automatic Back.
    **Remove the Main menu row** (and its text if unused).
11. **Leaving the options** (long press, Back row, either home confirm) returns to the Adjust page **on the Axis row's
    axis** (not the Options stop), so the next turn moves that axis. Sync on target and Go home keep going to Status
    as today. Long press on a confirm screen still returns to the options.
12. `optionLock`, click and hold handlers and the comment listing the rows stay consistent with the new indices.
13. Docs are the controller's job, but update any UI text tables and tests that referred to the Main menu row.

Tests (add to the list above): long press on an axis page and on the Options stop → main menu; click cycle includes
the Options stop once, after the last fitted axis, for a config without AZ/ALT/Focus too; turning on the Options stop
pushes nothing; Axis row preset to the last moved axis (RA when none) and cycles fitted axes only; Step size and the
home row follow the Axis row; leaving the options lands on Adjust on the Axis row's axis.
