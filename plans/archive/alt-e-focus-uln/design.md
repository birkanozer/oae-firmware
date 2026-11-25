# ALT on the E socket, focus on a ULN2003 (design)

Date: 2026-11-25. User in chat: "Move ALT to E. Yes, it's bipolar." (ALT on E approved) and earlier
"Fix it as 28BYJ in unipolar mode. I will use spare ports for that." (focus on a ULN2003). The focus
motor is not connected yet, so task 2 is built blind; task 1 is bench-testable now.

Two tasks, in order: **T1 ALT on E** (focus `STEPPER_TYPE_NONE` at the end of T1, the build and
gate must pass), then **T2 focus on ULN2003**.

## Hardware

| Axis | Motor | Driver | Pins |
|---|---|---|---|
| RA, DEC | NEMA17 | X, Y TMC2209 UART | unchanged |
| AZ | 28BYJ-48 bipolar-modded | Z socket TMC2209 UART | PC14 step, PC15 dir, PB8 EN, PC12 UART (unchanged) |
| ALT | 28BYJ-48 bipolar-modded | **E socket** TMC2209 UART | PB4 step, PB3 dir, PB5 EN, PC11 UART, address 0 |
| Focus | 28BYJ-48 **unipolar (stock)** | **ULN2003 board** on ULN group B | IN1 PA7 (EXP1 6), IN2 PC4 (EXP1 7), IN3 PC5 (EXP1 8), IN4 PC10 (EXP2 7); 5 V + GND from EXP1 10/9 |

ULN group A (PB13/PB14/PB15/PA15) stays defined and unused.

## Decisions

1. **Pins.** In `pins_MKS_ROBIN_E3.hpp` the AZ pins become their own defines on the Z socket (the
   AZ-aliases-ALT block goes), ALT pins move to the E socket, the E-socket FOCUS_* defines are
   replaced by `FOCUS_ULN_PIN1..4` = ULN group B. ALT_DIAG_PIN PB1 stays a spare.
2. **ALT axis** = an exact copy of the AZ pattern: same current (150 mA peak, 91 mA RMS, IRUN 2),
   hold 30 %, released when idle (`ALT_ALWAYS_ON 0`), 2048 SPR x16, speed/accel like AZ,
   `ALT_COIL_RESISTANCE_OHM` 50 in `applySupplyVoltage`, `_altDriverFailed` guard with
   `enableAltMotor()` returning bool (a failed driver refuses every ALT move without changing its
   position, EN never goes low), `#error` unless `UART_CONNECTION_TEST_TXRX 1`. Steps per degree
   from upstream's OAE ALT formula (whatever upstream uses for `ALTITUDE_STEPS_PER_ARC_MINUTE` /
   OAE), with the 2048 x16 motor numbers.
3. **Quiet TMC settings for AZ and ALT** (stealthChop, `TMC_PWM_FREQ`, `TMC_TOFF`, TBL,
   `TMC_IHOLDDELAY`, `TMC_TPOWERDOWN`, `TMC_TPWMTHRS`) like RA/DEC/Focus got. Closes the BACKLOG
   bug "AZ driver has no TMC quiet settings".
4. **Focus on ULN2003.** OAT v1.14 has no ULN2003 driver type. Add `DRIVER_TYPE_ULN2003 4` to
   `Constants.hpp` (one line) and, guarded for the Robin E3 / that driver type, build the focus
   stepper as AccelStepper `HALF4WIRE` with pin order IN1, IN3, IN2, IN4 (28BYJ-48 coil order).
   Half-step: `FOCUS_STEPPER_SPR 4096`, no microstepping (`FOCUS_MICROSTEPPING 1`), speed about
   800 half-steps/s, acceleration about 1000/s^2 (28BYJ-48 at 5 V stalls near 1000/s). Coils are
   released (`disableOutputs()`, all four pins low) when idle unless `FOCUSER_ALWAYS_ON 1`. No UART,
   no current setting, no fault detection: `focusDriverFailed()` is false and the
   UART-test `#error` for focus is removed for this driver type. ConfigurationValidation allows
   ULN2003 for focus only (not RA/DEC/AZ/ALT). On the Robin the steppers are plain AccelStepper
   objects (`Mount.cpp` ~line 405, `new AccelStepper(AccelStepper::DRIVER, ...)`), so HALF4WIRE
   fits; check the `StepperFocusSlew` branch is not used on this board. Grep for any other use of
   PA7, PC4, PC5, PC10 before claiming them.
5. **UI.** Adjust axes: RA, DEC, AZ, ALT, Focus (ALT shown only when present, like AZ and Focus).
   `UiAction::MoveSteps` axis indices keep 0 RA, 1 DEC, 2 AZ, 3 Focus and add **4 ALT** (no
   renumbering). Snapshot gets `hasAlt`, `altFault`; `axisPos` becomes `[5]` and `stepsPerDeg`
   becomes `[5]`, both indexed by the axis id, focus slot 0 (no degrees). Tests cycle the Adjust
   axes in display order with ALT present and absent (and AZ/Focus present and absent). Fault text
   names ALT. The adapter reports ALT running as Jogging and Stop stops ALT, like AZ. An ALT fault
   locks only ALT moves, like AZ.
6. **LX200.** ALT step move over `:MX`: grep upstream's `:MX` handler (MeadeCommandProcessor /
   core/meade) first and reuse its ALT letter if it has one, else add `:MXa<n>#` beside
   `:MXz`/`:MXf`. Upstream ALT commands (`:MAL`, etc.) keep working.
7. **Example config** `Configuration_local_robin_e3.hpp`: ALT enabled on TMC2209 UART with the AZ
   numbers; focus `DRIVER_TYPE_ULN2003` with the half-step numbers; comments updated;
   `SUPPLY_VOLTAGE` stays 24 (the bench may still be on 24 V; the user switches it in Settings).

## Out of scope

Camera and ESP32 (separate slice), ULN group A, homing switches for ALT/AZ.

## Not verifiable here

Everything on hardware: ALT answering on PC11, ULN coil order and direction, focus speed limit.
Native tests and the robin_e3 build are the gate.
