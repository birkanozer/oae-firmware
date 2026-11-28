# Task 3 review: Set home all (commits f0b1c12 + 6070b35)

Reviewed: brief, report, package diff, and the surrounding code (UiController.cpp, UiScreens.cpp, UiMountAdapter.cpp,
Mount.cpp/.hpp). Nothing edited or committed.

## Verdict 1: spec compliance

| Requirement | Result |
|---|---|
| Row "Set home all" / TR "Tumunu eve ayarla" (ASCII), right after Set home here | Met. UiText.cpp kTable and UiText.hpp enum entries are in the same position; UiScreens.cpp:277. |
| Row order Step size, Sync on target, Set home here, Set home all, Go home, Main menu, Back | Met. UiScreens.cpp:274-284. |
| Same idle-only lock, MsgStopFirst | Met. optionLock cases 2 and 3 share one body (UiScreens.cpp:348-350); the dot on the row uses the same call. |
| Confirm "Set home all?" / "Tumunu eve ayarla?", click = yes with idle re-check, HdrHomeSet + Status, long press = options | Met. The existing SetHomeConfirm screen and renderSmallConfirm are reused (UiController.cpp:331-342, 423-425, 717); the question text switches on `_homeAll`. |
| Effect: setHomeHere + AZ/ALT zeroed and stored via setAZALTHome() + Focus zeroed, each under its STEPPER_TYPE guard, no motor move | Met. Mount.cpp:3865-3874. setAZALTHome (Mount.cpp:2101) has the AZ and ALT guards and the EEPROM stores itself; Focus is guarded in setHomeAll. |
| One `UiAction::Kind::SetHomeAll`, handled in UiMountAdapter | Met (UiMountAdapter.cpp:197-200, `_synced = false` like SetHome). |
| Mount change guarded `#if BOARD == BOARD_STM32_MKS_ROBIN_E3` | Met. The definition (Mount.cpp:3865) is inside the board block that starts at 3850. The declaration (Mount.hpp:370) is inside the block at Mount.hpp:366-374. Both are under the same guard. |
| Comment at UiScreens.cpp:339 kept true | Met (now 340-341, lists the 6 rows). |
| Tests: row order, lock, confirm -> action + HOME SET + Status, long press -> options, idle re-check; shifted tests updated | Met. SetHomeAllRowConfirmAndLock covers all five; the three shifted tests are updated, none deleted. |
| clang-format v12 | Appears met. `IndentPPDirectives: BeforeHash` indents the nested `#if FOCUS...` inside the board block by 4, which matches. No formatter ran (reported). |
| Gate | Reported 557/557 tests and robin_e3 build SUCCESS. Not re-run here (review only). |

Changed versus the brief, with a coordinator ruling: the brief says "RAM only, no EEPROM write". The ruling replaced this
with Mount::setAZALTHome(), which stores 0 to EEPROM for AZ and ALT. Focus stays RAM only. The implemented behaviour
is consistent with the ruling. The brief text is now stale, and the README line at README.md:65 ("also zeroes AZ, ALT and Focus")
does not say AZ and ALT are stored (see Minor 2).

Extras: none. No Configuration_local_examples change, no CHANGELOG/README/BACKLOG edits in these two commits.

Spec: met.

## Verdict 2: correctness and quality

### Checked and clean
- `_homeAll` staleness: `Screen::SetHomeConfirm` has exactly one entry point, UiScreens.cpp:372-373, and it assigns `_homeAll = r == 3`
  immediately before entering. No other code assigns `_screen = SetHomeConfirm` (grep over firmware/src/core/ui). So the scenario
  "open Set home all confirm, long press back, then Set home here confirms as all" cannot happen: row 2 rewrites the flag to false.
  The fault, stop and reboot paths do not go through that screen, and nothing restores it from `_ret`.
  The test SetHomeHereConfirmStillSetsHomeHere exercises this exact sequence. The flag outlives the screen like `_pointSync`, but is safe.
- Row indices: items() (274-279), optionLock (346-352), clickAdjustOptions (360-382) and the tests agree: 2 here, 3 all, 4 Go home, default
  (5) is Main menu. clickList's Back is `n - 1`, so it follows automatically. The `showMessage(optionLock(_sel))` at UiScreens.cpp:318 uses
  the shifted index correctly. Go home's fault lock moved to case 4 together with its click.
- Mount guards: setAZALTHome is declared unguarded (Mount.hpp:309) and defined unguarded upstream; with AZ and ALT both absent it compiles to an empty
  function, so there is no warning or unused-symbol problem. `_stepperFocus` is only used under the FOCUS guard. The robin_e3 build passed.
- No other consumer of `UiAction::Kind` needs the new value (only UiMountAdapter switches on it; the stale hits are in `.claude/worktrees/driver-check`).
- Tests assert real things: the SetHomeAll-and-not-SetHome check, the lock beep, the re-check at the click, and the violations count. None are vacuous.

### Findings

Minor 1. firmware/src/Mount.cpp:3865-3874 (and :3873): `setHomeAll()` returns `bool` that is always true and `setHomeHere()`'s result is
discarded. Failure scenario: none today. It only mirrors setHomeHere. Fix: make it `void` (Mount.hpp:370 too) unless parity with setHomeHere is wanted,
and ignore this if the parity is deliberate.

Minor 2. firmware/src/Mount.hpp:370 and README.md:65: no comment on the declaration (setHomeHere has a 2-line contract above it), and the user
docs say "also zeroes AZ, ALT and Focus" without saying the AZ and ALT zero is stored in EEPROM while Focus is not. Scenario: after
reboot AZ and ALT come back as 0 (stored) but the Focus counter starts at 50000 again (Mount.cpp:416), which can surprise a bench tester. Fix: one
comment line in Mount.hpp ("AZ/ALT stored, Focus RAM only") and the same in the controller's docs pass. Also update the stale "RAM only" line in
plans/adjust-per-axis/task3-set-home-all.md if it is kept as a record.

Minor 3. firmware/src/Mount.cpp:3869-3871: zeroing AZ, ALT or Focus while one is still running. The UI `Idle` state only covers RA and DEC; an
Adjust move on AZ, ALT or Focus can still be in progress. AccelStepper's setCurrentPosition sets the target to 0 and speed to 0, so the axis halts
abruptly (no ramp) and the move is lost; the upstream stop-time store at Mount.cpp:3376 may then store 0 again, which is harmless. Failure
scenario: user taps a long Focus or AZ step, immediately does Set home all; the motor stops dead and may lose a step or two of holding position (no
position error, since the counter is zeroed at the stop point). Fix: not needed for correctness; if wanted, `stopSlewing`/wait for AZ, ALT and
Focus before zeroing, as setHome() does for RA/DEC. Accepting the risk is reasonable because the axis is also at 0 afterwards by definition.

Minor 4. firmware/unit_tests: Mount::setHomeAll and the adapter case have no native test (Mount and the adapter are not built natively); coverage of
the effect is the robin_e3 compile only. Not a defect of this change (same gap as SetHome), so no action is needed beyond the bench test.

No Critical or Important findings.

Quality: approved (minor notes only).
