# DEC / ALT buzz-from-first-step: per-axis code comparison

Read-only investigation, 2026-11-28. Line numbers are at commit `6070b35` (`firmware/src/Mount.cpp` moved by +11/+16
lines during the investigation because of the set-home-all commits; search by the quoted code if they drift again).
Build: env `robin_e3`, config `Configuration_local_examples/Configuration_local_robin_e3.hpp` (no
`firmware/Configuration_local_robin_e3.hpp` override exists), old AccelStepper path (`NEW_STEPPER_LIB` undefined),
`SW_SERIAL_UART 1`, `UART_CONNECTION_TEST_TXRX 1`, `UART_CONNECTION_TEST_TX 0`.

## Verdict

The code paths compile to near-identical sequences within each pair. **No firmware difference was found that sets
{DEC, ALT} apart from {RA, AZ}.** The few in-pair differences that do exist (table below) either favour DEC/ALT or are
on the working axis. The symptom also rules out a dead UART: a failed UART gives silence, not buzz (see 1d).
"Buzz from the first step, barely turns, independent of speed and chopper mode" is the textbook sign of one coil
not driven correctly: crossed coil pair, open coil, bad crimp, or a dead half-bridge in that socket's driver.
Firmware cannot see that today, because nothing reads driver state after boot.

## Differences found, ranked by how well they could explain DEC+ALT-only buzz

| # | Difference | Where | Could it cause DEC/ALT-only buzz? |
|---|---|---|---|
| 1 | Nothing reads driver state back after boot. `microsteps()`, `pwm_ofs()` and the rest are blind writes. No `DRV_STATUS`/`GSTAT`/`IFCNT` check anywhere (grep). | Mount.cpp runtime driver calls are only `microsteps` (1642, 1652, 1710, 1720, 1989, 2817, 2842, 2847, 3497, 3542, 3573, 3617, 4214, 4684, 4687) and `applySupplyVoltage` 2154-2180 | Not a cause. It is the blind spot: an open phase, a crossed pair or a driver that reset (GSTAT.reset, uv_cp) looks the same to the firmware as a healthy one. |
| 2 | ALT is the last half-duplex SoftwareSerial **listener** after boot: configured last, with reads in `connectToDriver("ALT")` at 852. In half-duplex, the active listener's pin sits in `INPUT_PULLUP`, about 40 kΩ against the board's 20 kΩ pull-down, so roughly 1.1 V, i.e. low. No later read moves the listener (no runtime reads at all). DEC's pin is back to OUTPUT HIGH once AZ's test calls `stopListening()`. | STM32duino `SoftwareSerial.cpp` `listen`/`stopListening`/`setRXTX`/`send`; `TmcHalfDuplex.cpp` | Low. With `pdn_disable(1)` already written, a held-low PDN_UART is only a UART break to the TMC2209. Every garbage frame still needs sync, address 0, the write bit and a CRC8 match. It is the only AZ-vs-ALT code asymmetry, but it does not cover DEC. |
| 3 | DEC idles at 256 microsteps (`DEC_GUIDE_MICROSTEPPING`) and is switched to 16 by a blind UART write before every step move (4214), then back to 256 on arrival (3542). If the 16 write were lost, DEC would move 1/16 of the distance. | Mount.cpp 688, 4214, 3542 | Low. A lost write gives a short but smooth, quiet move, not buzz. RA does the same switch (256 -> 8, 4187-4188) through the same SoftwareSerial code and works. The SW-serial timer (prio 14) preempts the stepper ISR (prio 15, `InterruptCallback.cpp:126-133`), so bit timing is protected. Does not cover ALT (ALT never switches: 877). |
| 4 | DEC is now spreadCycle (`DEC_UART_STEALTH_MODE 0` -> `en_spreadCycle(1)` at 676). RA stays stealthChop (Robin block default `RA_UART_STEALTH_MODE 1`, Configuration_adv.hpp:76-78). Chopper is default TOFF 3, TBL 1 (`blank_time(24)` = `tbl(1)`, not "TBL 2" as the comments say), HSTRT 5, HEND 0 (from shadow 0x10000053). | Mount.cpp 675-676, 686 | No. The buzz existed in stealthChop before the switch, and the switch did not change it. |
| 5 | `fclktrim(4)` is written on RA only (upstream). It changes RA's internal clock trim in FACTORY_CONF. | Mount.cpp 586 | No. It is on the working axis, and AZ works without it. |
| 6 | `SGTHRS` 100 (RA) vs 10 (DEC/AZ/ALT). TCOOLTHRS 0xFFFFF on all of them, DIAG pins only `INPUT`. | Configuration_adv.hpp:166-167, Mount.cpp 588/692/786/880 | No. StallGuard only drives DIAG, nothing acts on it. |
| 7 | DEC limits: target clamped to ±135° × steps/deg (`_decUpper/LowerLimit`, from EEPROM or the default). | Mount.cpp 231-241, `moveSteppersTo` 4101-4150 | No. A clamp shortens or cancels a move, it cannot buzz. DEC steps/deg (2000 = 1.0 × 45000 × 16 / 360) is used for limits, display and guiding, not for raw `moveStepperBy` steps or speed. `EEPROMStore::getDECStepsPerDegree` (EPROMStore.cpp:548) returns the default unless a calibration was stored. |
| 8 | Speeds: DEC 1300 µsteps/s at 16 µsteps (81 full steps/s), accel 6000. RA 1200 at 8 µsteps (150 full steps/s), accel 6000. ALT 3200 (200 full steps/s), accel 1600. AZ 6400 (400 full steps/s, above the 5 kHz ISR), accel 3200. `moveStepperBy` does not call `setMaxSpeed`: DEC uses the last value set (boot 1300, `startSlewing*` 1618/1678, `setSlewRate` 1357 scales it by 0.05-1). | Mount.cpp 339-341, 386-388, 2158-2160 | No. DEC and ALT run gentler than their working twins. Speed tests did not change the symptom. |

