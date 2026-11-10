#include "core/ui/UiSettings.hpp"

namespace core {

namespace {

// Same order as SettingId.
const SettingRow kRows[] = {
    {SettingId::Language, TextId::LangRow, SettingType::Enum, 0, 1, 0},
    {SettingId::MinAlt, TextId::MinAlt, SettingType::Int, -10, 30, 0},
    {SettingId::Beep, TextId::BeepRow, SettingType::Bool, 0, 1, 1},
    {SettingId::Supply, TextId::SupplyRow, SettingType::Enum, 0, 1, 1},
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

int8_t UiSettings::next(SettingId id) const
{
    const SettingRow &r = settingRow(id);
    const int8_t v = get(id);
    if (r.type == SettingType::Int)
        return v;
    return static_cast<int8_t>(v >= r.max ? r.min : v + 1);
}

}  // namespace core
