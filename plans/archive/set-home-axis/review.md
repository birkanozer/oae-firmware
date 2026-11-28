# Review: fix/set-home-axis (bcbefcb) vs dev

Reviewer re-ran `bash tools/gate.sh` at bcbefcb: native 565/565 passed, robin_e3 SUCCESS (RAM 8280 B, flash 167 900 B).
The robin_e3 config fits AZ, ALT and Focus (`firmware/Configuration_local_examples/Configuration_local_robin_e3.hpp:25-27`).
So the build only covers the "all fitted" case. The compiled-out guard paths are checked by reading only.

## Verdict 1: spec compliance

| # | Item | Status |
|---|------|--------|
| 1 | Row label "Set <AXIS> home" / "<AXIS> evini ayarla" from the Adjust axis names (`kAxisNames`, Focus = `AxisFocus`), one format text per language | Met. `UiScreens.cpp:279-280`, `UiText.cpp` "Set %s home". Longest is "ODAK evini ayarla" at 17 cols, within the 19-col menu row limit. |
| 2 | Confirm "Set <AXIS> home?" / "<AXIS> evini ayarla?"; "Set home all?" unchanged | Met. `UiController.cpp:714-719` builds the text + "?". "ODAK evini ayarla?" is 18 cols, exactly the F7x13B limit (126 px), and AllScreensFit covers it. |
| 3 | Both confirms land on Adjust with the HOME SET header; a failed idle re-check goes back to the options | Met. `UiController.cpp:331-342`, `UiScreens.cpp:660-662`. |
| 4 | Lock unchanged (idle only) | Met. `optionLock` rows 3 and 4. |
| 5 | `SetHome` carries the axis in `b` | Met. `UiController.cpp:339`. |
| 6 | Guarded `Mount::setHomeAxis` with RA, DEC, AZ/ALT, Focus semantics; `setHomeHere` kept | Met. `Mount.cpp:3874-3924`, inside the `#if BOARD == BOARD_STM32_MKS_ROBIN_E3` block (3850-3947). AZ/ALT duplicate the 8 lines of `setAZALTHome` instead of splitting it. That keeps the upstream function untouched, which is acceptable. |
| 7 | Adapter clears `_synced` only for RA/DEC | Met. `UiMountAdapter.cpp:193-197`. |
| 8 | Long press on Adjust (every axis page and the stop) opens the main menu | Met. `UiController.cpp:430-434`. Test: `AdjustLongPressOpensMainMenuOnEveryStop`. |
| 9 | Options stop after the last fitted axis, OPTIONS shown, footer "click: options", turn inert, click opens the options | **Changed** (two interpretation calls, see below). |
| 10 | Axis row first, starts on the last moved axis (RA if none), cycles fitted axes only; Step and Set home follow it; row order; Main menu row and its text removed | Met. |
| 11 | Leaving the options (hold, Back, either confirm) lands on Adjust on the Axis row axis; Sync and Go home go to Status; hold on a confirm goes back to the options | Met. |
| 12 | `optionLock`, click/hold handlers and the row comments match the new indices | Met. `UiScreens.cpp:343-382`, `UiController.hpp:144`. |
| 13 | Text tables and tests updated for the removed Main menu row | Met. Three entries removed and three added at matching positions; the static_assert at `UiText.cpp:186` holds. |

Changed or extra (for the controller to accept or bounce):

- **Item 9, "then back to RA".** The brief says the cycle goes "... Options, then back to RA". It also says a click on the stop opens the options. The implementation follows the second: no click takes you from the stop to RA. You leave the stop through the options (and land on the Axis row axis) or with a long press. The brief contradicts itself here and this reading is reasonable. Confirm it with the user.
- **Item 9, "OPTIONS where the axis and position are".** OPTIONS is shown only at the header's right edge, where the axis name sits. The body (Pos line, step line, axis strip) is blank, leaving a mostly empty page. Acceptable under "keep it simple", but it is a reading of the brief, not its letter.
- **Extra: Adjust footer** changed to "click:axis hold:menu" / "tikla:eksen tut:menu". Item 8 implies this. Fine.
- **Extra: `_zeroPosDEC = 0.0f` deleted** from `applyPendingHomeReference()` (`Mount.cpp:3935`). Safe, see Verdict 2.
- **Extra: `renderSmallConfirm` takes `const char *`.** Needed for the built confirm text. Fine.

