# lcd-ui Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The stock Ender 3 LCD (ST7920 128×64) and its knob on the Robin E3 run the full
standalone UI from the v2 mockups: status, setup fast path, jog, GoTo/Sync with safety gates,
home/park, settings, fault and saver screens, EN/TR.

**Architecture:** Pure UI in `firmware/src/core/ui/` rendering to a `Canvas` interface
(host-tested with a fake canvas); thin STM32 adapters draw through U8g2 and talk to the Mount.
Continues `2026-11-10-dwin-ui-plan.md` (its tasks 1–3 are done: DwinFrame, UiText/FieldEditor,
SkyMath); this plan replaces its tasks 4–6.

**Tech Stack:** C++17 core (googletest, `[env:native]`); STM32duino + U8g2 (BSD-2) on `[env:robin_e3]`.

**Spec:** [2026-11-10-lcd-ui-design.md](2026-11-10-lcd-ui-design.md) (binding), layout spec of
record [mockups/screens-12864-v2.html](mockups/screens-12864-v2.html), behaviour detail
[mockups/screen-audit.md](mockups/screen-audit.md).

## Global Constraints

- Paths relative to `firmware/` unless they start with `/` (= repo root). Gate `bash tools/gate.sh`
  from the repo root passes at the end of every task.
- `src/core/ui/` is standard C++ only (no Arduino, no `String`, no heap after construction, no
  feature `#if`), namespace `core`, fixed-size buffers.
- No GPL code copied (Marlin etc.). U8g2 is used as a library only.
- Screen 128×64. Fonts and cell sizes: `Font::F6x10` 6×10 (main), `Font::F7x13B` 7×13, `Font::F10x20`
  10×20, `Font::F5x7` 5×7. Width limits: menu rows ≤ 19 chars (x 2..121, scrollbar 124..127),
  header state ≤ 12 chars beside the 8-char clock, full lines ≤ 21, editors ≤ 12 big chars.
- Every interaction rule in the design's "Interaction rules" section is binding (STOP on any
  input while moving, fixed row-0 motion slot + 500 ms click ignore, locked rows, date+time
  unlock, hold = previous group, acceleration ×5 under 40 ms, GoTo confirm armed after 600 ms,
  refused screens, Sync confirm with > 5° big-jump frame, Sync on target, "Clears Sync. Sure?",
  jog auto-stop 300 ms + stale-turn drop, speeds SLOW/CENTER/FIND/MAX = rates 1–4, park/home
  started then polled, transient headers STOPPED 2 s / ARRIVED 3 s / SYNCED 2 s / HOME SET 2 s,
  fault page, saver after 120 s only idle/tracking, first input only wakes).
- Robin E3 pins: LCD CS PA5, SCK PA6, data PA4; encoder A PB11, B PB0; click PC3; buzzer PC1.
  TIM6 (stepper) and TIM7 (TMC SoftwareSerial) are taken; `tone()` uses another timer.
- Frame sent only on change, ≥ 250 ms apart idle, ≥ 500 ms while slewing.
- Commits: Conventional Commits, trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  `/CHANGELOG.md` `## [Unreleased]` one bullet per task.

## Review Focus

- A click arriving 0–500 ms after row 0 changes from Track here to STOP must not start tracking
  (Task L2 `ClickIgnoredAfterSlotChange`).
- Motion started over LX200 (state Slewing while the UI is on any screen, incl. an editor) must
  make the next click STOP (Task L2 `AnyClickStopsLx200Motion`).
- A knob spun while the UI was blocked (stale timestamps) must not jog (Task L4 `StaleTurnsDropped`).
- Every string any screen draws must fit its width limit in both languages (Task L4 `AllScreensFit`).
- A Sync/date/time change must never be applied without its confirm (Tasks L3 tests).

---

### Task L1: Canvas, editor extensions, DWIN removal

**Files:**
- Create: `src/core/ui/Canvas.hpp` (interface + font metrics), `unit_tests/test_core/ui/FakeCanvas.hpp`
  (records ops, asserts every op inside 128×64 and every text inside the screen)
- Modify: `src/core/ui/FieldEditor.hpp/.cpp` (+ tests): `bool back()` (previous group; returns
  true when called on the first group = cancel); `FieldKind::TimeHM` (hh:mm, value seconds with
  ss = 0); hemisphere-letter mode for Latitude/Longitude: `void setLetters(char pos, char neg)`
  — letter group shown last instead of a leading sign (`"41°01'N"`, `"028°58'E"`), turning it flips
  the sign.
