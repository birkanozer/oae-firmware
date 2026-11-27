#include "core/ui/UiSettings.hpp"
#include <cstring>

namespace core {

namespace {

// Same order as SettingId.
const SettingRow kRows[] = {
    {SettingId::Language, TextId::LangRow, SettingType::Enum, 0, 1, 0},
    {SettingId::MinAlt, TextId::MinAlt, SettingType::Int, -10, 30, 0},
    {SettingId::Beep, TextId::BeepRow, SettingType::Bool, 0, 1, 1},
    {SettingId::Supply, TextId::SupplyRow, SettingType::Enum, 0, 4, 0},  // Auto (no sense input: mirrored)
    {SettingId::OwnWifi, TextId::NetOwnWifi, SettingType::Enum, 0, 2, 0},
};

static_assert(sizeof(kRows) / sizeof(kRows[0]) == static_cast<unsigned>(SettingId::Count), "kRows must match SettingId");

}  // namespace

const SettingRow &settingRow(SettingId id)
{
    return kRows[static_cast<uint8_t>(id)];
}

UiSettings::UiSettings()
{
    for (const SettingRow &r : kRows)
        _v[static_cast<uint8_t>(r.id)] = r.def;
}

void UiSettings::set(SettingId id, int v)
{
    const SettingRow &r = settingRow(id);
    _v[static_cast<uint8_t>(id)] = static_cast<int8_t>(v < r.min ? r.min : (v > r.max ? r.max : v));
}

void UiSettings::save(uint8_t *out) const
{
    out[0] = kStoreMarker;
    for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i)
        out[1 + i] = static_cast<uint8_t>(_v[i]);
}

bool UiSettings::load(const uint8_t *in)
{
    const bool legacy = in[0] == kLegacyMarker, ok = in[0] == kStoreMarker || legacy;
    for (const SettingRow &r : kRows)
    {
        const int v = static_cast<int8_t>(in[1 + static_cast<uint8_t>(r.id)]) + (legacy && r.id == SettingId::Supply);
        _v[static_cast<uint8_t>(r.id)] = static_cast<int8_t>((ok && v >= r.min && v <= r.max) ? v : r.def);
    }
    return ok;
}

bool SettingsStoreLatch::observe(const uint8_t *block)
{
    const bool write = _seen && memcmp(block, _last, sizeof _last) != 0;
    memcpy(_last, block, sizeof _last);
    _seen = true;
    return write;
}

int8_t UiSettings::next(SettingId id) const
{
    const SettingRow &r = settingRow(id);
    const int8_t v = get(id);
    if (r.type == SettingType::Int)
        return v;
    return static_cast<int8_t>(v >= r.max ? r.min : v + 1);
}

}  // namespace core
