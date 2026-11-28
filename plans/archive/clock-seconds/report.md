# Report: seconds in the clock

Commit: feat(ui): seconds in the time editor, setup row and screensaver clock (branch fix/clock-seconds)

## Changes
- UiScreens.cpp: Edit::Time uses FieldKind::Time (hh:mm:ss, 3 groups); Setup Time row formats with FieldKind::Time, placeholder `--:--:--`;
  renderEdit drops the n=2 / FootAt00 override (kHms already has 3 labels, default footer "click: save").
- UiController.cpp renderSaver: `%02d:%02d:%02d`, placeholder `--:--:--` (8 chars x 10 px = 80 px; violations stay 0).
- Removed now-dead code: FieldKind::TimeHM (enum, begin(), format(), header comment), TextId::FootAt00 and its "click at :00 sec" table entry (EN/TR).
- Tests: test_ui_flows (FastPathDateThenTime now edits seconds -> SetTime 23:41:08; TimeSecondsZeroAtClick -> TimeSecondsEdited with
  seconds 55 reaching SetTime; new SetupTimeRowShowsSeconds incl. placeholder; click counts +1 in DatePrefilledFromLastSaved,
  ClearsSyncConfirmBeforeTimeChange, AbandonedSureNeverRidesAlong), test_ui_controller (saver shows 23:41:07, placeholder `--:--:--` on the saver,
  no violations), test_field_editor (TimeHM test removed).

## Tests
RED (`bash tools/gate.sh` native): 559 test cases: 4 failed (FastPathDateThenTime, TimeSecondsEdited, SetupTimeRowShowsSeconds, SaverAfter120sIdleOnly).
GREEN: `bash tools/gate.sh` -> 557 test cases: 557 succeeded; robin_e3 build SUCCESS (RAM 16.8%, Flash 63.8%).

## Concerns
- clang-format not installed locally; formatting done by hand (one wrapped line in renderSaver), col 140 checked.
- Trailer follows the brief (Claude Opus 5.5) rather than the session's default attribution line.
- Footer of the time editor is now "click: save"; the "click at :00 sec" hint is gone (no longer true).

## Review follow-up (Minor fixes)
- renderSaver now uses the existing `hms()` helper instead of the duplicated snprintf (forced wrap gone).
- TimeSecondsEdited also asserts the "3/3" group counter on the seconds step.
- Gate: 557/557 passed, robin_e3 build SUCCESS. Commit: refactor(ui): saver clock uses hms.
