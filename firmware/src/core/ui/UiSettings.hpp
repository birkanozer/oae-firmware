#pragma once

#include <cstdint>
#include "core/ui/UiText.hpp"

namespace core {

// The LCD's settings rows (RAM only until slice 3 swaps in the firmware-wide settings table).
enum class SettingId : uint8_t
{
    Language,  // 0 = EN, 1 = TR
    MinAlt,    // degrees
    Beep,      // 0 off, 1 on (click ticks only; safety beeps stay on)
    Supply,    // 0 = 12 V, 1 = 24 V; shown from the snapshot, changed with SetSupplyVoltage
    Count
};

enum class SettingType : uint8_t
{
    Enum,
    Int,
    Bool
};

struct SettingRow
{
    SettingId id;
    TextId label;
    SettingType type;
    int8_t min, max, def;
};

const SettingRow &settingRow(SettingId id);

class UiSettings
{
  public:
    UiSettings();
    int8_t get(SettingId id) const { return _v[static_cast<uint8_t>(id)]; }
    void set(SettingId id, int v);  // clamped to the row's range
    // Next value for Enum/Bool rows (wraps), the same value for Int rows.
    int8_t next(SettingId id) const;

  private:
    int8_t _v[static_cast<uint8_t>(SettingId::Count)];
};

}  // namespace core