## 1. Driver construction and UART setup (SW-serial path, `BOARD_STM32_MKS_ROBIN_E3` branch)

### RA (546-589) vs DEC (650-693): identical sequence
`new TMC2209Stepper(pin, pin, 0.11, 0)` (own `SoftwareSerial` per driver, RX == TX, half-duplex), `beginSerial(19200)`,
`mstep_reg_select(true)`, `pdn_disable(true)`, `connectToDriver` (`test_connection()` = `DRV_STATUS` not 0 and not
0xFFFFFFFF), `toff(0)`, `I_scale_analog(false)`, `rms_current(mA, X_HOLD_PERCENT/100 = 0.5)`, `iholddelay(10)`,
`TPOWERDOWN(20)`, `pwm_freq(2)`, `toff(3)`, `en_spreadCycle(STEALTH==0)`, `TPWMTHRS(1500)`, `blank_time(24)`, `semin(0)`,
`microsteps(...)`, then `fclktrim(4)` (RA only), `TCOOLTHRS(0xFFFFF)`, `SGTHRS`.
Pins and addresses: RA PC7 / DEC PD2, address 0 on both (separate wires).

### AZ (750-787) vs ALT (844-883): identical sequence
Same as above but `rms_current(mA, 0.30)`, `en_spreadCycle(0)` (stealthChop at every speed), no `TPWMTHRS` (0),
`microsteps(16)`, no `fclktrim`. Pins: AZ PC12 / ALT PC11, address 0.

### applySupplyVoltage (2154-2180, called at the end of setup and from Settings)
`pwm_ofs(374 × R × I / V)` + `pwm_autoscale(true)`. RA/DEC: R 1.6 Ω, 249 mA -> 6 at 24 V. AZ/ALT: R 50 Ω, 106 mA -> 83 at
24 V. Identical within each pair. PWMCONF is shadow-written, with no read.

### UART test failure (1d)
- RA/DEC: `digitalWrite(X_EN_PIN, HIGH)` (557/661) and `_driverFault` = 1 or 2. The only other DEC EN write is LOW at boot,
  b_setup.hpp:288, before configuration. A failed DEC is therefore unpowered: silent.
