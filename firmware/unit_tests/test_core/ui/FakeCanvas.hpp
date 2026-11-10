#pragma once

#include <cstring>
#include <set>
#include <string>
#include <vector>
#include "core/ui/Canvas.hpp"

// Records every op and counts violations: anything outside the 128x64 screen, a line longer than its
// font's column limit (6x10: 21, menu rows at x=2: 19; 10x20: 12; 7x13: 18; 5x7: 25), and two texts on
// the same row that overlap (e.g. a header state running into the clock).
struct FakeCanvas : core::Canvas
{
    struct Text
    {
        core::Font font;
        int16_t x, y;
        std::string s;
        bool inverted;
    };
    std::vector<Text> texts;
    std::set<std::string> seen;  // every text ever drawn (survives clear())
    int ops = 0;
    int violations = 0;

    void clear() override
    {
        texts.clear();
        ops = 0;
    }
    void text(core::Font f, int16_t x, int16_t y, const char *s, bool inverted) override
    {
        ++ops;
        const int chars = cols(s);
        const int w = core::fontWidth(f);
        const int limit = f == core::Font::F6x10 ? (x == 2 ? 19 : 21) : (f == core::Font::F10x20 ? 12 : (f == core::Font::F7x13B ? 18 : 25));
        if (x < 0 || y < 0 || x + chars * w > 128 || y + core::fontHeight(f) > 64 || chars > limit)
            ++violations;
        for (const Text &o : texts)
            if (o.y == y && x < o.x + cols(o.s.c_str()) * core::fontWidth(o.font) && o.x < x + chars * w)
                ++violations;
        texts.push_back({f, x, y, s, inverted});
        seen.insert(s);
    }
    void box(int16_t x0, int16_t y0, int16_t x1, int16_t y1) override { rect(x0, y0, x1, y1); }
    void frame(int16_t x0, int16_t y0, int16_t x1, int16_t y1) override { rect(x0, y0, x1, y1); }
    void dots(int16_t x0, int16_t x1, int16_t y) override { rect(x0, y, x1, y); }
    bool has(const char *s) const
    {
        for (const Text &t : texts)
            if (t.s == s)
                return true;
        return false;
    }

    static int cols(const char *s)
    {
        int n = 0;
        for (; *s; ++s)
            n += (*s & 0xC0) != 0x80;  // UTF-8 continuation bytes take no cell
        return n;
    }

  private:
    void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
    {
        ++ops;
        if (x0 < 0 || y0 < 0 || x1 > 127 || y1 > 63 || x0 > x1 || y0 > y1)
            ++violations;
    }
};
