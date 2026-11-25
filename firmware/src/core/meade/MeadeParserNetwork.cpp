/**
 * @file MeadeParserNetwork.cpp
 * @brief `:XN...` network leaves: the ESP32 bridge reports its WiFi state, the Robin shows it on the LCD.
 *
 * The bridge is a trust boundary: every value is checked strictly (digits, ranges, separators, nothing
 * after the last field) before a handler is called. A malformed M or I command replies `0`.
 */

#include "MeadeParser.hpp"
#include "MeadeParserHelpers.hpp"

namespace oat
{
namespace core
{
namespace meade
{

namespace
{

// 1..maxDigits decimal digits at p, value at most maxValue; p moves past them. No sign, no overflow.
bool readUnsigned(const char *&p, int maxDigits, unsigned maxValue, unsigned &out)
{
    unsigned v = 0;
    int n      = 0;
    while (*p >= '0' && *p <= '9')
    {
        if (++n > maxDigits)
        {
            return false;
        }
        v = v * 10 + static_cast<unsigned>(*p - '0');
        ++p;
    }
    if (n == 0 || v > maxValue)
    {
        return false;
    }
    out = v;
    return true;
}

bool expect(const char *&p, char c)
{
    if (*p != c)
    {
        return false;
    }
    ++p;
    return true;
}

}  // namespace

void handleMeadeExtraNet(MeadeResponse &r, const char *leaf, IMeadeExtraHandlers &h)
{
    if (leaf == nullptr || leaf[0] == '\0')
    {
        return;
    }
    const char *p = leaf + 1;
    switch (leaf[0])
    {
        case 'M':  // <mode>,<clients>,<rssi>
        {
            unsigned mode = 0, clients = 0, rssi = 0;

            bool ok = readUnsigned(p, 1, 3, mode) && expect(p, ',') && readUnsigned(p, 1, 9, clients) && expect(p, ',');
            if (ok)
            {
                const bool negative = expect(p, '-');
                ok                  = readUnsigned(p, 3, 127, rssi) && *p == '\0' && (negative || rssi == 0);  // "-0" is 0
            }
            if (ok)
            {
                h.onNetStatus(static_cast<uint8_t>(mode), static_cast<uint8_t>(clients), static_cast<int8_t>(-static_cast<int>(rssi)));
            }
            fillSetSuccessResponse(r, ok);
            return;
        }
        case 'I':  // a.b.c.d
        {
            uint8_t ip[4];
            bool ok = true;
            for (int i = 0; i < 4 && ok; ++i)
            {
                unsigned v = 0;
                ok         = readUnsigned(p, 3, 255, v) && (i == 3 ? *p == '\0' : expect(p, '.'));
                ip[i]      = static_cast<uint8_t>(v);
            }
            if (ok)
            {
                h.onNetIp(ip);
            }
            fillSetSuccessResponse(r, ok);
            return;
        }
        case 'N':  // SSID: the first 20 bytes, bytes outside 0x20..0x7E become '?'. f_serial already drops bytes < 0x20 and treats 0x06 as ACK, and '#' ends the command, so the bridge must replace those itself
        {
            char ssid[21];
            int n = 0;
            for (; p[n] != '\0' && n < 20; ++n)
            {
                const unsigned char c = static_cast<unsigned char>(p[n]);
                ssid[n]               = (c >= 0x20 && c <= 0x7E) ? static_cast<char>(c) : '?';
            }
            ssid[n] = '\0';
            h.onNetName(ssid);
            fillSetSuccessResponse(r, true);
            return;
        }
        case 'G':
            if (leaf[1] == '\0')
            {
                fillIntResponse(r, h.onNetOwnWifi());
            }
            return;
        default:
            return;
    }
}

}  // namespace meade
}  // namespace core
}  // namespace oat