## Verdict 2: correctness

### Important

None.

I checked one candidate and dropped it. `moveStepperBy()` clears `_homeReferencePending` (`Mount.cpp:4205`, pre-existing). That looked like it could lose RA's pending reference in the sequence "Set RA home with no clock → jog DEC → enter date and time". It cannot:
- `setLocalStartDate`, `setLocalStartTime`, `setLocalUtcOffset` and `setLongitude` (1434-1441, 4618-4654) all call `autoCalcHa()` → `setHA()` → `setLST()` (1406-1414).
- `setLST()` sets `_zeroPosRA = LST + 6 h` unconditionally on every clock change (upstream behaviour).
- So the RA reference is retaken whenever the clock is set, whatever the pending flag says. The flag only adds the `_targetRA = currentRA()` refresh.
- Upshot: the pending logic is redundant for `_zeroPosRA` but harmless. Only RA-only and DEC-only homes go through it, and both stay correct.

### Minor

**M1. `UiScreens.cpp:660-662`: Turkish header overlap on the Options stop.**
- Scenario: in TR, confirm "Set <last fitted axis> home" (for example ODAK, or DEC on a mount with no AZ/ALT/Focus). The page lands on that axis with "EV AYARLANDI" in the header for 2 s (`kSyncedMs`). Clicking once moves to the stop, where the header becomes "EV AYARLANDI" (12 cols) + "SECENEKLER" (10 cols).
- That is 22 columns on a 21-column header: the two texts overlap by about 6 px, and by about 12 px when the "!" warning is shown.
- FakeCanvas would count this as a violation, but no test reaches it.
- Fix: show the own transient only when `!_stop`, or draw no right-hand text while a transient is shown on the stop.

**M2. `Mount.cpp:3905, 3908, 3911, 3913, 3916, 3919`: nested `#if`/`#endif` at column 0.**
- The directives sit inside the `#if BOARD == ...` block, and `firmware/.clang-format:88` sets `IndentPPDirectives: BeforeHash`, so they should be indented 4 spaces.
- The removed `setHomeAll` lines had that indent (`    #if FOCUS_STEPPER_TYPE`), as does `UiMountAdapter.cpp:31`.
- The gate does not run clang-format; this is judged by convention, not by running the formatter.
- Fix: indent the six lines by 4 spaces.

**M3. `Mount.cpp:3880-3903`: RA and DEC cases skip `_slewingToHome = false; _slewingToPark = false;`, which `setHome()` does (3842-3843).**
- Not reachable from the UI: the only caller is the idle-only UI path, and STOP → `stopAll()` → `cancelPark()` already clears both flags (2202-2207).
- Note only. Add the two lines to case 0 if you want exact `setHome` parity.

**M4. Half-cleared sync.** An RA-only home keeps a DEC sync offset in `_zeroPosDEC`, and a DEC-only home keeps the RA sync in `_zeroPosRA`. Meanwhile the adapter sets `_synced = false` (`UiMountAdapter.cpp:195-196`).
- So the status reads "not synced" while half of the sync offset still applies.
- This follows the brief ("DEC untouched" / "RA untouched"). Flagged so the user knows; no fix asked.

**M5. Test gaps** (no test asserts nothing; these are missing cases):
- Turning on the Options stop *while moving* (Slewing or Jogging) is untested. By reading it is inert: `applyTurn` (`UiController.cpp:240`) lets Adjust turns through while moving, and `turnAdjust` returns on `_stop` (`UiScreens.cpp:418`).
- The M1 TR overlap scenario is untested.
- `Mount::setHomeAxis` has no test (Mount is not in the native build).
- Nit: in `SetHomeNeedsIdle` the final violations sum omits `u`.

