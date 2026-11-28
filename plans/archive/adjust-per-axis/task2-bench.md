# Task 2: DEC and ALT skipping, bench test build

Spec: `plans/adjust-per-axis/design.md` § Skipping. Only test 1 for each axis goes in now.

## Steps

1. In `firmware/Configuration_local_examples/Configuration_local_robin_e3.hpp` (the file the `robin_e3` build
   uses; `firmware/LocalConfiguration.hpp` falls back to it):
   - DEC: add `#define DEC_UART_STEALTH_MODE 0` next to the DEC current lines, with a short comment: bench
     2026-11-28 DEC buzzed/stalled in stealthChop under load (autoscale never tuned); spreadCycle test. Confirm
     `Configuration_adv.hpp` guards it with `#ifndef` (it does at the Robin E3 block) and that `Mount.cpp` uses it
     via `en_spreadCycle(DEC_UART_STEALTH_MODE == 0)`.
   - ALT: `ALT_STEPPER_SPEED` → `(200 * ALT_MICROSTEPPING)`, `ALT_STEPPER_ACCELERATION` → `(100 * ALT_MICROSTEPPING)`,
     comment: 400 full steps/s (6400 microsteps/s) was above the 5 kHz stepper ISR and stalled under the rod load.
   - Leave AZ, RA and everything else unchanged.
2. Check nothing else overrides these (`grep -rn "DEC_UART_STEALTH_MODE\|ALT_STEPPER_SPEED\|ALT_STEPPER_ACCELERATION" firmware --include=*.hpp --include=*.cpp --include=*.ini`, ignoring `.pio`).
3. `.venv/Scripts/pio.exe run -e robin_e3` from `firmware/` must build clean.
4. Commit only that file: `git commit -- firmware/Configuration_local_examples/Configuration_local_robin_e3.hpp`
   (Task 1 is committing on the same branch in parallel; if `.git/index.lock` exists, wait a few seconds and retry).

## Out of scope

Everything under `firmware/src/core/ui/`, tests, docs, CHANGELOG, BACKLOG.
