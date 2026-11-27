#include "core/SupplySense.hpp"

namespace core {

int32_t supplyMillivolts(uint32_t raw, uint32_t fullScale, const SupplyDivider &d)
{
    constexpr float kVdd = 3300.0f;  // the board's 3.3 V rail (an LDO: better than the F1's VREFINT, 1.16-1.24 V)
    // Node: (Vin/Rt + Vdd/Rp) / (1/Rt + 1/Rp + 1/Rb), solved for Vin.
    const float node = kVdd * raw / fullScale;
    const float vin = d.rTop * (node * (1.0f / d.rTop + 1.0f / d.rPull + 1.0f / d.rBot) - kVdd / d.rPull) * d.cal;
    return vin >= 9000.0f && vin <= 30000.0f ? static_cast<int32_t>(vin + 0.5f) : 0;
}

uint8_t snapSupplyVolts(int32_t mv, uint8_t applied)
{
    struct Window
    {
        int32_t lo, hi;  // [lo, hi) in mV
        uint8_t volts;
    };
    static const Window kWindows[] = {{10500, 14200, 12}, {14200, 16500, 15}, {18500, 21500, 20}, {22500, 25500, 24}};
    constexpr int32_t kMargin = 300;
    const Window *cur = nullptr;
    for (const Window &w : kWindows)
        if (w.volts == applied)
            cur = &w;
    for (const Window &w : kWindows)
    {
        int32_t lo = w.lo, hi = w.hi;
        if (cur && cur != &w)  // hysteresis: leave the applied window only 0.3 V past an edge the two share
        {
            if (cur->hi == w.lo)
                lo += kMargin;
            if (cur->lo == w.hi)
                hi -= kMargin - 1;  // hi is exclusive: mv <= edge - 0.3 V
        }
        if (mv >= lo && mv < hi)
            return w.volts;
    }
    return 0;
}

void SupplyFilter::sample(uint32_t nowMs, int32_t mv, uint8_t applied)
{
    _mv = mv == 0 ? 0 : (_mv == 0 ? mv : _mv + (mv - _mv) / 4);
    const uint8_t snap = snapSupplyVolts(_mv, applied);
    if (snap != _snap)
    {
        _snap = snap;
        _since = nowMs;
    }
    _stable = snap != 0 && nowMs - _since >= 2000 ? snap : 0;
}

}  // namespace core
