#pragma once

#include <cstdint>

namespace core {

struct AltAz
{
    float altDeg;
    float azDeg;  // from north, eastward, 0..360
};

AltAz equatorialToHorizontal(float haHours, float decDeg, float latDeg);

/// Low-precision solar position (Astronomical Almanac), about 1 degree.
void sunRaDec(int year, int month, int day, float utcHours, float &raHours, float &decDeg);

float angularSeparationDeg(float ra1Hours, float dec1Deg, float ra2Hours, float dec2Deg);

enum class TargetCheck : uint8_t
{
    Ok,
    BelowHorizon,
    NearSun,
    Invalid  // non-finite input or impossible date; never Ok
};

/// Refuses invalid input, targets within 15 degrees of the Sun (wins) or below minAltDeg. Date/time are UTC.
TargetCheck checkTarget(float raHours, float decDeg, float lstHours, float latDeg, float minAltDeg, int year, int month, int day,
                        float utcHours);

}  // namespace core
