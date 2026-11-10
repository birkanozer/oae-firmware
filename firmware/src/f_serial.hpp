#pragma once

#if TEST_VERIFY_MODE == 1
    #include "testmenu.hpp"
    #include "testmenudef.hpp"
#endif

#include "b_setup.hpp"

#if SUPPORT_SERIAL_CONTROL == 1
    #include "MeadeCommandProcessor.hpp"

// One partial command per port. len == sizeof(data) marks an over-long command being discarded up to its '#'.
struct SerialBuffer
{
    char    data[32];  // longest LX200 command is :SY (21 chars before #)
    uint8_t len;
};

void processSerialData();
void processSerialData(Stream &port, SerialBuffer &buf);

////////////////////////////////////////////////
// The main loop when under serial control
void serialLoop()
{
    mount.loop();
    mount.displayStepperPositionThrottled();

    #if defined(ESP32) || defined(ARDUINO_ARCH_STM32)
    processSerialData();
    #endif

    #if (WIFI_ENABLED == 1)
    wifiControl.loop();
    #endif

    #if TEST_VERIFY_MODE == 1
    mainTestMenu.tick();
    #endif
}

    //////////////////////////////////////////////////
    // Event that is triggered when the serial port receives data.
    #if !defined(ESP32) && !defined(ARDUINO_ARCH_STM32)
void serialEvent()
{
    processSerialData();
}
    #endif

    #if TEST_VERIFY_MODE == 1

void processTestState()
{
    static char buffer[32];
    static unsigned int index = 0;
    switch (TestMenu::getMenuState())
    {
        case testMenuState_t::CLEAR:
            TestMenu::setMenuState(testMenuState_t::WAITING_ON_INPUT);
            break;

        case testMenuState_t::WAITING_ON_INPUT:
            while (Serial.available() > 0)
            {
                if (Serial.readBytes(buffer, 1) == 1)
                {
                    if ((buffer[0] >= '0') && (buffer[0] <= '9'))
                    {
                        Serial.println(buffer[0]);
                        int pressedKey = buffer[0] - '0';
                        TestMenu::getCurrentMenu()->onKeyPressed(pressedKey);
                    }
                }
            }
            break;

        case testMenuState_t::WAITING_ON_COMMAND:
            while (Serial.available() > 0)
            {
                char ch;
                if (Serial.readBytes(&ch, 1) == 1)
                {
                    if (isascii(ch))
                    {
                        buffer[index] = ch;
                        if (ch == '#')
                        {
                            buffer[index + 1] = '\0';
                            TestMenu::getCurrentMenu()->onCommandReceived(buffer);
                            TestMenu::setMenuState(testMenuState_t::WAITING_ON_INPUT);
                            TestMenu::getCurrentMenu()->display();
                            index = 0;
                        }
                        else
                        {
                            index++;
                            if (index > ARRAY_SIZE(buffer) - 1)
                            {
                                Serial.println(F("Buffer overflow, too many chars received"));
                                index = 0;
                            }
                        }
                    }
                }
            }
            break;
    }
}

void processSerialData()
{
    processTestState();
}
    #else
// Handles the bytes waiting on one port. Each port owns its own buffer so two ports never share a partial command.
// At most 64 bytes per call, so a flooded port cannot starve the other port or the rest of loop(); the rest waits in the UART FIFO/ring.
void processSerialData(Stream &port, SerialBuffer &buf)
{
    for (uint8_t n = 0; n < 64 && port.available() > 0; n++)
    {
        const int ch = port.read();
        if (ch < 0)
        {
            break;
        }
        // While discarding an over-long command (len == sizeof(data)) only '#' is looked at, ACK included.
        if (ch == 0x06 && buf.len < sizeof(buf.data))
        {
            LOG(DEBUG_SERIAL, "[SERIAL]: Received: ACK request, replying P");
                // When not debugging, print the result to the serial port .
                // When debugging, only print the result to Serial if we're on seperate ports.
        #if (DEBUG_LEVEL == DEBUG_NONE) || (DEBUG_SEPARATE_SERIAL == 1)
            port.print('P');
        #elif defined(ARDUINO_ARCH_STM32)
            if (&port != &Serial)  // the bridge port never carries debug output
            {
                port.print('P');
            }
        #endif
            buf.len = 0;
        }
        else if (ch == '#')
        {
            if (buf.len >= sizeof(buf.data))
            {
                LOG(DEBUG_SERIAL, "[SERIAL]: Command buffer overflow! Ignoring received command.");
                buf.len = 0;
            }
            else
            {
                // Ignoring trailing hash
                buf.data[buf.len]  = '\0';
                const String inCmd = String(buf.data);
                buf.len            = 0;
                LOG(DEBUG_SERIAL, "[SERIAL]: ReceivedCommand(%d chars): [%s]", inCmd.length(), inCmd.c_str());

                const char *retVal = MeadeCommandProcessor::instance()->processCommand(inCmd);
                if (retVal[0] != '\0')
                {
                    LOG(DEBUG_SERIAL, "[SERIAL]: RepliedWith:  [%s]", retVal);
                        // When not debugging, print the result to the serial port .
                        // When debugging, only print the result to Serial if we're on seperate ports.
        #if (DEBUG_LEVEL == DEBUG_NONE) || (DEBUG_SEPARATE_SERIAL == 1)
                    port.print(retVal);
        #elif defined(ARDUINO_ARCH_STM32)
                    if (&port != &Serial)  // the bridge port never carries debug output
                    {
                        port.print(retVal);
                    }
        #endif
                }
            }
        }
        else if (ch >= ' ')
        {
            // len == sizeof(data) means "overflowed": drop everything up to the next '#', never store it.
            if (buf.len < sizeof(buf.data))
            {
                buf.data[buf.len++] = (char) ch;
                if (buf.len >= sizeof(buf.data))
                {
                    LOG(DEBUG_SERIAL, "[SERIAL]: Command buffer overflow! Ignoring received data.");
                }
            }
        }

        mount.loop();
    }
}

// ESP needs to call this in a loop :_(
void processSerialData()
{
    static SerialBuffer usbBuffer;
    processSerialData(Serial, usbBuffer);
        #if defined(ARDUINO_ARCH_STM32)
    // Commands are handled one complete command at a time (processCommand returns a shared reply buffer).
    static SerialBuffer bridgeBuffer;
    processSerialData(BRIDGE_SERIAL_PORT, bridgeBuffer);
        #endif
}

    #endif
#endif
