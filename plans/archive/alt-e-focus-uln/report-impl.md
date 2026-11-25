# alt-e-focus-uln: implementation report (saved by the controller from the implementer's hand-back)

Commits: 6f0bf1d T1 ALT on E socket, quiet AZ/ALT drivers, ALT on Adjust; 49c24ec T2 focus on ULN2003
(AccelStepper HALF4WIRE IN1,IN3,IN2,IN4, coils released when idle); 5da3ba4 T3 supply 12/15/20/24 V
(speed factor fminf(1, SUPPLY_12V_SPEED_FACTOR*V/12)).

Gate: T1 502/502, T2 502/502, T3 503/503 native; robin_e3 SUCCESS (RAM 16.6 %, flash 61.2 %).

Notes: upstream already parses `:MXl<n>#` as ALT (reused, now driver-checked). Adjust order RA, DEC, AZ,
ALT, Focus (kAxisOrder {0,1,2,4,3}); axisPos[5]/stepsPerDeg[5] indexed by axis id. ALT steps/deg from
upstream OAE formula (~59 476). "ALT DRIVER FAULT" on Adjust. clang-format v12 not installed: formatted
by hand.

Concerns raised: (1) TMC_TPWMTHRS put AZ/ALT into spreadCycle above ~31 full steps/s, i.e. nearly every
move; (2) fault text only for ALT; (3) ALT/AZ boot failure sets driverFault 3 (whole-UI lock), pre-existing
for AZ; (4) 5 kHz ISR caps AZ/ALT at ~5000 µsteps/s (pre-existing).

Not verifiable without hardware: ALT on PC11, ULN coil order/direction/speed, coils off at idle, PWM_OFS at
15/20 V, slew torque at 15 V.
