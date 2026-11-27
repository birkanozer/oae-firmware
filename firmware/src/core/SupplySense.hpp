#pragma once

#include <cstdint>

namespace core {

// Supply voltage sense on the Robin E3's bed-thermistor input: supply -> rTop -> node -> rBot -> GND, with the
// board's pull-up rPull from Vdd to the node. cal: multiplier from a one-time comparison with a meter.
struct SupplyDivider
{
    float rTop, rBot, rPull, cal;
};

// Supply in millivolts from ADC counts summed over any number of samples (raw = the node, fullScale = full-scale counts x
// samples). Vdd is taken as 3.3 V. 0 outside 9..30 V: not fitted (the pull-up holds the pin at Vdd, about 73 V) or out of range.
int32_t supplyMillivolts(uint32_t raw, uint32_t fullScale, const SupplyDivider &d);

// The supply choice whose window holds the reading (12: 10.5-14.2, 15: 14.2-16.5, 20: 18.5-21.5, 24: 22.5-25.5 V), else 0
// (gaps and out of range keep what is applied). With a different choice `applied` the new window starts 0.3 V inside
// an edge it shares with the applied one (12/15 at 14.2 V), so a 12 V battery on charge never flips to 15.
uint8_t snapSupplyVolts(int32_t mv, uint8_t applied = 0);

// Smooths the readings (one every 500 ms) and reports the snapped choice once it held for 2 s.
class SupplyFilter
{
  public:
    // mv 0 = no valid reading: clears the value and the debounce. applied = the voltage in use (0 = unknown).
    void sample(uint32_t nowMs, int32_t mv, uint8_t applied = 0);
    int32_t millivolts() const { return _mv; }  // smoothed, 0 = no valid reading
    uint8_t stableVolts() const { return _stable; }  // 12, 15, 20 or 24, 0 = none

  private:
    int32_t _mv = 0;
    uint8_t _snap = 0, _stable = 0;
    uint32_t _since = 0;
};

}  // namespace core
