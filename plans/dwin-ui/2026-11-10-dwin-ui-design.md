# dwin-ui — design

Slice 2 of [docs/architecture.md](../../docs/architecture.md) §3a/§8. The Ender 3 V2 screen
(DWIN T5UIC1, 480×272, knob + click + beeper) becomes a standalone controller on the Robin E3:
see where the mount points, set it up, jog it, aim it by hand and track, or GoTo / Sync typed
coordinates — no phone needed.

## Scope

In:
- DWIN driver written from the DWIN T5UIC1 development guide (facts only; no GPL code read or
  copied). Frames `AA <cmd> <data> CC 33 C3 3C`, big-endian words, RGB565. Commands used:
  handshake 0x00 (reply `AA 00 4F 4B CC 33 C3 3C`), clear 0x01, rectangle 0x05 (outline/fill),
  string 0x11 (ASCII only, fonts 0–9), backlight 0x30. Unconfirmed in DWIN docs, bench-tested:
  orientation 0x34 (`AA 34 5A A5 dir`) and flush 0x3D — sent only if the bench shows they are
  needed (config flags, default off).
- Knob: quadrature on EXTI interrupts (A PB0, B PA15), counts per detent and direction set by
  config; click on PC10 (polled, debounced); long press ≥ 700 ms = Back. Beeper on PB13: short
  tick on click, double beep on refused action.
- Screens (graphical, drawn with text + rectangles, no stock icons):
  - **Status** (home): RA, Dec, Alt, Az, LST, local time + date, site, state (Idle / Tracking /
    Slewing / Parked / …), one warning line ("Set date & time", "Below horizon", …).
  - **Menu** (knob scrolls, click selects, long press back). First item is **STOP** whenever
    anything moves. Items: Track here / Stop tracking · Jog · GoTo RA/Dec · Sync RA/Dec ·
    Setup (date, time, UTC offset, latitude, longitude) · Home (go home, set home here, park) ·
    Settings (language EN/TR, brightness, theme day/night, minimum altitude) · Back.
  - **Jog**: pick RA or DEC; each detent (re)starts a slew in that direction, the slew stops
    300 ms after the last detent (auto-stop, never runs away); click cycles slew rate 1–4.
  - **Editors** for typed values: hh:mm:ss (time, RA), ±dd°mm′ss″ (Dec), ±dd°mm′ (latitude),
    ±ddd°mm′ (longitude), yyyy-mm-dd, integers (UTC offset −12…+14, brightness 1–10, minimum
    altitude −10…+30°). Knob changes the selected group, click moves to the next group, click
    on the last group confirms, long press cancels.
- **Safety gates in the UI** (the mount itself never refuses a GoTo):
  - GoTo / Sync / Track here are disabled until date and time were set since this boot (from
    the screen or over LX200 — a new `Mount::isTimeSet()` flag set by `setLocalStartTime`).
  - GoTo refuses a target below the minimum altitude (default 0°) or within 15° of the Sun
    (low-precision Sun position, ±1°), with a message on the status line.
- **Language** EN / TR (Turkish without diacritics — the screen fonts are ASCII), **night
  theme** (dim red on black) as default, day theme (white on dark grey).
- Redraw only changed fields, values padded with background; ≤ 5 Hz when idle, ≤ 2 Hz while
  slewing (keeps the UART and the main loop free for LX200). Handshake ping every 1 s; no reply
  for 3 s = screen lost; when it answers again, full redraw (the panel can reset on its own).

Out (later):
- Persisting screen settings (language, brightness, theme, minimum altitude) → slice 3
  `runtime-config` (settings struct in EEPROM). In slice 2 they reset to defaults at boot.
  Latitude, longitude and UTC offset already persist through the mount.
- Homing on microswitches (slice 4); built-in object list (backlog); motor/gear settings
  (slice 3).

## Architecture

Pure logic in `src/core/ui/` (no Arduino, host-tested under `[env:native]`); hardware glue in
`src/adapters/`; the Mount stays the only authority.

```
 knob ISR ─┐                                        ┌─► DwinFrame ─► ByteSink ─► Serial3 (USART3)
 click ────┼─► UiController (core) ── UiAction ─────┤
 millis ───┘        ▲  MountSnapshot                └─► UiMountAdapter ─► Mount API
                    └──────────── UiMountAdapter ◄──────── Mount getters
```

- `DwinFrame` — encodes DWIN frames into a `ByteSink`. Rejects non-printable bytes (replaced by
  `?`) so text can never form GB2312 pairs or a fake frame tail.
- `UiText` — EN/TR string table.
- `FieldEditor` — the typed-value editor.
- `SkyMath` — RA/Dec + LST + latitude → Alt/Az; low-precision Sun RA/Dec from date/time;
  angular separation; `checkTarget()` → ok / below horizon / near Sun.
- `UiController` — screens, knob events in, `UiAction`s out (small queue), renders through
  `DwinFrame` with dirty tracking and the redraw throttle; input is a `MountSnapshot` struct.
- Adapters (STM32, guarded): `DwinScreen` (Serial3 + handshake/ping/reconnect), `Knob`
  (EXTI quadrature, click debounce, beeper via `tone()` on a timer that is not TIM6/TIM7),
  `UiMountAdapter` (fills `MountSnapshot`, applies actions with the same Mount calls the LX200
  handlers use), one `ui.update()` call in `serialLoop()` after `mount.loop()`.

Gotchas from the code map: setting date/time/longitude/UTC offset re-zeros the RA reference
(`autoCalcHa`) — the Setup menu says "set before Sync"; `park()` and `setHome()` block until the
axes stop — called from `update()`, never from an ISR; `Declination` is an axis coordinate —
use the celestial helpers (`fromCelestialSeconds`, `getCelestialDegrees`) for display and entry.

## Pins and timers (robin_e3)

USART3 TX PB10 / RX PB11 at 115200 (`-D ENABLE_HWSERIAL3`); knob A PB0, B PA15, click PC10,
beeper PB13 (docs/wiring.md). Stepper TIM6, TMC SoftwareSerial TIM7 — `tone()` must use another
timer (`TIMER_TONE`, chosen after reading the variant; TIM8 preferred). EXTI priority above the
stepper timer.

## Risks

- **Screen clones** (DACAI, TJC…) may lack fonts 6–9 or behave differently: use fonts ≤ 5.
- **Orientation / flush commands** unconfirmed: config flags, bench decides.
- **Encoder counts per detent** unknown: config value, default 4, bench-tuned.
- **5 V budget**: screen backlight on the board's 5 V buck (3 A) with the ESP32 later — fine on
  paper; measure on the bench.

## Bench checklist (needs PSU + adapter cable)

1. Multimeter the screen cable pins 1/2 before connecting (docs/wiring.md).
2. Boot: status screen appears; if not, swap TX/RX (pins 3/4); if rotated, enable orientation.
3. If text appears only partly or late, enable the flush flag.
4. Knob: one detent = one menu step, right direction; else set counts/invert.
5. Jog RA and DEC with the knob; release → stops within ~0.3 s.
6. Set date/time on the screen → GoTo unlocks; GoTo a star; Track here; Stop.
7. Unplug and replug the screen while running → redraws within ~3 s.
