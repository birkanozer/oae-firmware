#pragma once

#if USE_LCD_UI == 1

    #include "inc/Globals.hpp"
    #include "core/ui/UiController.hpp"

// Ender-3 encoder (EXTI quadrature), click button and buzzer. The ISR only updates a counter;
// everything else runs from update() in the main loop.
class Knob
{
  public:
    void begin();
    void update(uint32_t nowMs, core::UiController &ui);  // turns, click, long press, queued beeps
    void beep(core::UiAction::Kind kind);                 // Beep, BeepError or BeepLong

  private:
    static void isr();
    void playNext(uint32_t nowMs);

    static volatile int16_t _count;
    static volatile uint8_t _prev;

    int16_t _remainder        = 0;
    uint32_t _lastPollMs      = 0;
    uint32_t _lastDetentMs    = 0;
    bool _rawDown             = false;
    bool _down                = false;
    bool _longFired           = false;
    uint32_t _rawSinceMs      = 0;
    uint32_t _downAtMs        = 0;
    uint16_t _queue[16];  // tone durations in ms
    uint8_t _qHead = 0, _qCount = 0;
    uint32_t _toneFreeAtMs = 0;
};

#endif
