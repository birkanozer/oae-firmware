#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "core/ui/SkyMath.hpp"

using namespace core;

TEST(SkyMathTest, ZenithIsAlt90)
{
    EXPECT_NEAR(90.0f, equatorialToHorizontal(0.0f, 41.0f, 41.0f).altDeg, 0.01f);
}

TEST(SkyMathTest, PoleAltEqualsLatitude)
{
    const AltAz p = equatorialToHorizontal(3.0f, 90.0f, 41.0f);
    EXPECT_NEAR(41.0f, p.altDeg, 0.01f);
    EXPECT_NEAR(0.0f, p.azDeg < 180.0f ? p.azDeg : p.azDeg - 360.0f, 0.01f);
}

TEST(SkyMathTest, EastHorizon)
{
    const AltAz p = equatorialToHorizontal(-6.0f, 0.0f, 0.0f);
    EXPECT_NEAR(0.0f, p.altDeg, 0.1f);
    EXPECT_NEAR(90.0f, p.azDeg, 0.1f);
}

TEST(SkyMathTest, SunAtMarchEquinox2026)
{
    float ra, dec;
    sunRaDec(2026, 3, 20, 14.0f + 46.0f / 60.0f, ra, dec);
    EXPECT_NEAR(0.0f, ra < 12.0f ? ra : ra - 24.0f, 1.0f / 15.0f);
    EXPECT_NEAR(0.0f, dec, 1.0f);
}

TEST(SkyMathTest, SunAtJuneSolstice2026)
{
    float ra, dec;
    sunRaDec(2026, 6, 21, 8.0f + 24.0f / 60.0f, ra, dec);
    EXPECT_NEAR(23.44f, dec, 1.0f);
    EXPECT_NEAR(6.0f, ra, 1.0f / 15.0f);
}

TEST(SkyMathTest, SeparationPoles)
{
    EXPECT_NEAR(90.0f, angularSeparationDeg(0.0f, 90.0f, 5.0f, 0.0f), 0.01f);
}

// Sun on 2026-06-21 08:24 UTC is near RA 6 h, Dec +23.4; lat 41, lst 12 puts targets well up.
TEST(SkyMathTest, CheckRefusesNearSun)
{
    EXPECT_EQ(TargetCheck::NearSun, checkTarget(6.0f, 23.0f, 12.0f, 41.0f, 0.0f, 2026, 6, 21, 8.4f));
    // NearSun wins even when the target is below the horizon (lat -60, ha 12 h).
    EXPECT_EQ(TargetCheck::NearSun, checkTarget(6.0f, 23.0f, 18.0f, -60.0f, 0.0f, 2026, 6, 21, 8.4f));
}

TEST(SkyMathTest, CheckRefusesBelowMinAlt)
{
    // Dec -80 from lat 41 never rises.
    EXPECT_EQ(TargetCheck::BelowHorizon, checkTarget(18.0f, -80.0f, 18.0f, 41.0f, 0.0f, 2026, 6, 21, 8.4f));
    // Above horizon but under a raised minimum: Polaris-ish, alt ~41 < 50.
    EXPECT_EQ(TargetCheck::BelowHorizon, checkTarget(2.0f, 89.0f, 2.0f, 41.0f, 50.0f, 2026, 6, 21, 8.4f));
}

TEST(SkyMathTest, CheckOk)
{
    // Target at zenith of lat 41, opposite the Sun in RA.
    EXPECT_EQ(TargetCheck::Ok, checkTarget(18.0f, 41.0f, 18.0f, 41.0f, 0.0f, 2026, 6, 21, 8.4f));
}

TEST(SkyMathTest, WestHorizonAndSouthernAzimuth)
{
    const AltAz w = equatorialToHorizontal(6.0f, 0.0f, 0.0f);
    EXPECT_NEAR(0.0f, w.altDeg, 0.1f);
    EXPECT_NEAR(270.0f, w.azDeg, 0.1f);
    const AltAz s = equatorialToHorizontal(0.0f, -60.0f, -30.0f);  // south of zenith for a southern observer
    EXPECT_NEAR(60.0f, s.altDeg, 0.01f);
    EXPECT_NEAR(180.0f, s.azDeg, 0.01f);
}

