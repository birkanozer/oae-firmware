#include "../Configuration.hpp"
#include "Utility.hpp"

#if (BOARD == BOARD_STM32_MKS_ROBIN_E3) && (SW_SERIAL_UART == 1)                                                                           \
    && (RA_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART || DEC_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART                                          \
        || AZ_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART || ALT_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART                                       \
        || FOCUS_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)

PUSH_NO_WARNINGS
    #include <TMCStepper.h>
POP_NO_WARNINGS

// Single-wire TMC2209 UART on the MKS Robin E3: every PDN_UART line has a 20 kOhm pull-down on the
// board. TMCStepper's stock read path calls SoftwareSerial::listen() before sending the request,
// which in half-duplex mode switches the pin to input-pullup. The pull-down wins, so the line drops
// low for a few microseconds right before the sync byte, the TMC2209 takes that as a start bit and
// drops the whole request: reads never get a reply (writes have no listen() and always worked).
// These strong definitions replace TMCStepper's weak ones: keep the pin driven high while sending
// the 4-byte read request and only listen() once it is out. listen() waits for the last stop bit
// plus SoftwareSerial's switch delay, well inside the TMC2209's 8-bit SENDDELAY.

namespace
{
uint8_t readRequestBytesLeft = 0;
}

void TMC2208Stepper::preReadCommunication()
{
    if (SWSerial != nullptr)
    {
        readRequestBytesLeft = 4;  // sync, slave address, register, CRC
    }
    else if (HWSerial != nullptr && sswitch != nullptr)
    {
        sswitch->active();
    }
}

uint8_t TMC2208Stepper::serial_write(const uint8_t data)
{
    if (SWSerial != nullptr)
    {
        uint8_t written = SWSerial->write(data);
        if (readRequestBytesLeft != 0 && --readRequestBytesLeft == 0)
        {
            SWSerial->listen();
        }
        return written;
    }
    if (HWSerial != nullptr)
    {
        return HWSerial->write(data);
    }
    return 0;
}

#endif
