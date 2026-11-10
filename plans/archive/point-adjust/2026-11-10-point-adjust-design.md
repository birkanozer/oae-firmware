# point-adjust — design

Slice: rework the onboard LCD menus around how the user actually aligns an OAE, plus the focus motor.
Builds on v0.2.1 (LCD UI, TMC2209 UART working, AZ on the Z socket).

## Why

The Open Astro Explorer has its own alignment markers, so there are no endstops and no homing run.
The user lines the axes up with the markers **without touching the mount** (touching it shifts the
alignment): from the LCD on its long cable, or later from the phone. Today the menus split one job
across Jog, GoTo, Sync, Sync on target, Home/Park and Setup, which is confusing, and long press does
not consistently mean "back".

## Goals

1. **Point** page that merges GoTo, Sync and Sync on target.
2. **Adjust** page that merges Jog, Align and small manual corrections: step-sized moves per knob
   detent on RA, DEC, AZ and Focus, plus Set home here, Go home and Sync on target.
3. Home is mechanical and fixed: only *Set home here* changes it. Sync and Adjust never move home.
4. Adjust nudges at a target are one-off unless the user presses *Sync on target*.
5. Long press means back/cancel on every page; while the mount moves it stays STOP.
6. Setup (date, time, UTC offset, latitude, longitude) moves under Settings. Park is removed.
7. Focus motor on the E socket (bipolar 28BYJ-48, TMC2209 UART), like AZ on the Z socket.
8. The phone can do the same Adjust/Sync/Set home moves over LX200 (same Mount calls).

## Non-goals

- Backlight dimming: the user does not want a hardware mod; no Backlight row (drop it from BACKLOG).
- Motor/gear/driver settings table (next slice).
- Multi-star pointing model (one offset only).
- ALT motor; ULN2003 drive for AZ/Focus.
- Phone app UI (only the firmware commands it will use).

## Anti-goals

- Any input path that can no longer STOP a moving mount.
- A Sync or Adjust that silently moves the physical home position.
- Moving a motor whose driver failed its UART check (existing fault lock stays).

## Menu tree

```
Main
  [row 0 motion slot]   Track here / Stop tracking / STOP   (unchanged rules M1, M2)
  Point
  Adjust
  Settings
    Setup               Date, Time, UTC offset, Latitude, Longitude   (existing editors)
    Language, Min altitude, Beep, Supply 12/24 V                       (existing rows)
```

Removed: Jog submenu, GoTo, Sync, Sync on target, Home/Park (Go home, Set home, Park, Unpark),
Setup on the main menu. Unpark is no longer needed: there is no parked state from the UI.

## Point

- RA editor → Dec editor (arc-second resolution, prefilled with the last target), as today.
- Confirm screen: target, Alt, Az, Sun distance (as GoTo confirm today) and an action selector the
  knob turns between **GO** and **SYNC**. The click is armed 600 ms after the screen appears (as
  today) and re-checks the target at the click (as today). Big-jump frame for SYNC > 5° (as today).
- GO = existing GoTo (refused below min altitude, within 15° of the Sun, invalid input).
  SYNC = existing Sync to the entered coordinates. Both keep today's locks (date + time set, no
  driver fault) and the "Clears Sync. Sure?" rule for later date/time/UTC/longitude edits.
- The entered coordinates become "the target" used by Adjust → Sync on target.

## Adjust

One screen. Shows the selected axis, the step size (and what it is in degrees for RA/DEC/AZ;
steps only for Focus), and the axis position.

- **Turn**: move the selected axis by one step size per detent, in the turn's direction. With
  step size **JOG**, turning jogs continuously at the selected speed (today's Jog behaviour:
  stops 300 ms after the last detent, stale turns dropped). Speeds SLOW / CENTER / FIND / MAX.
- **Click**: next axis RA → DEC → AZ → Focus (AZ/Focus only when that motor is configured).
- **Long press**: opens the Adjust options list:
  - Step size: 1, 10, 100, 1 000, 10 000 steps, JOG SLOW, JOG CENTER, JOG FIND, JOG MAX
  - Sync on target (only when a target exists and date + time are set)
  - Set home here (confirm screen)
  - Go home (non-blocking, HOMING screen, ends idle — as today)
  - long press again = back to the main menu
