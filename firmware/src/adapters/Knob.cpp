#ifdef ARDUINO
    #include "../../Configuration.hpp"
#endif

#if USE_LCD_UI == 1

    #include "Knob.hpp"

namespace
{
constexpr uint32_t kDebounceMs   = 30;
constexpr uint32_t kLongPressMs  = 700;
constexpr uint32_t kFastGapMs    = 40;
constexpr uint8_t kFastFactor    = 5;
constexpr uint16_t kToneHz       = 2000;
constexpr uint32_t kToneGapMs    = 40;
constexpr uint16_t kTickMs       = 20;
constexpr uint16_t kErrorMs      = 60;
constexpr uint16_t kLongMs       = 400;
}  // namespace

volatile int16_t Knob::_count = 0;
volatile uint8_t Knob::_prev  = 0;

void Knob::isr()
{
    // index = old AB << 2 | new AB; one valid quadrature step is +-1, anything else (bounce) 0
    static const int8_t kStep[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    _prev = ((_prev << 2) | (digitalRead(KNOB_A_PIN) << 1) | digitalRead(KNOB_B_PIN)) & 0x0F;
    _count += kStep[_prev];
}

void Knob::begin()
{
    pinMode(KNOB_A_PIN, INPUT_PULLUP);
    pinMode(KNOB_B_PIN, INPUT_PULLUP);
    pinMode(KNOB_CLICK_PIN, INPUT_PULLUP);
    _prev = (digitalRead(KNOB_A_PIN) << 1) | digitalRead(KNOB_B_PIN);
    attachInterrupt(digitalPinToInterrupt(KNOB_A_PIN), isr, CHANGE);  // EXTI line 11
    attachInterrupt(digitalPinToInterrupt(KNOB_B_PIN), isr, CHANGE);  // EXTI line 0
}

void Knob::update(uint32_t nowMs, core::UiController &ui)
{
    noInterrupts();
    const int16_t counts = _count;
    _count               = 0;
    interrupts();

    _remainder += counts;
    int detents = _remainder / ENCODER_COUNTS_PER_DETENT;
    _remainder -= detents * ENCODER_COUNTS_PER_DETENT;
    if (detents != 0)
    {
    #if ENCODER_INVERT
        detents = -detents;
    #endif
        if (_lastDetentMs != 0 && nowMs - _lastDetentMs < kFastGapMs)
        {
            detents *= kFastFactor;
        }
        _lastDetentMs = nowMs;
        detents       = constrain(detents, -127, 127);
        // Stamped with the previous poll: if the loop was blocked, the turn is as old as the block and the UI drops it
        ui.onTurn(static_cast<int8_t>(detents), _lastPollMs);
    }
    _lastPollMs = nowMs;

    const bool raw = digitalRead(KNOB_CLICK_PIN) == LOW;
    if (raw != _rawDown)
    {
        _rawDown    = raw;
        _rawSinceMs = nowMs;
    }
    if (_rawDown != _down && nowMs - _rawSinceMs >= kDebounceMs)
    {
        _down = _rawDown;
        if (_down)
        {
            _downAtMs  = nowMs;
            _longFired = false;
        }
        else if (!_longFired)
        {
            ui.onClick(nowMs);
        }
    }
    if (_down && !_longFired && nowMs - _downAtMs >= kLongPressMs)
    {
        _longFired = true;
        ui.onLongPress(nowMs);
    }

    playNext(nowMs);
}

void Knob::beep(core::UiAction::Kind kind)
{
    const uint8_t n = kind == core::UiAction::Kind::BeepError ? 2 : 1;
    const uint16_t ms = kind == core::UiAction::Kind::BeepLong ? kLongMs : (kind == core::UiAction::Kind::BeepError ? kErrorMs : kTickMs);
    for (uint8_t i = 0; i < n && _qCount < sizeof(_queue) / sizeof(_queue[0]); ++i)
    {
        _queue[(_qHead + _qCount++) % (sizeof(_queue) / sizeof(_queue[0]))] = ms;
    }
}

void Knob::playNext(uint32_t nowMs)
{
    // One tone at a time: tone() restarts the timer, so overlapping calls would cut each other off
    if (_qCount == 0 || static_cast<int32_t>(nowMs - _toneFreeAtMs) < 0)
    {
        return;
    }
    const uint16_t ms = _queue[_qHead];
    _qHead            = (_qHead + 1) % (sizeof(_queue) / sizeof(_queue[0]));
    --_qCount;
    tone(BEEPER_PIN, kToneHz, ms);
    _toneFreeAtMs = nowMs + ms + kToneGapMs;
}

#endif
