# Patch: seconds in the clock (approved in chat 2026-11-28)

1. **Time editor gets a seconds field.** `Edit::Time` (Settings/Setup > Time, and wherever else `openEditor(Edit::Time, ...)`
   is called, e.g. the Rebooted / date-time-first flows) uses the existing `FieldKind::Time` (hh:mm:ss, seconds
   editable) instead of `FieldKind::TimeHM`. `firmware/src/core/ui/UiScreens.cpp:55` and the comment "S1c: hh:mm,
   seconds 00 at the click". The confirmed value carries the edited seconds into `SetTime` (today seconds are 0).
   Check the editor's sub-labels (`kHms`, hours/minutes/seconds) and the editor screen layout already handle 3 groups
   for this edit, as they do for RA.
2. **Setup's Time row** shows `hh:mm:ss` (`UiScreens.cpp:~251`, `FieldKind::TimeHM` → the Time format), placeholder
   `--:--:--` when the time is not set.
3. **Screensaver clock** (`UiController.cpp` `renderSaver`, ~line 896) shows `hh:mm:ss` in F10x20, placeholder
   `--:--:--`; check it still centers within 128 px (8 chars x 10 px = 80 px).
4. If `FieldKind::TimeHM` has no remaining user after this, remove it and its cases in `FieldEditor.cpp/.hpp`
   (and tests that only exercised it). Keep anything still used.
5. Tests (native, test-first) in `firmware/unit_tests/test_core/ui/`: time editor has three groups and seconds reach
   `SetTime`; Setup Time row and screensaver show seconds and the `--:--:--` placeholder. Update existing tests that
   assumed hh:mm.

Rules: clang-format v12 style (col 140), `-Werror`, LCD text ASCII. Gate `bash tools/gate.sh` from the repo root
must pass. Commit `feat(ui): seconds in the time editor, setup row and screensaver clock` with trailer
`Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. No CHANGELOG/README edits (controller does docs).
