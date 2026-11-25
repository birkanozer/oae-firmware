# Progress: alt-e-focus-uln

Branch feature/alt-e-focus-uln off dev 6498873.

## Rulings
- SUPPLY_VOLTAGE stays 24 in the example config (advisor: bench may still be 24 V).
- Split T1 ALT on E / T2 focus ULN2003.

## Tasks
- T1 ALT on E: done. ALT on the E socket with the AZ pattern (guard, quiet TMC, PWM_OFS), UI axis id 4 shown between AZ and Focus, focus NONE in the example config; gate 502/502 + robin_e3 build.
- T2 focus ULN2003: done. DRIVER_TYPE_ULN2003 (focus only, Robin only), HALF4WIRE IN1/IN3/IN2/IN4, coils released when idle, example config 4096/1/800/1000; built blind (motor not connected); gate 502/502 + robin_e3 build.
- T3 supply 12/15/20/24 V (added by coordinator 2026-11-25): done. applySupplyVoltage accepts 12/15/20/24 (else 24), factor min(1, SUPPLY_12V_SPEED_FACTOR*V/12), LCD row cycles 12->15->20->24; gate 503/503 + robin_e3 build.
- T1 ALT on E: done 6f0bf1d
- T2 focus ULN2003: done 49c24ec
- T3 supply 12/15/20/24: done 5da3ba4
- Ruling: AZ/ALT drop TMC_TPWMTHRS (stay in stealthChop at all speeds, as AZ was before); small 28BYJ-48s, noise matters more than top-speed torque.
