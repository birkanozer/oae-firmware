# lcd-ui — design (supersedes the DWIN parts of 2026-11-10-dwin-ui-design.md)

The user's screen is the **stock Ender 3 LCD (ENH12864Z-1)**: ST7920, 128×64 monochrome, light
pixels on a blue backlight, rotary encoder with click, buzzer. It plugs with its stock cable
from its **EXP3** connector into the Robin E3's **"Ender-3 EXP1"** header (Marlin `EXP3_*`,
Klipper `CR_EXP3`). Functions, safety rules and the slice scope of the DWIN design still apply;
this file replaces the display, driver, layout and interaction parts.

**Layout spec of record:** `mockups/screens-12864-v2.html` (40 screens, pixel-exact at 128×64)
with the behaviour rules in `mockups/screen-audit.md` (Must/Should items as applied in v2's
"Changes from v1" box). Where a mockup and this file disagree, this file wins.

## Hardware (Ender-3 header)

| Function | Pin | Notes |
|---|---|---|
| LCD CS (ST7920 RS) | PA5 | software SPI — the pins do not line up with SPI1 |
| LCD SCK (E) | PA6 | |
| LCD data (SID / RW) | PA4 | |
| Encoder A / B | PB11 / PB0 | EXTI, internal pull-ups; swap via `ENCODER_INVERT` |
| Click | PC3 | active low, pull-up |
| Buzzer | PC1 | driven with `tone()` (works for active and passive buzzers) |

Driver: **U8g2** (BSD-2-Clause), `U8G2_ST7920_128X64_F_SW_SPI(U8G2_R0, PA6, PA4, PA5)`, full
frame buffer (1 KB RAM). Fonts (X11 misc-fixed, public domain): `u8g2_font_6x10_tf` (main),
`u8g2_font_7x13B_tf`, `u8g2_font_10x20_tf` (editors), `u8g2_font_5x7_tf` (rare). The `_tf`
variants carry `°`. A full `sendBuffer()` takes tens of ms in software SPI, so the frame is
sent only when something changed, at most 4 Hz idle and 2 Hz while slewing (steppers run in the
timer ISR and are unaffected; LX200 waits at most one frame).

USART3 is no longer used by the screen. The two ULN2003 groups move off EXP1:
group A PB13, PB14, PB15, PA15 (EXP2); group B PA7, PC4, PC5, PC10 (EXP1/EXP2). PA15 and PB3/PB4
still need JTAG off (already done).

## Interaction rules (from the audit, all binding)

- **Any input stops motion (M1).** While the mount slews, jogs, parks or homes — including motion
  started over LX200 — any click or long press on any screen issues STOP (stops all motors incl.
  tracking), long beep, header STOPPED for 2 s. The footer of every screen shows `click: STOP`
  while moving. Turning the knob while moving does nothing except on the Jog screen.
- **Menu row 0 is a fixed motion slot (M2):** Track here (idle) / Stop tracking (tracking) /
  STOP (moving); other rows never move. Clicks are ignored for 500 ms after row 0's label changes.
  The menu always opens at row 0. Unavailable rows stay in place, drawn locked (dotted underline +
  short tag); clicking one gives a double beep and a 2 s footer message.
- **Unlock rule (M6):** GoTo, Sync, Sync on target and Track here need **both date and time**
  set since this boot (`Mount::isDateSet()` + `isTimeSet()`). The status alert names what is
  missing; clicking it goes straight to Date then Time (S1a). Date prefills from the last saved
  date (EEPROM) or the build date (S1b); time edits as hh:mm, seconds 00 at the click (S1c).
- **Editors:** turn = change group, click = next group, **hold = previous group** (hold on the
  first group = cancel) (S3). Encoder acceleration: detents < 40 ms apart count ×5 (S1d).
  Latitude/longitude show N/S and E/W letters as the last group (TR K/G, D/B) (S10).
- **GoTo:** RA → Dec editors prefilled with the last target → confirm screen with Alt, Az and Sun
  distance; GO is armed 600 ms after the screen appears (S4). Refused below minimum altitude or
  within 15° of the Sun → refused screen, click returns to the editor with values kept (S9).
  Arrival: header ARRIVED 3 s, then TRACKING.
