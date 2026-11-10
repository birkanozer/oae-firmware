#include "core/ui/SkyMath.hpp"

#include <cmath>

#include "core/SiderealClock.hpp"

namespace core {

namespace {

constexpr float kDeg = 0.017453292519943295f;
constexpr float kSunLimitDeg = 15.0f;

float wrap360(float d)
{
    d = std::fmod(d, 360.0f);
    d = d < 0.0f ? d + 360.0f : d;
    return d >= 360.0f ? 0.0f : d;  // d + 360 can round up to exactly 360
}

}  // namespace

AltAz equatorialToHorizontal(float haHours, float decDeg, float latDeg)
{
    const float ha = haHours * 15.0f * kDeg;
    const float dec = decDeg * kDeg;
    const float lat = latDeg * kDeg;
    const float sinAlt = std::sin(dec) * std::sin(lat) + std::cos(dec) * std::cos(lat) * std::cos(ha);
    // Azimuth from north through east: atan2(-cos(dec) sin(ha), sin(dec) cos(lat) - cos(dec) cos(ha) sin(lat)).
    const float y = -std::cos(dec) * std::sin(ha);
    const float x = std::sin(dec) * std::cos(lat) - std::cos(dec) * std::cos(ha) * std::sin(lat);
    // atan2 keeps full precision near the zenith (asin does not) and propagates NaN instead of clamping it.
    const float alt = std::atan2(sinAlt, std::hypot(x, y));
    return {alt / kDeg, wrap360(std::atan2(y, x) / kDeg)};
}

void sunRaDec(int year, int month, int day, float utcHours, float &raHours, float &decDeg)
{
    // calculateDeltaJd counts days from 2000-01-01 0h UT; J2000.0 itself is 12h, hence the -0.5.
    const double n = SiderealClock::calculateDeltaJd(year, month, day) - 0.5 + utcHours / 24.0;
    const float L = wrap360(static_cast<float>(std::fmod(280.460 + 0.9856474 * n, 360.0)));
    const float g = wrap360(static_cast<float>(std::fmod(357.528 + 0.9856003 * n, 360.0))) * kDeg;
    const float lambda = (L + 1.915f * std::sin(g) + 0.020f * std::sin(2.0f * g)) * kDeg;
    const float eps = (23.439f - 0.0000004f * static_cast<float>(n)) * kDeg;
    raHours = wrap360(std::atan2(std::cos(eps) * std::sin(lambda), std::cos(lambda)) / kDeg) / 15.0f;
    decDeg = std::asin(std::sin(eps) * std::sin(lambda)) / kDeg;
}

float angularSeparationDeg(float ra1Hours, float dec1Deg, float ra2Hours, float dec2Deg)
{
    const float d1 = dec1Deg * kDeg;
    const float d2 = dec2Deg * kDeg;
    const float c = std::sin(d1) * std::sin(d2) + std::cos(d1) * std::cos(d2) * std::cos((ra1Hours - ra2Hours) * 15.0f * kDeg);
    return std::acos(std::fmax(-1.0f, std::fmin(1.0f, c))) / kDeg;
}

TargetCheck checkTarget(float raHours, float decDeg, float lstHours, float latDeg, float minAltDeg, int year, int month, int day,
                        float utcHours)
{
    // Fail closed: NaN/inf, out-of-range values or an impossible date must never read as Ok (NaN fails every comparison).
    const auto in = [](float v, float lo, float hi, bool hiInclusive) { return v >= lo && (hiInclusive ? v <= hi : v < hi); };
    if (!in(raHours, 0.0f, 24.0f, false) || !in(lstHours, 0.0f, 24.0f, false) || !in(utcHours, 0.0f, 24.0f, false)
        || !in(decDeg, -90.0f, 90.0f, true) || !in(latDeg, -90.0f, 90.0f, true) || !in(minAltDeg, -90.0f, 90.0f, true))
    {
        return TargetCheck::Invalid;
    }
    static const uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > kDays[month - 1] + (leap && month == 2 ? 1 : 0))
    {
        return TargetCheck::Invalid;
    }
    float sunRa, sunDec;
    sunRaDec(year, month, day, utcHours, sunRa, sunDec);
    if (angularSeparationDeg(raHours, decDeg, sunRa, sunDec) < kSunLimitDeg)
    {
        return TargetCheck::NearSun;
    }
    return equatorialToHorizontal(lstHours - raHours, decDeg, latDeg).altDeg < minAltDeg ? TargetCheck::BelowHorizon : TargetCheck::Ok;
}

}  // namespace core
