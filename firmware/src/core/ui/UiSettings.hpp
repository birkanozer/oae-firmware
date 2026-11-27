#pragma once

#include <cstdint>
#include "core/ui/UiText.hpp"

namespace core {

// The LCD's settings rows. Kept across power cycles: the adapter stores save()'s block and load()s it at boot.
enum class SettingId : uint8_t
{
    Language,  // 0 = EN, 1 = TR
    MinAlt,    // degrees
    Beep,      // 0 off, 1 on (click ticks only; safety beeps stay on)
    Supply,    // 0 Auto (supply sense), 1..4 = kSupplyVolts[v - 1]; manual: shown from the snapshot, set with SetSupplyVoltage
    OwnWifi,   // 0 Auto (own access point when no hotspot is found), 1 On (always), 2 Off (hotspot only); on Network, read by :XNG
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

constexpr int kSupplyVolts[] = {12, 15, 20, 24};  // motor supply choices (15 V: USB-PD power banks)

class UiSettings
{
  public:
    UiSettings();
    int8_t get(SettingId id) const { return _v[static_cast<uint8_t>(id)]; }
    void set(SettingId id, int v);  // clamped to the row's range
    // Next value for Enum/Bool rows (wraps), the same value for Int rows.
    int8_t next(SettingId id) const;

    // Stored block: a marker byte, then one byte per SettingId (a manual Supply mirrors the firmware's voltage, see
    // UiController::tick).
    static constexpr uint8_t kStoreMarker = 0xA2;  // never 0x00 (cleared store) or 0xFF (blank chip, failed read)
    static constexpr uint8_t kLegacyMarker = 0xA1;  // before Auto: Supply was 0..3 (loads as 1..4)
    static constexpr uint8_t kStoreBytes = 1 + static_cast<uint8_t>(SettingId::Count);
    void save(uint8_t *out) const;
    // Always starts from the defaults. False on a blank, cleared or unknown block (all defaults). A value outside
    // its row's range falls back to that row's default.
    bool load(const uint8_t *in);

  private:
    int8_t _v[static_cast<uint8_t>(SettingId::Count)];
};

// Store-on-change latch: the first observe() only records the block (a boot never writes); later calls
// return true when the block differs from the last one recorded, which is then the one to store.
// reset(): the next observe() records again without writing (after the store was cleared and reloaded).
class SettingsStoreLatch
{
  public:
    bool observe(const uint8_t *block);
    void reset() { _seen = false; }

  private:
    uint8_t _last[UiSettings::kStoreBytes];
    bool _seen = false;
};

}  // namespace core