- Steps are microsteps of that axis's driver as configured (the user picks counts, not degrees).
- **Tracking**: if the mount is tracking when an RA/DEC step move is made, it keeps tracking
  afterwards (step moves are on top of tracking, like OAT's `moveStepperBy`).
- Moving is locked by the same rules as motion today: driver fault → locked; AZ refuses moves when
  its driver failed (existing). Date/time are **not** required for Adjust (aligning to the markers
  happens before the clock is set).
- Any click or long press while an Adjust move is still running = STOP (rule M1 unchanged).

## Home, Sync and offsets

Verified in the code: OAT's `Mount::syncPosition` already keeps home fixed. It changes only the
sky reference of home (`_zeroPosRA`, `_zeroPosDEC`) and never the stepper step counters, so
`startSlewingToHome()` still goes to step 0 = the physical markers. This slice keeps that and
adds a test or written check that no new path rewrites step counters except *Set home here*
(`Mount::setHome`).

- Adjust moves change the step position (the scope physically moves); the sky offset is untouched,
  so the mount's reported RA/Dec follow the move. The next GoTo is computed from the same offset,
  so a nudge is one-off.
- *Sync on target* = `syncPosition(target)` at the current position: the nudge becomes the new
  offset for later GoTos. Home stays at the markers.
- *Set home here* = the markers: `setHome` at the current position (existing).

## Long press everywhere

- Editors: long press = previous field; on the first field = cancel (existing rule, keep working).
- Every other screen (menus, Point confirm, Adjust, Adjust options, Settings, info pages, refused
  page): long press = up one level; on the main menu it does nothing.
- While moving (slew, jog, Adjust move, homing): any click or long press = STOP first (M1).
- Fault page and reboot page keep their own rules.

## Park removed

- UI: no Park/Unpark.
- LX200 `:hP` (park, e.g. from Stellarium): go home and stop tracking (calls the same path as
  Adjust → Go home). Unpark `:hU` becomes a no-op reply. Builder's call how to keep upstream's
  park code compiled but unused.

## Focus motor

- E socket: STEP PB4, DIR PB3, EN PB5, UART PC11 (docs/wiring.md, Marlin E0).
- `FOCUS_STEPPER_TYPE ENABLED`, `FOCUS_DRIVER_TYPE DRIVER_TYPE_TMC2209_UART`, bipolar-modded
  5 V 28BYJ-48: same current rule as AZ (peak 150 mA → about 91 mA RMS, hold low, released when
  idle), same safety rules as AZ: EN high until the UART current is set, a failed UART test refuses
  every focus move without changing position, UART test mandatory.
- Microstepping/speed like AZ (2048 SPR, 16 µsteps, modest speed); builder checks the focuser
  speed/accel defines upstream uses.
- Fault page names "DRIVER FAULT" (code 3) for AZ/Focus as today.

## Phone (LX200)

Upstream already has a move-by-steps command per axis (`MeadeCommandProcessor::onMoveStepper`,
RA/DEC/AZ/ALT/Focus → `Mount::moveStepperBy`). The UI's Adjust step moves must call the same
`Mount` function, so the phone and the screen share one mechanism. Sync and set-home also exist
over LX200 (`:CM`, set-home command). The builder lists the exact command strings in
docs/architecture.md for the app slice. No new LX200 commands unless one is missing.

## Constraints

- Core UI stays pure C++ and host-tested (`src/core/ui/`), FakeCanvas width checks for every new
  string in EN and TR (menu rows ≤ 19, full lines ≤ 21, headers ≤ 12 beside the clock).
- Upstream files: small diffs guarded `#if BOARD == BOARD_STM32_MKS_ROBIN_E3`.
- `docs/wiring.md` updated for the focus socket.
- Gate `bash tools/gate.sh`; flash only on the 12/24 V supply.

## Calls made after the builder check (coordinator's calls, not the user's)

1. **Adjust RA moves and tracking.** Adjust calls `Mount::moveStepperBy` (same as the phone).
   The builder verifies on host/reading that tracking resumes after an RA step move when it was on
   before (no RA drift, TMC microstep switching intact). If not, a small guarded `Mount` fix is in
   scope.
2. **Set home here works without the clock.** It zeroes the step counters at the markers (that is
   the mechanical home). The sky reference of home (`_zeroPosRA` from LST, `_zeroPosDEC`) is
   computed when date and time are set, or immediately if they are already set. A Sync offset is
   dropped at Set home here: a new home is a new reference. Guarded change if `Mount::setHome`
   needs one.
3. **Step units.** 1 step = one slew-mode microstep of that axis, the unit `moveStepperBy` takes.
   Degrees shown for RA/DEC use the slew steps-per-degree, for AZ its steps-per-degree. Focus
   shows its own steps only.
4. **Target for Sync on target.** The last coordinates confirmed on the Point screen (GO or SYNC).
   RAM only, cleared by reboot. The existing "Clears Sync. Sure?" rule is unchanged.
5. **Go home** moves RA and DEC only (AZ and Focus have no home), needs no clock, ends idle.
   LX200 `:hP` does the same and leaves tracking off after the slew.
6. **Focus driver** reuses the AZ pattern (failed-driver guard, mandatory UART test, fault code 3).
   JTAG is already off at boot (PB3/PB4/PA15 free). PC11 does not clash with the knob's EXTI 11:
   SoftwareSerial samples RX from TIM7, it never attaches an interrupt (docs/wiring.md).

## Left to the builder

- Exact screen layouts within the 128×64 rules (follow the existing status/menu style).
- Step-size list storage (RAM is fine; persistence comes with the settings slice).
- How Adjust shows position (steps from home is fine).
- Turkish wording (ASCII, no diacritics).
