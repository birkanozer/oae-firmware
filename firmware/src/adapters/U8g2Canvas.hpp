#pragma once

#if USE_LCD_UI == 1

    #include "inc/Globals.hpp"

PUSH_NO_WARNINGS
    #include <U8g2lib.h>
POP_NO_WARNINGS

    #include "core/ui/Canvas.hpp"

// core::Canvas on the Ender-3 ST7920 LCD (U8g2, software SPI, full frame buffer).
class U8g2Canvas : public core::Canvas
{
  public:
    U8g2Canvas();
    void begin();
    void flush();  // sendBuffer(): tens of ms in software SPI, call only when the frame changed

    void clear() override;
    void text(core::Font f, int16_t x, int16_t y, const char *s, bool inverted) override;
    void box(int16_t x0, int16_t y0, int16_t x1, int16_t y1) override;
    void frame(int16_t x0, int16_t y0, int16_t x1, int16_t y1) override;
    void dots(int16_t x0, int16_t x1, int16_t y) override;

  private:
    U8G2_ST7920_128X64_F_SW_SPI _u8g2;
};

#endif
