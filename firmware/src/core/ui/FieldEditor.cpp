#include "core/ui/FieldEditor.hpp"

#include <cstdint>
#include <cstdio>
#include <utility>

namespace core {

namespace {

struct Grp
{
    int32_t lo, hi, w;  // group range and its weight in the value
};

// Signed kinds start with a sign group (0 = +, 1 = -) whose weight is unused.
const Grp kTime[] = {{0, 23, 3600}, {0, 59, 60}, {0, 59, 1}};
const Grp kDec[] = {{0, 1, 0}, {0, 90, 3600}, {0, 59, 60}, {0, 59, 1}};
const Grp kLat[] = {{0, 1, 0}, {0, 90, 3600}, {0, 59, 60}};
const Grp kLon[] = {{0, 1, 0}, {0, 180, 3600}, {0, 59, 60}};
const Grp kDate[] = {{2000, 2099, 10000}, {1, 12, 100}, {1, 31, 1}};
const Grp kInt[] = {{0, 1, 0}, {0, 0, 1}};  // magnitude hi comes from _intHi

const Grp *table(FieldKind k)
{
    switch (k)
    {
        case FieldKind::Dec:
            return kDec;
        case FieldKind::Latitude:
            return kLat;
        case FieldKind::Longitude:
            return kLon;
        case FieldKind::Date:
            return kDate;
        case FieldKind::Int:
            return kInt;
        default:
            return kTime;
    }
}

int32_t daysIn(int32_t y, int32_t m)
{
    static const int8_t d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    if (m < 1 || m > 12)
        return 31;
    return (m == 2 && leap) ? 29 : d[m - 1];
}

int32_t absv(int32_t v)
{
    return v < 0 ? -v : v;
}

}  // namespace

uint8_t FieldEditor::idx() const
{
    return _letters[0] ? (_group == _n - 1 ? 0 : _group + 1) : _group;
}

int32_t FieldEditor::groupHi(uint8_t i) const
{
    if (_kind == FieldKind::Date && i == 2)
        return daysIn(_g[0], _g[1]);
    if (_kind == FieldKind::Int && i == 1)
        return _intHi;
    return table(_kind)[i].hi;
}

void FieldEditor::decompose(int32_t v)
{
    const Grp *t = table(_kind);
    if (_signed)
    {
        if (v != 0)
            _g[0] = v < 0;
        v = absv(v);
    }
    for (uint8_t i = _signed ? 1 : 0; i < _n; ++i)
    {
        _g[i] = v / t[i].w;
        v %= t[i].w;
    }
}

int32_t FieldEditor::value() const
{
    const Grp *t = table(_kind);
    int32_t v = 0;
    for (uint8_t i = _signed ? 1 : 0; i < _n; ++i)
        v += _g[i] * t[i].w;
    return (_signed && _g[0]) ? -v : v;
}

void FieldEditor::clampAll()
{
    if (_kind == FieldKind::Date)
    {
        _g[0] = _g[0] < 2000 ? 2000 : (_g[0] > 2099 ? 2099 : _g[0]);
        _g[1] = _g[1] < 1 ? 1 : (_g[1] > 12 ? 12 : _g[1]);
        const int32_t dim = daysIn(_g[0], _g[1]);
        _g[2] = _g[2] < 1 ? 1 : (_g[2] > dim ? dim : _g[2]);
    }
    const int32_t v = value();
    const int32_t c = v < _min ? _min : (v > _max ? _max : v);
    if (c != v)
        decompose(c);
    if (_signed && _min >= 0)
        _g[0] = 0;  // no negative values allowed, so no "-0" either
}

void FieldEditor::begin(FieldKind kind, int32_t value, int32_t min, int32_t max)
{
    _kind = kind;
    _group = 0;
    _letters[0] = _letters[1] = 0;
    _signed = kind == FieldKind::Dec || kind == FieldKind::Latitude || kind == FieldKind::Longitude || kind == FieldKind::Int;
    _n = kind == FieldKind::Dec ? 4 : (kind == FieldKind::Int || kind == FieldKind::TimeHM ? 2 : 3);
    switch (kind)
    {
        case FieldKind::Dec:
        case FieldKind::Latitude:
            _min = -324000, _max = 324000;
            break;
        case FieldKind::Longitude:
            _min = -648000, _max = 648000;
            break;
        case FieldKind::Date:
            _min = 20000101, _max = 20991231;
            break;
        case FieldKind::Int:
            if (min == INT32_MIN)
                min = -INT32_MAX;
            if (max == INT32_MIN)
                max = -INT32_MAX;
            if (min > max)
                std::swap(min, max);
            _min = min, _max = max;
            _intHi = absv(min) > absv(max) ? absv(min) : absv(max);
            break;
        default:
            _min = 0, _max = 86399;
            break;
    }
    if (kind == FieldKind::Latitude || kind == FieldKind::Longitude)
        value -= value % 60;  // minutes resolution
    for (int32_t &g : _g)
        g = 0;
    decompose(value < _min ? _min : (value > _max ? _max : value));
    clampAll();
}

void FieldEditor::turn(int8_t detents)
{
    const uint8_t i = idx();
    if (_signed && i == 0 && _min >= 0)
        return;  // no negative values allowed, so the sign stays +
    const int32_t lo = table(_kind)[i].lo;
    const int32_t n = groupHi(i) - lo + 1;
    _g[i] = lo + ((_g[i] - lo + detents) % n + n) % n;
    clampAll();
}

bool FieldEditor::click()
{
    if (_group + 1 < _n)
    {
        ++_group;
        return false;
    }
    return true;
}

bool FieldEditor::back()
{
    if (_group == 0)
        return true;
    --_group;
    return false;
}

void FieldEditor::setLetters(char pos, char neg)
{
    if (_kind != FieldKind::Latitude && _kind != FieldKind::Longitude)
        return;
    _letters[0] = pos, _letters[1] = neg;
}

void FieldEditor::format(char *out, size_t len) const
{
    const char sg = _g[0] ? '-' : '+';
    switch (_kind)
    {
        case FieldKind::Dec:
            snprintf(out, len, "%c%02d°%02d'%02d\"", sg, (int)_g[1], (int)_g[2], (int)_g[3]);
            break;
        case FieldKind::Latitude:
            if (_letters[0])
                snprintf(out, len, "%02d°%02d'%c", (int)_g[1], (int)_g[2], _letters[_g[0]]);
            else
                snprintf(out, len, "%c%02d*%02d", sg, (int)_g[1], (int)_g[2]);
            break;
        case FieldKind::Longitude:
            if (_letters[0])
                snprintf(out, len, "%03d°%02d'%c", (int)_g[1], (int)_g[2], _letters[_g[0]]);
            else
                snprintf(out, len, "%c%03d*%02d", sg, (int)_g[1], (int)_g[2]);
            break;
        case FieldKind::Date:
            snprintf(out, len, "%04d-%02d-%02d", (int)_g[0], (int)_g[1], (int)_g[2]);
            break;
        case FieldKind::Int:
            snprintf(out, len, "%c%d", sg, (int)_g[1]);
            break;
        case FieldKind::TimeHM:
            snprintf(out, len, "%02d:%02d", (int)_g[0], (int)_g[1]);
            break;
        default:
            snprintf(out, len, "%02d:%02d:%02d", (int)_g[0], (int)_g[1], (int)_g[2]);
            break;
    }
}

}  // namespace core