- AZ/ALT: EN HIGH plus `_azDriverFailed` / `_altDriverFailed` (477/481). `enableAltMotor()` (2371-2382) then refuses every
  move: silent, no step counting.
- The buzz therefore proves that both UARTs answered at boot.

## 2. Microsteps

- Resolved values: `RA_SLEW_MICROSTEPPING` 8, `RA_TRACKING_MICROSTEPPING` 256, `DEC_SLEW_MICROSTEPPING` 16,
  `DEC_GUIDE_MICROSTEPPING` 256 (Configuration_adv.hpp:112-141, no local override), `ALT_MICROSTEPPING` 16,
  `AZ_MICROSTEPPING` 16 (local config).
- `TMCStepper::microsteps()` maps 256->MRES 0 and 16->MRES 4. Any other value is silently ignored, but only 8/16/256 are
  used, so it never falls through. `mstep_reg_select(1)` is set first, so MRES wins over the MS1/MS2 straps (which would
  give 8). GCONF is shadow-written each time with all bits, so later GCONF writes keep it.
- DEC: 256 at boot (688), 16 in `moveStepperBy` (4214), 256 on arrival (3542) and after a home slew (3617), 16 for a park leg
  (3573). The switch is a blind write: no readback, no IFCNT check (item 3).
- ALT: 16 once at boot (877), never changed. AccelStepper counts in 16ths, which matches the driver.
- Neither axis can reach full step: `== 1 ? 0` never triggers. 256 is reachable for DEC only if a 16 write is lost.

## 3. Enable pins (LOW = on)

- RA PC13, DEC PB12: LOW at boot (b_setup.hpp:253, 288), HIGH only on a UART failure. Always on, so they use TMC
  standstill reduction (TPOWERDOWN 20 ≈ 0.44 s, IHOLD 50 %).
- AZ PB8, ALT PB5: HIGH at boot (b_setup.hpp:324, 345). LOW in `enableAzMotor`/`enableAltMotor` right before `moveTo`, HIGH
  again in `disableAzAltMotors` (2315-2349) from `loop()` (3370) only when **both** AZ and ALT have stopped. The ISR never
  touches EN. No toggle mid-move: `isRunning()` is true as soon as `moveTo` sets a target.
- Hold current never applies during motion: TPOWERDOWN counts from the last step, and the step interval is under 1 ms.
- IHOLD for AZ/ALT rounds to 0 (2 × 0.3). It is irrelevant because EN is released when idle, and identical for both.

## 4. Step/dir pins and the ISR

- One TIM6 ISR at 5 kHz, priority 15 (`InterruptCallback.cpp:121-139`), calls `Mount::interruptLoop()` (3245).
  RA and DEC: `_stepperRA->run()` / `_stepperDEC->run()` under `STATUS_SLEWING` (3271-3272).
  AZ and ALT: `_stepperAZ->run()` / `_stepperALT->run()` on every tick (3305-3310). So DEC shares RA's path and ALT shares
  AZ's path exactly.
- AccelStepper `DRIVER`, `digitalWrite` (BSRR, atomic), DIR written before STEP in `step1()`. No `setPinsInverted`
  (all `*_INVERT_DIR` are 0).
- Pins match Marlin / Klipper for the Robin E3 v1.1 (docs/wiring.md): Y = PC2 / PB9 / PB12 / PD2, E = PB4 / PB3 / PB5 / PC11.
- JTAG: `__HAL_AFIO_REMAP_SWJ_NOJTAG()` first thing in `setup()` (b_setup.hpp:124-127). STM32duino's `pin_function()`
  also calls `pinF1_DisconnectDebug()`, which does NOJTAG for PA15/PB3/PB4 on every `pinMode`. Later HAL remaps OR in
  SWJ_CFG = 111, which RM0008 treats as "no change". PB3/PB4 are GPIO. Ruled out.
