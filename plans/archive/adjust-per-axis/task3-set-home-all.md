# Task 3: "Set home all" in the Adjust options

User request 2026-11-28: add "Set Home All" to the Adjust options to save all positions as home.

## Behaviour

- New Adjust options row **Set home all** (TR **Tumunu eve ayarla**, ASCII only), right after **Set home here**.
  Rows become: Step size, Sync on target, Set home here, Set home all, Go home, Main menu (then the automatic Back).
- Same lock as Set home here (only while idle; message `MsgStopFirst` otherwise) and same confirm flow: a small
  confirm screen with **Set home all?** (TR **Tumunu eve ayarla?**); click = yes, long press = back to the options.
  Re-check idle at the click like `Screen::SetHomeConfirm` does; on success show `HdrHomeSet` and go to Status,
  same as Set home here.
- Effect: everything Set home here does (`Mount::setHomeHere()`: RA/DEC counters and sky reference), plus the step
  counters of AZ, ALT and Focus set to 0 when those axes are fitted (`#if AZ_STEPPER_TYPE != STEPPER_TYPE_NONE` etc.).
  No motor moves. RAM only, like Set home here (no EEPROM write).
- Reuse: one `UiAction::Kind` (e.g. `SetHomeAll`) handled in `firmware/src/adapters/UiMountAdapter.cpp`; one Mount
  method guarded `#if BOARD == BOARD_STM32_MKS_ROBIN_E3` next to `setHomeHere()` in `firmware/src/Mount.cpp/.hpp`
  that calls `setHomeHere()` and zeroes the AZ/ALT/Focus steppers (check how AZ/ALT/Focus positions are held:
  `_stepperAZ`, `_stepperALT`, `_stepperFocus`, `focusSetStepperPosition`; respect any per-axis home/limit
  bookkeeping those axes have, e.g. limits relative to 0). Reuse the existing confirm screen with a parameter or a
  second screen value, whichever is fewer lines; do not duplicate the renderer.
- `optionLock` and the click/hold handlers shift row indices: keep them consistent and keep the comment at
  `UiScreens.cpp:339` true.

## Tests

Native UI tests in `firmware/unit_tests/test_core/ui/test_ui_flows.cpp`, test-first: row order, lock while not idle,
confirm → `SetHomeAll` action + HOME SET + Status, long press on the confirm → options, idle re-check at the click.
Existing Adjust options tests whose row indices shift must be updated, not deleted.

## Gate and commit

`bash tools/gate.sh` from the repo root must pass (robin_e3 build compiles the Mount and adapter change).
Commit `feat(ui): set home all in the adjust options` with the trailer
`Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, only your files by path.
No CHANGELOG/README/BACKLOG edits (controller does docs). Do not touch
`firmware/Configuration_local_examples/`.
