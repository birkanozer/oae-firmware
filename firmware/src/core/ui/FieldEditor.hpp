#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

enum class FieldKind : uint8_t
{
    Time,
    TimeHM,
    RA,
    Dec,
    Latitude,
    Longitude,
    Date,
    Int
};

// Edits one value group by group (hours/minutes/seconds, sign/degrees/..., year/month/day).
// Values are clamped to the kind's range after every turn, so value() is always valid.
class FieldEditor
{
  public:
    // Time/RA: seconds 0..86399. TimeHM: same, hh:mm only (seconds dropped, stay 0). Dec: arc-seconds +-324000. Latitude: +-324000, Longitude: +-648000
    // (both edited in minutes, seconds dropped). Date: yyyy*10000+mm*100+dd. Int: min..max.
    void begin(FieldKind kind, int32_t value, int32_t min = 0, int32_t max = 0);
    // Changes the selected group, wrapping inside its range; nothing carries into other groups.
    void turn(int8_t detents);
    // Next group. Returns true when the last group was confirmed.
    bool click();
    // Previous group. Returns true when already on the first group (= cancel).
    bool back();
    // Latitude/Longitude: show a hemisphere letter as the last group instead of a leading sign.
    // Call after begin().
    void setLetters(char pos, char neg);
    int32_t value() const;
    uint8_t group() const { return _group; }
    void format(char *out, size_t len) const;

  private:
    void decompose(int32_t v);
    void clampAll();
    int32_t groupHi(uint8_t i) const;
    uint8_t idx() const;  // internal group of the selected one (letter group = sign group)

    FieldKind _kind = FieldKind::Time;
    uint8_t _group = 0;
    uint8_t _n = 3;
    bool _signed = false;
    int32_t _min = 0;
    int32_t _max = 0;
    int32_t _intHi = 0;  // Int: largest magnitude
    char _letters[2] = {};  // {pos, neg}; 0 = no letter mode
    int32_t _g[4] = {};
};

}  // namespace core