- **Sync:** confirm screen with the offset; offset > 5° adds a "BIG JUMP - CHECK" frame (M5);
  afterwards header SYNCED 2 s. **Sync on target** (S2) prefills with the last GoTo target. A date,
  time, UTC offset or longitude change after a Sync asks "Clears Sync. Sure?" first (S5).
- **Jog (M4):** Jog submenu = Jog RA, Jog DEC, Speed. Jog screen: turn moves, slew stops 300 ms
  after the last detent, turns older than 300 ms are dropped; click toggles RA/DEC; hold returns
  to the submenu. Speed cycles the firmware's four slew rates, labelled **SLOW / CENTER / FIND /
  MAX** (OAT rates 1–4 are fractions of max speed, not sidereal multiples — ruling over the
  mockup's x1/x8/x64 labels).
- **Home / Park:** Go home, Set home here (confirm), Park, Unpark (only when parked; when parked,
  Unpark is the only motion row). Park and home are **started, then polled** (M7): a PARKING /
  HOMING screen with STOP footer, the UI loop never blocks.
- **Status (S7):** header state + clock, RA, Dec, HA, Alt/Az with °, LST + UTC; while slewing,
  the target on row 4 instead of LST. Transient headers: STOPPED, ARRIVED, SYNCED, HOME SET.
- **Fault page (S14):** shown on any screen when a driver fails its UART check at boot or the
  firmware detects an unplanned reset; motors stopped; the only inverted header. After an
  unplanned reset, date/time count as unset (GoTo locked again) with "REBOOTED - SET TIME".
- **Saver (M3, S17):** after 2 min without input, only while idle or tracking; never while moving
  (leaves at once if motion starts); shows warnings; the first turn/click only wakes.
- **Settings** rows come from a small table (id, EN/TR label, type, range, default) so slice 3
  can swap in the firmware-wide settings table that the phone app also renders: Language
  (EN/TR), Min altitude (−10…+30°, default 0), Beep (on/off). RAM-only until slice 3.
- **Text:** every drawn string is width-checked in tests (menu rows ≤ 19 chars, header state
  ≤ 12 beside the clock, full lines ≤ 21, editors ≤ 12). Turkish wording per v2 (e.g. "Motorla
  kaydir" for Jog, "Yks" for altitude, "tikla: DURDUR").

## Architecture

`src/core/ui/` (pure, host-tested): `Canvas` interface (text with font id + inverted flag, box,
frame, dotted rule, clear) and font metrics; `UiText`, `FieldEditor`, `SkyMath` (done);
`UiController` (screens, input, actions, rendering to `Canvas`). `src/adapters/` (STM32):
`U8g2Canvas` + frame throttle, `Knob` (EXTI quadrature, acceleration, click, beeper),
`UiMountAdapter` (snapshot ↔ Mount, non-blocking park/home), `lcdUiSetup()/lcdUiUpdate()` hooked
like slice 2's plan. The DWIN encoder from task 1 is removed.

## Bench checklist (screen on the Ender-3 header)

1. Boot: status screen appears; if the screen stays blank or garbled, check the cable key and
   try the slower U8g2 bus delay (`U8G2_ST7920_..._SW_SPI` clock); the ST7920 at 3.3 V logic is
   the known risk.
2. Knob: one detent = one step in the right direction (else `ENCODER_INVERT` / counts per detent).
3. Buzzer beeps on click.
4. Fast path: click the alert → set date and time → GoTo unlocks.
5. Jog RA/DEC; let go → stops within ~0.3 s; click while moving anywhere → STOP.
6. GoTo a star → confirm → ARRIVED → TRACKING; Sync on target.
7. Park → PARKING screen → click → STOP works mid-park.
8. Time `canvas.flush()` (one frame send, e.g. `micros()` around the call, print over USB): each
   send holds up `loop()` (software SPI), so the stepper ISR keeps running but the main loop,
   serial and guiding stall for that long. Check it stays short (a few ms) while slewing and
   jogging.
