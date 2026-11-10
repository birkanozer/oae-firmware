# dwin-ui Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The Ender 3 V2 screen and knob on the Robin E3 show mount status and drive setup,
jog, Track here, GoTo and Sync, with EN/TR text and a night theme.

**Architecture:** Pure UI logic in `firmware/src/core/ui/` (host-tested, no Arduino): DWIN frame
encoder, text table, field editor, sky math, UI controller. Thin STM32 adapters in
`firmware/src/adapters/` connect it to USART3, the knob, the beeper and the Mount API.

**Tech Stack:** C++17 (gnu++17, -Werror -Wall -Wextra -Wpedantic -Wshadow), ETL no-STL, googletest
on `[env:native]`, STM32duino core 4.30000.0 on `[env:robin_e3]`.

**Spec:** [2026-11-10-dwin-ui-design.md](2026-11-10-dwin-ui-design.md); pins in
[docs/wiring.md](../../docs/wiring.md).

## Global Constraints

- Paths relative to `firmware/` unless they start with `/` (= repo root). Gate: `bash tools/gate.sh`
  from the repo root, must pass at the end of every task.
- **No GPL code.** Do not open, copy or paraphrase Marlin, Petrel, mriscoc, sezgynus or any other
  GPL DWIN code. Protocol facts are in the design doc; nothing else is needed.
- `src/core/ui/` follows `src/core/README.md`: standard C++ only, no Arduino, no `String`, no
  heap after construction, no feature `#if`. Fixed-size buffers.
- Upstream edits outside new files: guarded `defined(ARDUINO_ARCH_STM32)` / `USE_DWIN_UI`,
  small, no reformatting. New files in clang-format style (Allman, 4 spaces, 140 cols).
- DWIN frame: header `0xAA`, tail `0xCC 0x33 0xC3 0x3C`, words big-endian, colours RGB565.
  Handshake TX `AA 00 CC 33 C3 3C`, reply `AA 00 4F 4B CC 33 C3 3C`. Clear `AA 01 c c`. Rect
  `AA 05 mode c c x0 x0 y0 y0 x1 x1 y1 y1` (mode 0 outline, 1 fill). String `AA 11 mode fg fg bg
  bg x x y y text…` (mode bit6 = draw background, bits3-0 font 0–9, bit7 width-adjust = 0).
  Backlight `AA 30 level`. Orientation `AA 34 5A A5 dir`. Flush `AA 3D`.
- Screen 480×272. Fonts used: 1 (8×16), 2 (10×20), 3 (12×24), 5 (16×32); never 6–9.
- Colours: night theme fg `0xA000` (dim red), accent `0xF800`, bg `0x0000`; day theme fg
  `0xFFFF`, accent `0xFD20`, bg `0x2104`.
- Knob: long press ≥ 700 ms = Back; jog auto-stop 300 ms after the last detent; `ENCODER_COUNTS_PER_DETENT`
  default 4, `ENCODER_INVERT` default 0.
- Redraw throttle: ≥ 200 ms between field refreshes when idle, ≥ 500 ms while slewing; screen
  change redraws at once. Ping every 1000 ms, lost after 3000 ms without reply, full redraw on
  recovery.
- GoTo refuses: target altitude < minimum altitude (default 0°, range −10…+30), or separation
  from the Sun < 15°. GoTo / Sync / Track here disabled until `Mount::isTimeSet()`.
- Robin E3 pins: USART3 TX PB10 / RX PB11 115200 (`-D ENABLE_HWSERIAL3`); knob A PB0, B PA15;
  click PC10; beeper PB13. Timers TIM6 (stepper) and TIM7 (TMC SoftwareSerial) are taken.
- Turkish text without diacritics (ASCII only).
- Commits: Conventional Commits, trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  root `/CHANGELOG.md` `## [Unreleased]` gets one bullet per task.

## Review Focus

- A text field containing bytes ≥ 0x80 or the tail sequence must not reach the screen raw
  (Task 1 `TextSanitized`).
- A knob spun during a blocking `park()` must not queue a burst of jog starts afterwards
  (Task 4 `StaleTurnsDropped`: turns older than 300 ms when `tick` runs are discarded in Jog).
- Losing the screen mid-slew must never stop or start motion; only the knob does (Task 5
  adapter review).
- Editors must not allow out-of-range values to be confirmed (Task 2 boundary tests).
- GoTo with time unset or target below horizon / near Sun emits no `Goto` action (Task 4).

---