- Delete: `src/core/ui/DwinFrame.hpp/.cpp`, `unit_tests/test_core/ui/test_dwin_frame.cpp`; update
  the CHANGELOG bullet that announced it (replace with the Canvas bullet).

**Interfaces:**
- Produces: `enum class Font : uint8_t { F5x7, F6x10, F7x13B, F10x20 };`
  `constexpr uint8_t fontWidth(Font)`, `constexpr uint8_t fontHeight(Font)` (5/7, 6/10, 7/13, 10/20);
  `struct Canvas { virtual void clear() = 0; virtual void text(Font f, int16_t x, int16_t y, const char *s, bool inverted) = 0;
  virtual void box(int16_t x0, int16_t y0, int16_t x1, int16_t y1) = 0; virtual void frame(int16_t x0, int16_t y0, int16_t x1, int16_t y1) = 0;
  virtual void dots(int16_t x0, int16_t x1, int16_t y) = 0; protected: ~Canvas() = default; };`
  (y = top of the text cell; inverted = light-on-dark inside its own cell box).

- [ ] **Step 1: Failing tests:** `BackMovesToPreviousGroup`, `BackOnFirstGroupCancels`,
  `TimeHMFormatAndSecondsZero` (`"23:41"`, value % 60 == 0), `LatitudeLettersFormat`
  (`"41°01'N"` / `"41°01'S"` for ±), `LongitudeLettersFormat` (`"028°58'E"`), `LetterGroupFlipsSign`;
  `FakeCanvasRejectsOutOfBounds` (a text ending past x = 127 fails the fake's check).
- [ ] **Step 2:** Gate → FAIL. **Step 3:** Implement; delete DWIN files. **Step 4:** Gate → PASS.
- [ ] **Step 5:** Commit `feat(ui): canvas interface, editor back/hh:mm/hemisphere letters; drop dwin`.

### Task L2: UiController core — status, menu, safety, locks, fault, saver

**Files:**
- Create: `src/core/ui/UiController.hpp/.cpp`, `unit_tests/test_core/ui/test_ui_controller.cpp`
- Modify: `src/core/ui/UiText.*` only to add ids (both languages, ASCII, ≤ 20 chars)

**Interfaces:**
- Consumes: `Canvas`, `Font` (L1); `uiText`, `Lang`, `FieldEditor` (done); `equatorialToHorizontal`,
  `checkTarget`, `TargetCheck` (done, incl. its fix round's `Invalid`).
- Produces:
  ```cpp
  enum class MountState : uint8_t { Idle, Tracking, Slewing, Jogging, Parking, Homing, Parked, Fault };
  struct MountSnapshot {
      int32_t raSeconds, decArcSec;            // current, celestial
      int32_t targetRaSeconds, targetDecArcSec; bool hasTarget;
      int32_t lstSeconds, localSeconds;        // 0..86399
      int16_t year; int8_t month, day, utcOffsetHours;
      int32_t latArcSec, lonArcSec;
      MountState state; uint8_t slewRate;      // 1..4
      bool dateSet, timeSet, rebooted, synced; uint8_t faultCode; // 0 = none
  };
  struct UiAction {
      enum class Kind : uint8_t { Stop, TrackOn, TrackOff, JogStart, JogStop, SetSlewRate, Goto, Sync,
                                  SetDate, SetTime, SetUtcOffset, SetLatitude, SetLongitude,
                                  GoHome, SetHome, Park, Unpark, Beep, BeepError, BeepLong };
      Kind kind; int32_t a; int32_t b;         // Jog a = direction bits N1 E2 S4 W8; Goto/Sync a = raSeconds, b = decArcSec;
  };                                           // SetDate a = yyyymmdd; others a = value
  class UiController {
  public:
      explicit UiController(Canvas &canvas);
      void onTurn(int8_t detents, uint32_t nowMs);
      void onClick(uint32_t nowMs);
      void onLongPress(uint32_t nowMs);
      bool tick(uint32_t nowMs, const MountSnapshot &m);   // returns true when the canvas changed
      bool popAction(UiAction &out);                       // FIFO, capacity 8, drops oldest
      void setLastSavedDate(int32_t yyyymmdd);             // prefill (S1b)
  };
  ```
  Scope of this task: status screen (all variants incl. alert bar, target row while slewing,
  transient headers), main menu with fixed row-0 slot, locked rows + footer message, STOP-on-any-
  input while moving (state Slewing/Jogging/Parking/Homing, whatever started it), 500 ms click
  ignore after slot change, fault page, rebooted state, saver rules. Menu rows other than row 0
  open placeholder screens that L3/L4 fill (a placeholder returns on hold).
- [ ] **Step 1: Failing tests** (FakeCanvas; helper `drew("text")`): `BootShowsStatusWithAlert`
  (dateSet/timeSet false → alert names both), `AlertClickOpensDateEditor`, `MenuOpensAtRow0`,
  `Row0LabelFollowsState` (Track here / Stop tracking / STOP), `ClickIgnoredAfterSlotChange`,
  `AnyClickStopsLx200Motion` (state Slewing set by snapshot while on a placeholder screen → click
  → `Stop` + `BeepLong`, header STOPPED 2 s), `LongPressAlsoStopsWhileMoving`,
  `TurnDoesNothingWhileMovingOutsideJog`, `LockedRowDoubleBeepAndMessage`, `UnlockNeedsDateAndTime`,
  `FaultPageOverridesEverything`, `RebootedLocksAgain`, `SaverAfter120sIdleOnly`,
  `SaverNeverWhileMoving`, `FirstInputOnlyWakes`, `ArrivedHeaderThenTracking`,
  `RedrawOnlyOnChange` (tick returns false when nothing changed), `ActionQueueDropsOldest`.
- [ ] **Step 2:** Gate → FAIL. **Step 3:** Implement to the v2 layouts (status, menus, fault,
  saver screens in screens-12864-v2.html). **Step 4:** Gate → PASS.
- [ ] **Step 5:** Commit `feat(ui): lcd controller core with stop-anywhere, locks, fault and saver`.

### Task L3: GoTo, Sync and Setup flows

**Files:** Modify `src/core/ui/UiController.*`, `UiText.*`, `unit_tests/test_core/ui/test_ui_controller.cpp`
(or a new `test_ui_flows.cpp`).

**Interfaces:** Consumes L2's controller; produces the GoTo, Sync, Sync on target, Setup list,
fast path (Date → TimeHM), Latitude/Longitude (letters), UTC offset, "Clears Sync. Sure?" screens.

- [ ] **Step 1: Failing tests:** `GotoEditorsPrefilledWithLastTarget`, `HoldGoesToPreviousGroup`,
  `GotoConfirmShowsAltAzSun`, `GoNotArmedBefore600ms`, `GotoEmitsActionAfterConfirm`,
  `GotoRefusedBelowHorizonKeepsValues`, `GotoRefusedNearSun`, `GotoRefusedInvalid`,
  `SyncConfirmShowsOffset`, `SyncBigJumpFrameAbove5deg`, `SyncOnTargetUsesLastTarget`,
  `SyncedHeader2s`, `FastPathDateThenTime`, `TimeSecondsZeroAtClick`, `DatePrefilledFromLastSaved`,
  `ClearsSyncConfirmBeforeTimeChange` (synced true → SetTime only after "Sure?" yes),
  `LatLonUseHemisphereLetters`, `UtcOffsetRange`.
- [ ] **Step 2:** Gate → FAIL. **Step 3:** Implement. **Step 4:** Gate → PASS.
- [ ] **Step 5:** Commit `feat(ui): goto, sync and setup flows with confirms and safety gates`.

### Task L4: Jog, Home/Park, Settings, Turkish, width lint

**Files:** Modify `src/core/ui/UiController.*`, `UiText.*`; create `src/core/ui/UiSettings.hpp/.cpp`
(row table: id, EN/TR label id, type enum/int/bool, min, max, default; values in RAM), tests.

**Interfaces:** Produces Jog submenu + jog screen, Home/Park menu (parked variant), Set home
confirm, PARKING/HOMING screens, Settings (Language, Min alt, Beep) rendered from the table;
`Lang` switch redraws everything; beep setting gates Beep actions (BeepError/BeepLong too? —
no: safety beeps stay on; only click ticks follow the setting).

- [ ] **Step 1: Failing tests:** `JogStartsAndAutoStops300ms` (RA turn → JogStart E/W; +301 ms →
  JogStop), `StaleTurnsDropped`, `JogClickTogglesAxis`, `JogHoldReturnsToSubmenu`,
  `SpeedCyclesFourRates` (SLOW→CENTER→FIND→MAX→SLOW, SetSlewRate 1..4), `ParkedShowsOnlyUnpark`,
  `SetHomeNeedsConfirm`, `ParkingScreenAndStop`, `SettingsFromTable`, `LanguageSwitchRedrawsTurkish`
  (`drew("Ayarlar")`), `MinAltUsedByGoto`, `BeepOffSilencesTicksOnly`, `AllScreensFit` (walk every
  screen in both languages with long values; FakeCanvas asserts the width limits).
- [ ] **Step 2:** Gate → FAIL. **Step 3:** Implement. **Step 4:** Gate → PASS.
- [ ] **Step 5:** Commit `feat(ui): jog, home/park, settings table and turkish`.

### Task L5: STM32 adapters and wiring

**Files:**
- Create: `src/adapters/U8g2Canvas.hpp/.cpp`, `src/adapters/Knob.hpp/.cpp`,
  `src/adapters/UiMountAdapter.hpp/.cpp`, `src/adapters/LcdUi.hpp/.cpp` (`lcdUiSetup()`,
  `lcdUiUpdate()`) — all `#if USE_LCD_UI == 1`.
- Modify: `src/Mount.hpp/.cpp` — `isDateSet()`, `isTimeSet()` (set by `setLocalStartDate` /
  `setLocalStartTime`, false at boot), `startPark()` (non-blocking: stop all, then the loop starts
  the home slew and sets PARKING when the axes have stopped — do not call the blocking `park()` /
  `waitUntilStopped()` from the UI), `setHome` called only when idle (the UI enforces it);
  `src/f_serial.hpp` (`lcdUiUpdate()` after `mount.loop()`), `src/b_setup.hpp` (setup call),
  `Configuration_adv.hpp` (`USE_LCD_UI` default 0, `ENCODER_COUNTS_PER_DETENT` 4,
  `ENCODER_INVERT` 0), `ConfigurationValidation.hpp` (`USE_LCD_UI` only on robin_e3),
  `boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp` (LCD/encoder/click/buzzer pins; drop the old
  screen reservation comment; ULN2003 group pins A PB13 PB14 PB15 PA15, B PA7 PC4 PC5 PC10 as
  defines for slice 3), `Configuration_local_examples/Configuration_local_robin_e3.hpp`
  (`USE_LCD_UI 1`), `platformio.ini` robin_e3 (`olikraus/U8g2@^2.35` pinned exact,
  `-D TIMER_TONE=<free timer>`).

**Interfaces:** Consumes everything in L1–L4. `U8g2Canvas : core::Canvas` maps fonts to
`u8g2_font_5x7_tf`, `u8g2_font_6x10_tf`, `u8g2_font_7x13B_tf`, `u8g2_font_10x20_tf`
(`setFontPosTop`), inverted text = box + XOR draw colour; `flush()` = `sendBuffer()`, called by
`lcdUiUpdate` only when `tick()` returned true and the throttle allows. `Knob`: EXTI CHANGE on
PB11/PB0, 16-entry quadrature table into a `volatile int16_t`; detents with timestamps;
acceleration ×5 when < 40 ms apart; click debounced 30 ms; long press 700 ms; buzzer `tone()`
(tick 2 kHz 20 ms, error two tones, long 400 ms). `UiMountAdapter` fills `MountSnapshot`
(celestial Dec via `getCelestialDegrees`, state from `is*()` getters, faultCode from the TMC
UART test result, `rebooted` from the reset-cause flags `RCC_CSR` (IWDG/WWDG/software/power-on
distinguishes cold boot from an unplanned reset)) and applies actions with the same Mount calls
the LX200 handlers use (see the dwin-ui plan Task 5 list; Park → `startPark()`, Unpark →
`startSlewing(TRACKING)`).

- [ ] **Step 1:** Read the variant's `TIMER_TONE` default and EXTI lines for PB11/PB0 (distinct EXTI
  lines 11 and 0 — confirm); choose a free tone timer (not TIM6/TIM7).
- [ ] **Step 2:** Implement; gate → native PASS, robin_e3 SUCCESS; RAM < 50 %, flash < 70 %.
- [ ] **Step 3:** Commit `feat(firmware): ender 3 lcd and knob ui on robin e3`.

### Task L6: Docs

- Modify `/docs/wiring.md` (Ender-3 header table: display EXP3 → board "Ender-3 EXP1"; remove the
  DWIN/USART3 screen section; ULN2003 groups moved; timers incl. TIMER_TONE), `/README.md`
  ("Onboard screen" section: knob rules, click-anywhere STOP, fast date/time path, GoTo locked
  until date+time), `/BACKLOG.md` (Next: "Slice 2 bench test" from the lcd-ui design checklist;
  Later: settings persistence in slice 3), `/docs/architecture.md` §3a (stock 12864 LCD, U8g2).
- Gate; commit `docs: ender 3 lcd usage, wiring and bench checklist`.