TEST(SkyMathTest, AzimuthNeverReaches360)
{
    for (float ha = -12.0f; ha < 12.0f; ha += 0.013f)
    {
        EXPECT_LT(equatorialToHorizontal(ha, 10.0f, 41.0f).azDeg, 360.0f);
    }
}

TEST(SkyMathTest, SunInNovember2026)
{
    float ra, dec;
    sunRaDec(2026, 11, 10, 20.0f, ra, dec);
    EXPECT_NEAR(15.07f, ra, 1.0f / 15.0f);
    EXPECT_NEAR(-17.3f, dec, 1.0f);
}

TEST(SkyMathTest, NonFiniteInputsAreInvalid)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float bad[] = {nan, inf, -inf};
    for (const float b : bad)
    {
        EXPECT_EQ(TargetCheck::Invalid, checkTarget(b, 0.0f, 18.0f, 41.0f, 0.0f, 2026, 6, 21, 8.4f));
        EXPECT_EQ(TargetCheck::Invalid, checkTarget(18.0f, b, 18.0f, 41.0f, 0.0f, 2026, 6, 21, 8.4f));
        EXPECT_EQ(TargetCheck::Invalid, checkTarget(18.0f, 41.0f, b, 41.0f, 0.0f, 2026, 6, 21, 8.4f));
        EXPECT_EQ(TargetCheck::Invalid, checkTarget(18.0f, 41.0f, 18.0f, b, 0.0f, 2026, 6, 21, 8.4f));
        EXPECT_EQ(TargetCheck::Invalid, checkTarget(18.0f, 41.0f, 18.0f, 41.0f, b, 2026, 6, 21, 8.4f));
        EXPECT_EQ(TargetCheck::Invalid, checkTarget(18.0f, 41.0f, 18.0f, 41.0f, 0.0f, 2026, 6, 21, b));
    }
    EXPECT_TRUE(std::isnan(equatorialToHorizontal(nan, 0.0f, 41.0f).altDeg));
}

TEST(SkyMathTest, ImpossibleDatesAreInvalid)
{
    // Target exactly at the Sun would be NearSun on a valid date.
    EXPECT_EQ(TargetCheck::Invalid, checkTarget(6.0f, 23.0f, 12.0f, 41.0f, 0.0f, 2026, 13, 21, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, checkTarget(6.0f, 23.0f, 12.0f, 41.0f, 0.0f, 2026, 0, 21, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, checkTarget(6.0f, 23.0f, 12.0f, 41.0f, 0.0f, 2026, 2, 29, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, checkTarget(6.0f, 23.0f, 12.0f, 41.0f, 0.0f, 2026, 6, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, checkTarget(6.0f, 23.0f, 12.0f, 41.0f, 0.0f, 1999, 6, 21, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, checkTarget(18.0f, 41.0f, 18.0f, 41.0f, 0.0f, 2028, 2, 29, 8.4f));
}

TEST(SkyMathTest, OutOfRangeInputsAreInvalid)
{
    // checkTarget(ra, dec, lst, lat, minAlt, ..., utc); defaults are a valid Ok case.
    auto chk = [](float ra, float dec, float lst, float lat, float minAlt, float utc)
    { return checkTarget(ra, dec, lst, lat, minAlt, 2026, 6, 21, utc); };
    EXPECT_EQ(TargetCheck::Invalid, chk(-0.01f, 41, 18, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(24.0f, 41, 18, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(1e9f, 41, 18, 41, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(0.0f, 41, 18, 41, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(23.99f, 41, 18, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 90.01f, 18, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, -90.01f, 18, 41, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 90, 18, 41, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, -90, 18, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, -0.01f, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 24.0f, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 1e9f, 41, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 0.0f, 41, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, 90.01f, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, -90.01f, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 18, 90, 0, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 18, -90, 0, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, 41, -90.01f, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, 41, 90.01f, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 18, 41, -90, 8.4f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 18, 41, 90, 8.4f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, 41, 0, -0.01f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, 41, 0, 24.0f));
    EXPECT_EQ(TargetCheck::Invalid, chk(18, 41, 18, 41, 0, 1e9f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 18, 41, 0, 0.0f));
    EXPECT_NE(TargetCheck::Invalid, chk(18, 41, 18, 41, 0, 23.99f));
}
