# Task 3 report: Set home all

Status: DONE_WITH_CONCERNS. Commit f0b1c12 `feat(ui): set home all in the adjust options`.

## Changes
- UiText: `SetHomeAll` ("Set home all" / "Tumunu eve ayarla"), `SetHomeAllQ` ("Set home all?" / "Tumunu eve ayarla?").
- UiController: `UiAction::Kind::SetHomeAll`, `_homeAll` flag (like `_pointSync`). The existing `SetHomeConfirm` screen and
  `renderSmallConfirm` are reused; click pushes `SetHomeAll` or `SetHome`, idle re-check, HOME SET and Status unchanged.
- UiScreens: row 3 Set home all (same lock as Set home here, `optionLock` case 2/3), Go home is row 4, Main menu row 5; comment
  updated. Row 2 and 3 click set `_homeAll = r == 3`.
- UiMountAdapter: `Kind::SetHomeAll` calls `Mount::setHomeAll()` and clears `_synced`.
- Mount.hpp/.cpp (guarded `BOARD_STM32_MKS_ROBIN_E3`): `setHomeAll()` = `setHomeHere()` + `setCurrentPosition(0)` on
  `_stepperAZ` / `_stepperALT` / `_stepperFocus` under their `*_STEPPER_TYPE != NONE` guards. RAM only.
- Tests (test_ui_flows.cpp): shifted indices in AdjustLongPressOpensOptions, AdjustOptionsMainMenuRow (EN + TR),
  GoHomeAllowedFromTracking; new SetHomeAllRowConfirmAndLock (row order/TR text, lock while tracking, confirm ->
  SetHomeAll not SetHome + HOME SET + Status, long press -> options, idle re-check at the click) and
  SetHomeHereConfirmStillSetsHomeHere. Written first (red = compile error on `Kind::SetHomeAll`), then green.

## Gate
`bash tools/gate.sh`: 557/557 native tests, robin_e3 build SUCCESS (flash 63.8%, RAM 16.8%). No clang-format binary
available here; formatted by hand to the file's style, comment lines kept <= 140 cols.

## Concerns
1. AZ and ALT positions are persisted by upstream to EEPROM whenever they stop (`Mount.cpp` ~3376). Zeroing in RAM only (as
   specced) means a reboot restores the old EEPROM AZ/ALT value until the next AZ/ALT move. `Mount::setAZALTHome()` already
   exists and does zero + EEPROM store; using it instead would make Set home all survive a reboot. Not done (spec: no EEPROM write).
2. Focus counter starts at 50000 at boot (`Mount.cpp:416`, not persisted); zeroing to 0 is fine, there are no focus limits.
3. AZ/ALT/Focus have no limit or home bookkeeping beyond the counters (no limit variables found), so zeroing is safe.
4. Test used `x.click(600)` for the idle re-check case because a click 10 ms after a state change is swallowed by the existing
   click-ignore window; the re-check itself is what is exercised.

## Follow-up (coordinator ruling on concern 1)
`Mount::setHomeAll()` now calls `setHomeHere()`, `setAZALTHome()` and zeroes Focus (RAM only). `setAZALTHome()` read end to end: zeroes
AZ/ALT under their type guards and stores 0 to EEPROM; no motor move, no waiting, no RA/DEC. So AZ/ALT home survives a reboot like
upstream's stop-time store. Concern 1 is resolved. Commit: `fix(ui): set home all stores az/alt home`. Gate: 557/557, robin_e3 SUCCESS.