- Conflicts checked: TIM4 tone toggles only BEEPER PC1 from its update IRQ, with no output-compare channel on PB9.
  TIM6 is the stepper, TIM7 the SoftwareSerial. LCD on PA4/5/6 uses BSRR (`U8g2Canvas.cpp:28`). Knob on PB11/PB0
  (EXTI 11/0), click PC3, supply sense PA1, ULN group B PA7/PC4/PC5/PC10, I2C PB6/PB7 (no remap), USART1 PA9/10, USART2
  PA2/3. No SPI1 remap (which would grab PB3/4/5). Nothing else touches PC2, PB9, PB12, PB3, PB4 or PB5. Ruled out.
- PC11 (ALT UART) shares EXTI line 11 with knob A, but SoftwareSerial samples from TIM7, with no EXTI. Ruled out.

## 5. DEC_TRANSMISSION / steps per degree

`DEC_TRANSMISSION` 1.0 (pins file), `DEC_STEPPER_SPR` 45000, so `DEC_STEPS_PER_DEGREE` = 2000. It feeds limits (231-241),
position display, guiding speed (1818) and manual slew `setSpeed` (2023). It does not feed `moveStepperBy`'s raw step count
or `_maxDECSpeed`. An old stored calibration could only change the limits or the display. Ruled out for buzz.

## 6. Currents (TMCStepper `rms_current`, R_SENSE 0.11 + 0.02)

`CS = 32 × 1.41421 × mA/1000 × 0.13 / 0.325 - 1`, and if that is under 16, vsense = 1 and divide by 0.180 instead.
`I_rms = (CS+1)/32 × 0.180/0.13/1.41421`.

| Axis | Request | CS (IRUN) | vsense | Actual RMS | IHOLD |
|---|---|---|---|---|---|
| RA | 470 × 0.75 / 1.414 = 249 mA | 7 | 1 | 245 mA | 3 (122 mA) |
| DEC | same | 7 | 1 | 245 mA | 3 (122 mA) |
| AZ | 150 / 1.414 = 106 mA | 2 | 1 | 92 mA | 0 |
| ALT | same | 2 | 1 | 92 mA | 0 |

Identical within each pair. Note: `DEC_STEPPER_SPR` (200 × 50 × 4.5) says DEC is a 1.8° geared motor, while RA is
400 × 9 (0.9°). If DEC's motor is not the 470 mA part, its current setting may simply be wrong for it. That would make it
weak, though, not buzz from the first step. ALT at 92 mA carries the rod load that AZ does not. Current is the only ALT knob
not yet tried: the next bin is IRUN 3 = 122 mA for a 123-153 mA RMS request, i.e. `ALT_MOTOR_CURRENT_RATING` 174-216
at 100 %.

## Suggested bench discriminators (not done, read-only task)

1. **Swap motors between a good and a bad socket** (same cable with the motor): DEC motor on X and RA motor on Y; ALT
   motor on Z and AZ motor on E. If the buzz follows the motor, it is coil pairing, cable or motor. If it stays on the
   socket, it is that driver or the board. This alone settles it.
2. **Check the coil pairs** with an ohmmeter at the plug end that goes into the board: pins 1-2 one coil, 3-4 the other
   (the Robin E3 socket order). DEC's geared motor may use a different wire order from RA's. ALT's cut-trace mod may
   have a coil open or a pair crossed (wiring.md: "If the motor only shakes ... recheck the pairing").
3. **DEC step-path bypass:** `#define UART_CONNECTION_TEST_TX 1` runs `testDEC_UART_TX()` (4677) at boot, which drives DEC
   with VACTUAL (the driver's own step generator, no STEP/DIR pins) 5° each way. Smooth means the motor and driver are
   fine and the fault is in the step/dir path. Buzz means motor, cable or driver output. RA and DEC only, no ALT
   equivalent.
4. **Read back once (temporary debug):** `DRV_STATUS` during a slow DEC move in spreadCycle (`ola`/`olb` open load,
   `s2ga`/`s2gb` short, `ot`/`otpw`), plus `GSTAT`, `IFCNT` (it should rise by one per write; this proves the microstep
   switch landed) and `CHOPCONF.mres`. Open-load flags are only valid in spreadCycle, which DEC already is.
