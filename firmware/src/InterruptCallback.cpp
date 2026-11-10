#include "../Configuration.hpp"
#include "Utility.hpp"
#include "InterruptCallback.hpp"

//////////////////////////////////////
// This is an hardware-independent abstraction layer over
// whatever timer is used for the hardware being run
//////////////////////////////////////

#ifndef NEW_STEPPER_LIB

    #if defined ESP32
    // We don't support ESP32 boards in interrupt mode
    #elif defined __AVR_ATmega2560__  // Arduino Mega
        #define USE_TIMER_1 true
        #define USE_TIMER_2 true
        #define USE_TIMER_3 false
        #define USE_TIMER_4 false
        #define USE_TIMER_5 false
PUSH_NO_WARNINGS
        #include "libs/TimerInterrupt/TimerInterrupt.h"
POP_NO_WARNINGS
    #elif defined(ARDUINO_ARCH_RP2040)
        // Earle Philhower arduino-pico core exposes the Pico SDK alarm API.
        // add_repeating_timer_us() fires a hardware-alarm callback at a precise interval in
        // interrupt context, like the AVR timers. (The ESP32 port instead steps from a FreeRTOS
        // task; this core has no RTOS at all, and no second core needs to be dedicated.)
        #include <hardware/timer.h>
    #elif defined(ARDUINO_ARCH_STM32)
        // STM32duino HardwareTimer on basic timer TIM6 (TIM7 is taken by SoftwareSerial).
        #include <HardwareTimer.h>
    #else
        #error Unrecognized board selected. Either implement interrupt code or define the board here.
    #endif

    #if defined(ESP32)

    #elif defined __AVR_ATmega2560__

bool InterruptCallback::setInterval(float intervalMs, interrupt_callback_p callback, void *payload)
{
    // We have requested to use Timer2 (see above)
    ITimer2.init();

    // This timer supports the callback with payload
    return ITimer2.attachInterruptInterval<void *>(intervalMs, callback, payload, 0UL);
}

void InterruptCallback::stop()
{
    ITimer2.stopTimer();
}

void InterruptCallback::start()
{
    ITimer2.restartTimer();
}

    #elif defined(ARDUINO_ARCH_RP2040)

// Storage for the repeating timer handle and the user callback+payload.
static repeating_timer_t _rp2040_timer;
static interrupt_callback_p _rp2040_callback = nullptr;
static void *_rp2040_payload                 = nullptr;

// The Pico SDK alarm callback signature returns bool; returning true keeps
// the timer repeating.
static bool rp2040_timer_isr(repeating_timer_t * /* rt */)
{
    if (_rp2040_callback)
    {
        _rp2040_callback(_rp2040_payload);
    }
    return true;  // Keep repeating
}

bool InterruptCallback::setInterval(float intervalMs, interrupt_callback_p callback, void *payload)
{
    _rp2040_callback = callback;
    _rp2040_payload  = payload;

    // Negative delay_us gives a fixed wall-clock period from callback start to
    // callback start (matching the AVR TimerInterrupt semantics). A positive
    // delay would add the callback's own runtime to every period.
    int64_t intervalUs = -static_cast<int64_t>(intervalMs * 1000.0f);
    return add_repeating_timer_us(intervalUs, rp2040_timer_isr, nullptr, &_rp2040_timer);
}

void InterruptCallback::stop()
{
    cancel_repeating_timer(&_rp2040_timer);
}

void InterruptCallback::start()
{
    // Re-arm with the same interval. The stored callback/payload are still set.
    add_repeating_timer_us(_rp2040_timer.delay_us, rp2040_timer_isr, nullptr, &_rp2040_timer);
}

    #elif defined(ARDUINO_ARCH_STM32)

// The period must be a whole number of microseconds, and >= 50 us so the ISR cannot starve loop().
constexpr float kStepperIntervalUs = STEPPER_CONTROL_INTERVAL_MS * 1000.0f;
constexpr uint32_t kStepperTicksUs = static_cast<uint32_t>(kStepperIntervalUs + 0.5f);
static_assert(kStepperTicksUs >= 50, "stepper interval below 50 us");
static_assert(kStepperTicksUs - kStepperIntervalUs < 0.01f && kStepperIntervalUs - kStepperTicksUs < 0.01f,
              "stepper interval must be a whole number of microseconds");

static HardwareTimer *_stm32_timer          = nullptr;
static interrupt_callback_p _stm32_callback = nullptr;
static void *_stm32_payload                 = nullptr;

static void stm32_timer_isr()
{
    if (_stm32_callback)
    {
        _stm32_callback(_stm32_payload);
    }
}

bool InterruptCallback::setInterval(float intervalMs, interrupt_callback_p callback, void *payload)
{
    _stm32_callback = callback;
    _stm32_payload  = payload;

    if (_stm32_timer == nullptr)
    {
        _stm32_timer = new HardwareTimer(TIM6);
        // Core 4.30000.0 gives every HardwareTimer (incl. SoftwareSerial's TIM7) priority 14, and
        // main.cpp selects NVIC group 4 (no sub-priority), so equal priorities never preempt.
        // Drop the stepper to 15 so a 19200 baud SoftwareSerial bit tick (17 us) is never held
        // up by the stepper ISR.
        _stm32_timer->setInterruptPriority(15, 0);
    }
    _stm32_timer->setOverflow(static_cast<uint32_t>(intervalMs * 1000.0f + 0.5f), MICROSEC_FORMAT);
    _stm32_timer->attachInterrupt(stm32_timer_isr);
    _stm32_timer->resume();
    return true;
}

void InterruptCallback::stop()
{
    _stm32_timer->pause();
}

void InterruptCallback::start()
{
    _stm32_timer->resume();
}

    #endif
#endif