#pragma once

// Text helpers and timings shared by UiController.cpp and UiScreens.cpp (internal to the UI).

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "core/ui/Canvas.hpp"

namespace core {
namespace draw {

constexpr char kDeg[] = "\xC2\xB0";
constexpr uint32_t kMessageMs = 2000;  // footer message after a locked row
constexpr uint32_t kStepLatchMs = 300;  // an own step move counts as moving this long until the snapshot shows it
constexpr uint32_t kJogStopMs = 300;   // jog stops 300 ms after the last detent; older turns are dropped (M4)
constexpr uint32_t kArmMs = 600;       // GoTo confirm: GO armed 600 ms after the screen appeared (S4)

// Display columns: UTF-8 continuation bytes take no cell.
inline int cols(const char *s)
{
    int n = 0;
    for (; *s; ++s)
        n += (*s & 0xC0) != 0x80;
    return n;
}

inline void center(Canvas &c, Font f, int16_t y, const char *s, bool inverted)
{
    c.text(f, static_cast<int16_t>((128 - fontWidth(f) * cols(s) + 1) / 2), y, s, inverted);
}

inline void hms(char *out, size_t n, int32_t sec)
{
    snprintf(out, n, "%02d:%02d:%02d", (int)(sec / 3600), (int)(sec / 60 % 60), (int)(sec % 60));
}

inline void dms(char *out, size_t n, int32_t arcSec, bool withSeconds)
{
    const char sg = arcSec < 0 ? '-' : '+';
    const int32_t v = arcSec < 0 ? -arcSec : arcSec;
    if (withSeconds)
        snprintf(out, n, "%c%02d%s%02d'%02d\"", sg, (int)(v / 3600), kDeg, (int)(v / 60 % 60), (int)(v % 60));
    else
        snprintf(out, n, "%c%02d%s%02d'", sg, (int)(v / 3600), kDeg, (int)(v / 60 % 60));
}

// "label     value": value right-aligned at column 19 (a menu row), or just the label when it does not fit.
inline void row(char *out, size_t n, const char *label, const char *value)
{
    int pad = 19 - cols(label) - cols(value);
    if (pad < 1)
        pad = 1;
    snprintf(out, n, "%s%*s%s", label, pad, "", value);
}

}  // namespace draw
}  // namespace core