### Task 1: DWIN frame encoder

**Files:**
- Create: `src/core/ui/DwinFrame.hpp`, `src/core/ui/DwinFrame.cpp`,
  `unit_tests/test_core/ui/test_dwin_frame.cpp`

**Interfaces:**
- Produces: `struct ByteSink { virtual void write(const uint8_t *data, size_t len) = 0; protected: ~ByteSink() = default; };`
- Produces: `class DwinFrame` — `explicit DwinFrame(ByteSink &sink)`; `void handshake()`;
  `void clear(uint16_t color)`; `void rect(uint8_t mode, uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)`;
  `void text(uint8_t font, uint16_t fg, uint16_t bg, uint16_t x, uint16_t y, const char *s, uint8_t width)`
  (always draws background; pads with spaces to `width` chars, truncates beyond; max width 48);
  `void backlight(uint8_t level)`; `void orientation(uint8_t dir)`; `void flush()`;
  `static bool isHandshakeReply(const uint8_t *buf, size_t len)`.

- [ ] **Step 1: Failing tests** (fake sink collecting bytes): `HandshakeBytes`
  (`AA 00 CC 33 C3 3C`); `ClearBytes` (`clear(0x001F)` → `AA 01 00 1F CC 33 C3 3C`);
  `RectFillBytes` (`rect(1,0x07E0,64,64,256,256)` → `AA 05 01 07 E0 00 40 00 40 01 00 01 00 CC 33 C3 3C`);
  `TextBytes` (`text(2,0xFFFF,0x0000,0,32,"OK",4)` → `AA 11 42 FF FF 00 00 00 00 00 20 'O' 'K' ' ' ' ' CC 33 C3 3C`);
  `TextTruncates` (width 3, "ABCDE" → "ABC"); `TextSanitized` (bytes 0x80, 0x0A and the
  sequence `CC 33 C3 3C` inside the string come out as `?`); `BacklightBytes` (`AA 30 80 …`);
  `OrientationBytes` (`AA 34 5A A5 01 …`); `FlushBytes` (`AA 3D …`);
  `HandshakeReplyRecognised` (true for the exact reply, false for one byte off / short buffer).
- [ ] **Step 2:** Run the gate. Expected: FAIL (header missing).
- [ ] **Step 3:** Implement. Build each frame in a local `uint8_t buf[64]`, one `sink.write` per
  frame.
- [ ] **Step 4:** Gate. Expected: PASS.
- [ ] **Step 5:** Commit `feat(ui): dwin t5uic1 frame encoder`.

### Task 2: Text table and field editor

**Files:**
- Create: `src/core/ui/UiText.hpp`, `src/core/ui/UiText.cpp`, `src/core/ui/FieldEditor.hpp`,
  `src/core/ui/FieldEditor.cpp`, `unit_tests/test_core/ui/test_ui_text.cpp`,
  `unit_tests/test_core/ui/test_field_editor.cpp`

**Interfaces:**
- Produces: `enum class Lang : uint8_t { EN, TR };` `enum class TextId : uint8_t { … };`
  `const char *uiText(TextId id, Lang lang);` — every id has both strings, ASCII only, ≤ 20 chars.
  Ids cover every label the screens in the design need (status labels, states, menu items,
  setup items, settings items, warnings: SetDateTime, BelowHorizon, NearSun, Refused, Stop).
  TR examples: Track here = "Burayi takip et", Settings = "Ayarlar", Stop = "DUR",
  Set date & time = "Tarih/saat ayarla".
- Produces: `enum class FieldKind : uint8_t { Time, RA, Dec, Latitude, Longitude, Date, Int };`
  `class FieldEditor` — `void begin(FieldKind kind, int32_t value, int32_t min = 0, int32_t max = 0)`
  (value units: seconds for Time/RA (0…86399); arc-seconds for Dec (±324000), Latitude
  (±324000, minutes resolution), Longitude (±648000, minutes resolution); Date as
  `yyyy*10000+mm*100+dd` (2000-01-01…2099-12-31, day clamped to month length incl. leap years);
  Int with min/max); `void turn(int8_t detents)` (changes the selected group, wraps within the
  group's range, carries nothing); `bool click()` (next group; returns true when the last group
  was confirmed); `int32_t value() const`; `uint8_t group() const`; `void format(char *out, size_t len) const`
  (`"12:34:56"`, `"+12*34:56"`, `"+41*00"`, `"-074*00"`, `"2026-11-10"`, `"+3"`) with the
  selected group marked by the caller from `group()`. Sign is its own group for Dec/Lat/Lon/Int.

