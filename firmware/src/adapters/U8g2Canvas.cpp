#ifdef ARDUINO
    #include "../../Configuration.hpp"
#endif

#if USE_LCD_UI == 1

    #include "U8g2Canvas.hpp"

U8g2Canvas::U8g2Canvas() : _u8g2(U8G2_R0, LCD_SCK_PIN, LCD_DATA_PIN, LCD_CS_PIN)
{
}

void U8g2Canvas::begin()
{
    _u8g2.begin();
}

void U8g2Canvas::flush()
{
    _u8g2.sendBuffer();
}

void U8g2Canvas::clear()
{
    _u8g2.clearBuffer();
}

void U8g2Canvas::text(core::Font f, int16_t x, int16_t y, const char *s, bool inverted)
{
    // _tf fonts carry the degree sign; strings are UTF-8
    static const uint8_t *const kFonts[] = {u8g2_font_5x7_tf, u8g2_font_6x10_tf, u8g2_font_7x13B_tf, u8g2_font_10x20_tf};
    _u8g2.setFont(kFonts[static_cast<uint8_t>(f)]);
    _u8g2.setFontPosTop();
    if (inverted)
    {
        _u8g2.drawBox(x, y, _u8g2.getUTF8Width(s), core::fontHeight(f));
        _u8g2.setDrawColor(2);  // XOR: dark glyphs on the lit box
    }
    _u8g2.drawUTF8(x, y, s);
    _u8g2.setDrawColor(1);
}

void U8g2Canvas::box(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    _u8g2.drawBox(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

void U8g2Canvas::frame(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    _u8g2.drawFrame(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

void U8g2Canvas::dots(int16_t x0, int16_t x1, int16_t y)
{
    for (int16_t x = x0; x <= x1; x += 2)
    {
        _u8g2.drawPixel(x, y);
    }
}

#endif
