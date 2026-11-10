#ifdef ARDUINO
    #include "../../Configuration.hpp"
#endif

#if USE_LCD_UI == 1

    #include "U8g2Canvas.hpp"

namespace
{
// Direct-register pin driver for U8g2's software SPI: clock, data and CS are single BSRR writes and the
// 1-bit delay is a DWT cycle wait. Interrupts stay enabled (stepper ISR and TMC SoftwareSerial keep running).
struct LcdPin
{
    GPIO_TypeDef *port;
    uint32_t mask;
};
LcdPin pinClk, pinData, pinCs;

LcdPin lcdPin(uint8_t arduinoPin)
{
    const PinName pn = digitalPinToPinName(arduinoPin);
    return {get_GPIO_Port(STM_PORT(pn)), static_cast<uint32_t>(STM_LL_GPIO_PIN(pn))};
}

inline void lcdWrite(const LcdPin &p, uint8_t level)
{
    p.port->BSRR = level ? p.mask : (p.mask << 16);
}

void lcdDelayNs(uint8_t argNs)
{
    if (argNs == 0 || !(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))  // counter not running: skip rather than hang
        return;
    const uint32_t arg  = argNs;
    const uint32_t half = LCD_SPI_HALF_PERIOD_NS;
    const uint32_t ns   = arg < half ? half : arg;  // U8g2 asks for 20 ns (setup) and 140 ns (pulse); both get a half period
    const uint32_t cycles = (ns * (F_CPU / 1000000UL) + 999) / 1000;
    const uint32_t start  = DWT->CYCCNT;
    while (DWT->CYCCNT - start < cycles)
    {
    }
}

uint8_t lcdGpioAndDelay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_GPIO_SPI_CLOCK:
            lcdWrite(pinClk, arg_int);
            return 1;
        case U8X8_MSG_GPIO_SPI_DATA:
            lcdWrite(pinData, arg_int);
            return 1;
        case U8X8_MSG_GPIO_CS:
            lcdWrite(pinCs, arg_int);
            return 1;
        case U8X8_MSG_DELAY_NANO:
            lcdDelayNs(arg_int);
            return 1;
        default:
            return u8x8_gpio_and_delay_arduino(u8x8, msg, arg_int, arg_ptr);  // init (pinMode), ms delays
    }
}
}  // namespace

U8g2Canvas::U8g2Canvas() : _u8g2(U8G2_R0, LCD_SCK_PIN, LCD_DATA_PIN, LCD_CS_PIN)
{
}

void U8g2Canvas::begin()
{
    _u8g2.getU8x8()->gpio_and_delay_cb = lcdGpioAndDelay;
    // Pins and cycle counter first: the display init already goes through the callback
    pinClk  = lcdPin(LCD_SCK_PIN);
    pinData = lcdPin(LCD_DATA_PIN);
    pinCs   = lcdPin(LCD_CS_PIN);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    _u8g2.begin();  // INIT message runs pinMode on the three pins
    _u8g2.setFontMode(1);  // transparent glyphs: inverted text is XORed onto a box and must not XOR its background
    _lastValid = false;
}

void U8g2Canvas::flush()
{
    #if LCD_SHOW_FRAME_MS
    _u8g2.setFont(u8g2_font_5x7_tf);
    _u8g2.setFontPosTop();
    char ms[8];
    snprintf(ms, sizeof(ms), "%lu", static_cast<unsigned long>((_lastFlushUs + 500) / 1000));
    _u8g2.setDrawColor(0);
    _u8g2.drawBox(128 - 5 * 4, 57, 20, 7);
    _u8g2.setDrawColor(1);
    _u8g2.drawStr(128 - 5 * strlen(ms), 57, ms);
    const uint32_t t0 = micros();
    #endif
    // Full buffer: 8 tile rows of 16 tiles x 8 bytes. The ST7920 driver addresses each pixel row itself and only needs
    // full-width (even tile count) areas, so any row range works.
    constexpr uint8_t kRowBytes = 128;
    constexpr uint8_t kRows     = 8;
    const uint8_t *buf          = _u8g2.getBufferPtr();
    uint8_t row                 = 0;
    while (row < kRows)
    {
        if (_lastValid && memcmp(buf + row * kRowBytes, _last + row * kRowBytes, kRowBytes) == 0)
        {
            row++;
            continue;
        }
        uint8_t end = row + 1;
        while (end < kRows && (!_lastValid || memcmp(buf + end * kRowBytes, _last + end * kRowBytes, kRowBytes) != 0))
            end++;
        _u8g2.updateDisplayArea(0, row, 16, end - row);
        row = end;
    }
    memcpy(_last, buf, sizeof(_last));
    _lastValid = true;
    #if LCD_SHOW_FRAME_MS
    _lastFlushUs = micros() - t0;
    #endif
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