### Questions from the brief, answered

- **RA zeroed while DEC is off home.** `currentRA()` = `_zeroPosRA` − RA steps in hours, plus 12 h when the DEC counter (with `_zeroPosDEC`) is below 0 (`Mount.cpp:1490-1511`).
  - After an RA home, RA reads LST+6 h, or LST+18 h if the DEC counter is on the far side of the pole. DEC reads what it read before.
  - That means "the RA shaft is at its mechanical home", which is geometrically right as long as the DEC counter is right. This matches the brief's intent.
  - After a DEC-only home, DEC reads the pole, and RA may jump 12 h if the DEC counter changed sign. At the pole that is expected.
- **Deleted `_zeroPosDEC = 0` in `applyPendingHomeReference()`.** The only writers of `_zeroPosDEC` are the constructor (105), `syncPosition` (1588, which clears pending at 1540), `setHome` (3834, sets 0) and `setHomeAxis(1)` (3900, sets 0).
  - While pending is true after `setHomeHere()` with no clock, `_zeroPosDEC` is already 0 (via `setHome`), so the line was redundant there.
  - After an RA-only home it would have wrongly dropped DEC's sync offset. The deletion is safe and correct.
- **Tracking restart and TRK zeroing, and LX200.** `setHomeAxis` is called only from the adapter (`UiMountAdapter.cpp:194`) and from `setHomeAll` (3870, case 3, Focus only). LX200 `:SHP#` still uses `setHomeHere()` (`MeadeCommandProcessor.cpp:317`).
  - The UI re-checks idle at the click, so the `wasTracking` branch is reachable only if the state changes between the click and the adapter's apply.
  - That branch mirrors `setHome()` (3818-3825, 3838). `stopSlewing(EAST|WEST)` never stops tracking, so `startSlewing(TRACKING)` re-arms a TRK stepper that is still running, the same as upstream.
  - `addHours(6)` is unconditional here, whereas `setHome` wraps it in `#if defined(OAM) || defined(OAE)`. That matches the existing `applyPendingHomeReference()`. The robin_e3 config defines `OAE` (`Configuration_local_examples/Configuration_local_robin_e3.hpp:14`), so "Set RA home" and "Set home all" agree.
- **EEPROM.** The AZ and ALT cases make the same `setCurrentPosition(0)` and `EEPROMStore::storeAZPosition(0)` / `storeALTPosition(0)` calls as `setAZALTHome()` (2101-2111). Same format.
- **Guards.** With AZ, ALT or Focus absent, each case reduces to `case N: break;`. That compiles cleanly under `-Werror` (by reading; the robin_e3 build fits all three). The `Mount.hpp:371` declaration is inside the board guard.
- **UI state.**
  - `_stop` is reset on Menu → Adjust (`UiController.cpp:387`) and when the stop opens the options (323). Every other way into Adjust comes from the options or a confirm, where `_stop` is already false.
  - STOP keeps the Adjust page and `_stop` (155). On the stop a click while moving is STOP (201), and the footer shows the STOP bar (`footer()` 753).
  - The fault page (click → Status) and the reboot screen lead back through Status/Menu, which resets `_stop`. The screensaver does not change `_screen`: waking returns to the same page with `_stop` kept, which is fine.
  - The step index stays per axis (`_stepSel[_axis]`).
  - `_moved` is set only after a move is actually pushed (`UiScreens.cpp:427`), so a move refused for a fault leaves it alone. `AdjustAltMovesAxis4AndItsFaultLocksOnlyAlt` covers this indirectly.
  - Axis row cycling uses `nextAxis(true)` and skips unfitted axes.
  - Widths: "Set FOCUS home" 14 cols, "Axis: FOCUS" and "Eksen: ODAK" 11 cols. All fit; M1 is the only width problem.

## Cannot verify

- Hardware behaviour of `setHomeAxis` (no native Mount test).
- clang-format v12 conformance (no formatter binary in the repo tools).
- The compiled-out AZ/ALT/Focus builds (robin_e3 fits all three).
