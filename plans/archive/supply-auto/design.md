# Supply voltage auto-detect (design)

Date: 2026-11-27. User: "Can we automatically detect the voltage and change the setting" -> "Yes,
build it". Context: a ZY12PDN USB-PD trigger gives 15 V from the power bank and 20 V from the wall
adapter on the same setting, so the supply voltage changes with the source.

## Hardware (no board modification)

The Robin E3 has no input-voltage sense (schematic V1.1_004 checked: the only dividers on 12-24 V feed
the 5 V buck). A plug on the unused bed-thermistor socket **J8 (TB, MCU pin PA1)** carries two
resistors:

- R_top 100 kOhm 1 %: supply + (after the 3 A fuse) -> J8 pin 1.
- R_bot 4.7 kOhm 1 %: J8 pin 1 -> J8 pin 2 (GND).

On-board (schematic): R20 4.7 kOhm pull-up to +3.3 V on the node, R21 100 Ohm node -> PA1, C20 (BOM
10 uF) node -> GND. Node voltage: V = (Vin/Rt + Vdd/Rp) / (1/Rt + 1/Rp + 1/Rb), Rp = 4.7 k.
12 V -> ~1.89 V, 15 -> ~1.95, 20 -> ~2.07, 24 -> ~2.16, 30 -> ~2.30 V (safe). Divider absent: the
pull-up holds the pin at Vdd (full scale) -> computed Vin ~ 73 V -> "not fitted".

## Firmware (Robin E3 only, guarded)

1. Pins/config: `SUPPLY_SENSE_PIN PA1` in pins_MKS_ROBIN_E3.hpp; `SUPPLY_SENSE` (0/1) with
   `SUPPLY_SENSE_RTOP 100000`, `SUPPLY_SENSE_RBOT 4700`, `SUPPLY_SENSE_RPULL 4700`,
   `SUPPLY_SENSE_CAL 1.0f` (multiplier for a one-time meter calibration) in Configuration_adv.hpp
   defaults; example config enables `SUPPLY_SENSE 1`. ConfigurationValidation: only on the Robin E3.
2. Measurement: 12-bit ADC; average 16 samples every 500 ms in the main loop (never in an ISR);
   correct Vdd with the STM32's internal reference (VREFINT 1.20 V, `AVREF` in STM32duino) if the core
   supports it, else assume 3.3 V; light exponential smoothing. Pure conversion + classification in
   core (host-tested): raw counts + vref counts -> millivolts; valid only between 9 and 30 V,
   otherwise "not fitted / out of range".
3. Snap: nearest of 12 / 15 / 20 / 24 within bands 10.5-13.5, 13.5-17.5, 17.5-22.0, 22.0-26.0 V;
   outside the bands or invalid -> no change. Debounce: the same snapped value for >= 2 s before it
   is applied. Apply through the existing `Mount::applySupplyVoltage` only while the mount is not
   slewing, jogging or homing (tracking is fine); otherwise defer until it stops.
4. Setting: Supply row choices **Auto / 12 / 15 / 20 / 24**, persisted in the existing settings block
   (0 = Auto; keep the block layout valid, bump the marker only if needed, and an old stored value
   must still load). Default Auto when `SUPPLY_SENSE 1`, else `SUPPLY_VOLTAGE`. In Auto with no valid
   reading, keep the last applied voltage (boot: stored manual value or `SUPPLY_VOLTAGE`). Manual
   values ignore the sense input.
5. Display: the Supply row shows `Auto 15.1V` (measured, one decimal) or `Auto --` without a valid
   reading; manual rows show `15 V` plus the measured value if it fits. EN/TR, width lint 21 columns
   (TR: `Oto`).
6. Snapshot: add measured millivolts and the applied voltage for the UI; adapter fills them.

## Docs

docs/wiring.md (J8 divider, values, safety: 1 % resistors, after the fuse, polarity), README
(Settings > Supply Auto, how to calibrate with a meter by setting `SUPPLY_SENSE_CAL`), CHANGELOG
[Unreleased], BACKLOG bench item (fit the divider, compare the shown voltage with a meter on bank and
adapter, check Auto switches 15 <-> 20 while idle).

## Out of scope

LX200 command for the voltage (later, for the phone); LCD calibration editor.

## Changes after review (2026-11-27)

- VREFINT dropped: the F103's VREFINT (+-3.3 %) is less accurate than the board's 3.3 V; Vdd is assumed 3.3 V.
- Snap replaced by acceptance windows: 12 V 10.5-14.2, 15 V 14.2-16.5, 20 V 18.5-21.5, 24 V 22.5-25.5;
  gaps keep the applied value; 0.3 V hysteresis at the 12/15 edge (12 V lead-acid/LiFePO4 sits at 13.2-14.4 V).
- Row shows applied and measured: `Auto 15 15.1` (EN), `Oto 15 15.1` (TR), `Auto 15 --` without a reading.
- An Auto change is skipped in a tick that issued Goto, GoHome, Park, JogStart or MoveSteps.
- Leaving Auto starts the manual cycle at the applied value.