- [ ] **Step 1: Failing tests:** `AllTextIdsHaveBothLanguages` (non-null, ASCII 0x20–0x7E,
  length ≤ 20, for every id); `TimeWrapsWithinGroup` (59 s + 1 → 0 s, minutes unchanged);
  `RaMaxIs235959`; `DecSignGroupFlips`; `DecClampsAt90` (cannot confirm > +90°00′00″);
  `LatitudeClampsAt90`; `LongitudeClampsAt180`; `DateFeb29LeapYear` (2028-02-29 ok, 2026-02-29
  → day clamped to 28); `IntRespectsMinMax` (UTC offset −12…+14); `ClickReturnsTrueOnLastGroup`;
  `FormatShapes` (each kind's string above).
- [ ] **Step 2:** Gate. Expected: FAIL.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** Gate. Expected: PASS.
- [ ] **Step 5:** Commit `feat(ui): en/tr text table and field editor`.

### Task 3: Sky math and target check

**Files:**
- Create: `src/core/ui/SkyMath.hpp`, `src/core/ui/SkyMath.cpp`,
  `unit_tests/test_core/ui/test_sky_math.cpp`
- Reuse: existing `src/core/CalendarMath.hpp` / `SiderealClock.hpp` for Julian day if they
  expose it (read first; do not duplicate).

**Interfaces:**
- Produces: `struct AltAz { float altDeg; float azDeg; };`
  `AltAz equatorialToHorizontal(float haHours, float decDeg, float latDeg);` (az from north,
  eastward, 0…360);
  `void sunRaDec(int year, int month, int day, float utcHours, float &raHours, float &decDeg);`
  (low-precision solar position, ±1°);
  `float angularSeparationDeg(float ra1Hours, float dec1Deg, float ra2Hours, float dec2Deg);`
  `enum class TargetCheck : uint8_t { Ok, BelowHorizon, NearSun };`
  `TargetCheck checkTarget(float raHours, float decDeg, float lstHours, float latDeg, float minAltDeg, int year, int month, int day, float utcHours);`
  (Sun limit 15°; NearSun wins over BelowHorizon).

- [ ] **Step 1: Failing tests** (tolerances in brackets): `ZenithIsAlt90` (ha 0, dec = lat → alt
  90 [0.01]); `PoleAltEqualsLatitude` (dec 90, lat 41 → alt 41, az 0 [0.01]); `EastHorizon`
  (dec 0, ha −6 h, lat 0 → alt 0, az 90 [0.1]); `SunAtMarchEquinox2026` (2026-03-20 14:46 UTC →
  RA ≈ 0.0 h, Dec ≈ 0.0° [1°]); `SunAtJuneSolstice2026` (2026-06-21 08:24 UTC → Dec ≈ +23.44°
  [1°]); `SeparationPoles` (90° between pole and equator); `CheckRefusesNearSun`;
  `CheckRefusesBelowMinAlt`; `CheckOk`.
- [ ] **Step 2:** Gate. Expected: FAIL.
- [ ] **Step 3:** Implement (float math, `<cmath>`).
- [ ] **Step 4:** Gate. Expected: PASS.
- [ ] **Step 5:** Commit `feat(ui): alt/az, sun position and goto target check`.

### Task 4: UI controller

**Files:**
- Create: `src/core/ui/UiController.hpp`, `src/core/ui/UiController.cpp`,
  `unit_tests/test_core/ui/test_ui_controller.cpp`

**Interfaces:**
- Consumes: `DwinFrame`, `ByteSink` (Task 1); `uiText`, `Lang`, `FieldEditor` (Task 2);
  `equatorialToHorizontal`, `checkTarget` (Task 3).
- Produces:
  ```cpp
  enum class MountState : uint8_t { Idle, Tracking, Slewing, Parking, Parked, Homing };
  struct MountSnapshot {
      int32_t raSeconds; int32_t decArcSec;          // current, celestial
      int32_t lstSeconds; int32_t localSeconds;      // 0..86399
      int16_t year; int8_t month; int8_t day; int8_t utcOffsetHours;
      int32_t latArcSec; int32_t lonArcSec;
      MountState state; uint8_t slewRate; bool timeSet;
  };
  struct UiAction {
      enum class Kind : uint8_t { Stop, TrackOn, TrackOff, JogStart, JogStop, SetSlewRate, Goto, Sync,
                                  SetDate, SetTime, SetUtcOffset, SetLatitude, SetLongitude,
                                  GoHome, SetHome, Park, SetBrightness, Beep, BeepError };
      Kind kind; int32_t a; int32_t b;               // Jog: a = direction bits (N1 E2 S4 W8); Goto/Sync: a = raSeconds, b = decArcSec;
  };                                                 // SetDate: a = yyyymmdd; SetTime/SetLatitude/SetLongitude/SetUtcOffset/SetBrightness/SetSlewRate: a = value
  class UiController {
  public:
      explicit UiController(DwinFrame &screen);
      void onTurn(int8_t detents, uint32_t nowMs);
      void onClick(uint32_t nowMs);
      void onLongPress(uint32_t nowMs);
      void tick(uint32_t nowMs, const MountSnapshot &m);   // render (throttled), jog auto-stop
      void forceRedraw();
      bool popAction(UiAction &out);                       // FIFO, capacity 8, drops oldest on overflow
  };
  ```
  Settings held inside the controller (RAM, slice 2): language (EN), theme (night), brightness
  (8 of 1–10 → backlight `level*25`), minimum altitude (0°). Brightness change emits
  `SetBrightness` and re-sends backlight.

- [ ] **Step 1: Failing tests** (fake sink; helper `sawText("…")` searches emitted 0x11 frames):
  `BootDrawsStatusScreen` (clear + RA/Dec labels); `ClickOpensMenu`; `MenuFirstItemIsStopWhenMoving`
  (state Slewing → first menu text = Stop, click → `Stop` action); `LongPressGoesBack`;
  `TurnScrollsMenuAndWraps`; `TrackHereDisabledUntilTimeSet` (timeSet false → selecting it
  emits `BeepError`, no `TrackOn`); `JogStartsAndAutoStops` (turn +1 in RA → `JogStart` with
  E/W bit; tick at +301 ms → `JogStop`); `StaleTurnsDropped` (turns stamped > 300 ms before
  `tick` start no jog); `JogClickCyclesRate` (1→2→3→4→1, `SetSlewRate`); `GotoRefusedBelowHorizon`
  (no `Goto`, warning text shown); `GotoRefusedNearSun`; `GotoEmitsAction` (valid target →
  `Goto` with exact seconds/arc-seconds); `SyncEmitsAction`; `SetupDateTimeEmitsActions`;
  `LanguageSwitchRedrawsInTurkish` (after switching, `sawText("Ayarlar")`); `RedrawThrottled`
  (two ticks 50 ms apart with changed RA → one RA text frame; 500 ms while Slewing);
  `UnchangedFieldNotResent`; `ActionQueueDropsOldest`.
- [ ] **Step 2:** Gate. Expected: FAIL.
- [ ] **Step 3:** Implement. Layout constants in the .cpp (status: labels font 2, values font 3,
  two columns; menu: font 3 rows, highlighted row drawn as a filled accent rectangle with bg-
  coloured text). Keep the controller one class; split a screen into its own file only if the
  .cpp passes ~600 lines.
- [ ] **Step 4:** Gate. Expected: PASS.
- [ ] **Step 5:** Commit `feat(ui): screen controller for status, menu, jog, editors`.

### Task 5: STM32 adapters and wiring

**Files:**
- Create: `src/adapters/DwinScreen.hpp/.cpp`, `src/adapters/Knob.hpp/.cpp`,
  `src/adapters/UiMountAdapter.hpp/.cpp` (all `#if USE_DWIN_UI == 1`)
- Modify: `src/Mount.hpp/.cpp` (`bool isTimeSet() const`, set true in `setLocalStartTime`),
  `src/f_serial.hpp` (`serialLoop()`: call `dwinUiUpdate()` after `mount.loop()` when
  `USE_DWIN_UI == 1`), `src/b_setup.hpp` (init after Serial/bridge begin), `Configuration_adv.hpp`
  (`USE_DWIN_UI` default 0, `ENCODER_COUNTS_PER_DETENT` 4, `ENCODER_INVERT` 0,
  `DWIN_SEND_ORIENTATION` 0, `DWIN_SEND_FLUSH` 0), `ConfigurationValidation.hpp` (`USE_DWIN_UI`
  only on `BOARD_STM32_MKS_ROBIN_E3`), `boards/STM32_MKS_ROBIN_E3/pins_MKS_ROBIN_E3.hpp`
  (`DWIN_SERIAL_PORT Serial3`, `DWIN_BAUDRATE 115200`, `KNOB_A_PIN PB0`, `KNOB_B_PIN PA15`,
  `KNOB_CLICK_PIN PC10`, `BEEPER_PIN PB13`; remove those from the "reserved" comment),
  `Configuration_local_examples/Configuration_local_robin_e3.hpp` (`USE_DWIN_UI 1`),
  `platformio.ini` robin_e3 (`-D ENABLE_HWSERIAL3`, `-D TIMER_TONE=<free timer>`).

**Interfaces:**
- Consumes: `UiController`, `MountSnapshot`, `UiAction`, `DwinFrame`, `ByteSink` (Tasks 1–4).
- Produces: `void dwinUiSetup(); void dwinUiUpdate();` (free functions in the adapter, called
  from setup and serialLoop).
- `DwinScreen : ByteSink` — writes to `DWIN_SERIAL_PORT`; `update(nowMs)` sends a handshake
  every 1000 ms, parses replies with `DwinFrame::isHandshakeReply`, reports lost after 3000 ms
  and calls `forceRedraw()` when it recovers; sends orientation/flush only when their flags are 1.
  Never blocks: if `availableForWrite()` is too small for a frame, skip this tick.
- `Knob` — `attachInterrupt` CHANGE on A and B with a 16-entry quadrature table into a
  `volatile int16_t` count (ISR touches only that); `int8_t takeDetents()` divides by
  `ENCODER_COUNTS_PER_DETENT`, applies `ENCODER_INVERT`, keeps the remainder; click polled with
  30 ms debounce, `onClick` on release < 700 ms, `onLongPress` once at 700 ms; beeper via
  `tone(BEEPER_PIN, 2000, 30)`, error = two tones.
- `UiMountAdapter` — snapshot from `currentRA()`, `currentDEC()` (celestial via
  `getCelestialDegrees`), `calculateLst()`, `getLocalTime()`, `getLocalDate()`,
  `getLocalUtcOffset()`, `latitude()`, `longitude()`, `is*()` getters, `getSlewRate()`,
  `isTimeSet()`. Actions map exactly to the LX200 handlers' calls: Stop → `stopSlewing(ALL_DIRECTIONS | TRACKING)`;
  TrackOn/Off → `startSlewing/stopSlewing(TRACKING)`; JogStart/Stop → `startSlewing(dir)` /
  `stopSlewing(dir)`; SetSlewRate → `setSlewRate`; Goto → set `targetRA()` / `targetDEC()`
  (`Declination::fromCelestialSeconds`) then `startSlewingToTarget()`; Sync → `syncPosition`;
  SetDate → `setLocalStartDate`; SetTime → `setLocalStartTime`; SetUtcOffset →
  `setLocalUtcOffset(-hours)` (mount stores the inverse, as the `:SG` handler does);
  SetLatitude/SetLongitude → `Latitude v; v.addSeconds(...)` pattern as the `:St`/`:Sg`
  handlers; GoHome → `startSlewingToHome()`; SetHome → `setHome(false)`; Park → `park()`;
  Beep/BeepError → knob beeper; SetBrightness → screen backlight.

- [ ] **Step 1:** Read `TIMER_TONE` default and the EXTI / Serial3 pin mapping in the installed
  core's F103RC variant; choose `TIMER_TONE` (not TIM6/TIM7; TIM8 preferred) and note it in the
  report.
- [ ] **Step 2:** Implement adapters, Mount flag, hooks, config, validation, pins, env flags.
- [ ] **Step 3:** Gate. Expected: native PASS, robin_e3 SUCCESS; record RAM/flash (must stay
  < 50 % RAM, < 60 % flash).
- [ ] **Step 4:** Commit `feat(firmware): ender 3 v2 screen and knob on robin e3`.

### Task 6: Docs

**Files:**
- Modify: `/docs/wiring.md` (screen section: final pins, timers incl. `TIMER_TONE`, EXTI),
  `/README.md` (short "Onboard screen" section: knob = scroll, click = select, long press = back,
  jog auto-stop, GoTo locked until date/time set), `/BACKLOG.md` (Next: "Slice 2 bench test
  (needs PSU + screen cable)" with the design doc's checklist; Later: persist screen settings
  in slice 3), `/docs/architecture.md` §3a (night theme, settings persistence moves to slice 3).

- [ ] **Step 1:** Update the four files.
- [ ] **Step 2:** Gate. Expected: pass.
- [ ] **Step 3:** Commit `docs: onboard screen usage, wiring and bench checklist`.
