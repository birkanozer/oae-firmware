#pragma once

#include <cstdint>

namespace core {

enum class Font : uint8_t
{
    F5x7,
    F6x10,
    F7x13B,
    F10x20
};

constexpr uint8_t fontWidth(Font f)
{
    return f == Font::F5x7 ? 5 : (f == Font::F6x10 ? 6 : (f == Font::F7x13B ? 7 : 10));
}

constexpr uint8_t fontHeight(Font f)
{
    return f == Font::F5x7 ? 7 : (f == Font::F6x10 ? 10 : (f == Font::F7x13B ? 13 : 20));
}

// 128x64 monochrome drawing surface. Coordinates are pixels; text y is the top of the cell;
// inverted draws light-on-dark inside the text's own cell box. box fills, frame outlines
// (both inclusive of x1/y1), dots draws a dotted horizontal line.
struct Canvas
{
    virtual void clear() = 0;
    virtual void text(Font f, int16_t x, int16_t y, const char *s, bool inverted) = 0;
    virtual void box(int16_t x0, int16_t y0, int16_t x1, int16_t y1) = 0;
    virtual void frame(int16_t x0, int16_t y0, int16_t x1, int16_t y1) = 0;
    virtual void dots(int16_t x0, int16_t x1, int16_t y) = 0;

  protected:
    ~Canvas() = default;
};

}  // namespace core
