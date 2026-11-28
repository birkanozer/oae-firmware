# Adjust page fixes and per-axis step sizes

Bench report 2026-11-28 on v0.6.2 (`BACKLOG.md` › Bugs › Adjust page). Approved in chat 2026-11-28.

## UI (verified by the native tests)

1. **Long press on Adjust options returns to Adjust**, not the main menu. To avoid a loop with no way out
   (long press on Adjust opens the options), the options list gets a last row **Main menu** (TR: **Ana menü**)
   that opens the main menu. Rows: Step size, Sync on target, Set home here, Go home, Main menu.
   Long press on Set home confirm still returns to the options.
2. **Stopping a move on the Adjust page stays on Adjust.** `stop()` today always switches to Status. When the
   stop comes from the Adjust page (click or long press while moving), the screen stays Adjust; the STOPPED
   transient and long beep stay. Stops from every other screen keep going to Status.
3. **Step size per axis, Jog options removed.** Each axis keeps its own step size selection (RAM, survives
   leaving and reopening Adjust, not power cycles). Lists, in steps:
   - RA, DEC: 10, 100, 1 000, 10 000, 50 000 (DEC 2 000 steps/deg: ~0.005° to 25°)
   - AZ, ALT: 100, 1 000, 10 000, 60 000, 300 000 (AZ 64 054, ALT 59 479 steps/deg: ~5″ to ~5°)
   - Focus: 1, 10, 100, 1 000, 10 000 (as today)
   Default selection: the middle entry (index 2) for every axis. The JOG SLOW / CENTER / FIND / MAX options and
   the UI's own jog path go away from the Adjust page; LX200 jogging (`MountState::Jogging`) is untouched.
   Each move runs at the axis's own configured max speed and acceleration.

## Skipping (bench test builds, not verified by the gate)

Bench: RA and AZ clean; DEC and ALT buzz / stall and lose position.

- **DEC.** On this board (old AccelStepper path, no `NEW_STEPPER_LIB`) DEC runs at `DEC_STEPPER_SPEED` 1300
  microsteps/s × supply factor (0.875 at 15 V), about 0.57 °/s = 70 full steps/s, accel 6000. `DEC_SLEWING_SPEED_DEG`
  does not apply. Speed and interrupt rate are not the cause. Suspect: stealthChop under load with untuned
  autoscale (no AT#1/AT#2 run). Test 1: DEC in spreadCycle (`DEC_UART_STEALTH_MODE 0`). If it still stalls:
  test 2 DEC current 100 %, test 3 accel 3000.
- **ALT.** 400 full steps/s × 16 = 6400 microsteps/s is above the 5 kHz stepper ISR (likely ~4.4 kHz: the core runs
  at 64 MHz) and near the 28BYJ-48's limit under the rod load; AZ has the same speed but less load. Test 1: speed
  200 full steps/s (3200 microsteps/s), accel 100 full steps/s² (1600). If it still stalls: ALT stealthChop off.

The backlog's skip items close only after the user confirms on the bench.
