#include <gtest/gtest.h>
#include <cstdlib>
#include "core/SupplySense.hpp"

using core::snapSupplyVolts;
using core::SupplyDivider;
using core::SupplyFilter;
using core::supplyMillivolts;

namespace {

const SupplyDivider kDiv{100000.0f, 4700.0f, 4700.0f, 1.0f};

// Forward model of the J8 node: counts for a supply of `vin` volts at a Vdd of `vdd` volts (one 12-bit sample).
uint32_t counts(float vin, float vdd = 3.3f)
{
    const float node = (vin / 100000.0f + vdd / 4700.0f) / (1 / 100000.0f + 1 / 4700.0f + 1 / 4700.0f);
    return static_cast<uint32_t>(node / vdd * 4095.0f + 0.5f);
}

}  // namespace

TEST(SupplySenseTest, ConvertsTheChoicesAssumingA3V3Rail)
{
    for (int v : {12, 15, 20, 24})
        EXPECT_NEAR(v * 1000, supplyMillivolts(counts(static_cast<float>(v)), 4095, kDiv), 60) << v;
}

TEST(SupplySenseTest, SummedSamplesGiveTheSameResult)
{
    EXPECT_NEAR(15000, supplyMillivolts(16 * counts(15.0f), 16 * 4095, kDiv), 60);
}

TEST(SupplySenseTest, CalibrationScales)
{
    const SupplyDivider cal{100000.0f, 4700.0f, 4700.0f, 1.05f};
    EXPECT_NEAR(15750, supplyMillivolts(counts(15.0f), 4095, cal), 60);
}

TEST(SupplySenseTest, NotFittedOrOutOfRangeIsZero)
{
    EXPECT_EQ(0, supplyMillivolts(4095, 4095, kDiv));  // no divider: the pull-up holds the pin at Vdd (~73 V)
    EXPECT_EQ(0, supplyMillivolts(0, 4095, kDiv));     // shorted pin: negative
    EXPECT_EQ(0, supplyMillivolts(counts(8.5f), 4095, kDiv));
    EXPECT_EQ(0, supplyMillivolts(counts(31.0f), 4095, kDiv));
    EXPECT_NE(0, supplyMillivolts(counts(9.2f), 4095, kDiv));
    EXPECT_NE(0, supplyMillivolts(counts(29.8f), 4095, kDiv));
}

TEST(SupplySenseTest, SnapWindows)
{
    EXPECT_EQ(0, snapSupplyVolts(0));
    EXPECT_EQ(0, snapSupplyVolts(10499));
    EXPECT_EQ(12, snapSupplyVolts(10500));
    EXPECT_EQ(12, snapSupplyVolts(14199));
    EXPECT_EQ(15, snapSupplyVolts(14200));
    EXPECT_EQ(15, snapSupplyVolts(16499));
    EXPECT_EQ(0, snapSupplyVolts(16500));  // gaps keep what is applied
    EXPECT_EQ(0, snapSupplyVolts(18499));
    EXPECT_EQ(20, snapSupplyVolts(18500));
    EXPECT_EQ(20, snapSupplyVolts(21499));
    EXPECT_EQ(0, snapSupplyVolts(21500));
    EXPECT_EQ(0, snapSupplyVolts(22499));
    EXPECT_EQ(24, snapSupplyVolts(22500));
    EXPECT_EQ(24, snapSupplyVolts(25499));
    EXPECT_EQ(0, snapSupplyVolts(25500));
}

TEST(SupplySenseTest, HysteresisAtTheSharedEdge)
{
    // Applied 12: 15 needs 0.3 V past the 14.2 V edge; the value itself stays up to the edge.
    EXPECT_EQ(12, snapSupplyVolts(14199, 12));
    EXPECT_EQ(0, snapSupplyVolts(14200, 12));
    EXPECT_EQ(0, snapSupplyVolts(14300, 12));
    EXPECT_EQ(0, snapSupplyVolts(14499, 12));
    EXPECT_EQ(15, snapSupplyVolts(14500, 12));
    // Applied 15: 12 needs 0.3 V below the edge.
    EXPECT_EQ(15, snapSupplyVolts(14200, 15));
    EXPECT_EQ(0, snapSupplyVolts(14199, 15));
    EXPECT_EQ(0, snapSupplyVolts(13901, 15));
    EXPECT_EQ(12, snapSupplyVolts(13900, 15));
    // Windows that do not touch the applied one are not shrunk.
    EXPECT_EQ(20, snapSupplyVolts(18500, 15));
    EXPECT_EQ(12, snapSupplyVolts(10500, 24));
}

TEST(SupplySenseTest, FilterSeedsThenSmooths)
{
    SupplyFilter f;
    EXPECT_EQ(0, f.millivolts());
    f.sample(0, 15000);
    EXPECT_EQ(15000, f.millivolts());  // the first reading seeds
    f.sample(500, 16000);
    EXPECT_GT(f.millivolts(), 15000);
    EXPECT_LT(f.millivolts(), 16000);
    f.sample(1000, 0);  // invalid: no reading
    EXPECT_EQ(0, f.millivolts());
}

TEST(SupplySenseTest, StableAfterTwoSeconds)
{
    SupplyFilter f;
    uint32_t t = 0;
    for (; t < 2000; t += 500)
    {
        f.sample(t, 15100);
        EXPECT_EQ(0, f.stableVolts()) << t;
    }
    f.sample(t, 15100);  // held for 2 s
    EXPECT_EQ(15, f.stableVolts());
    // The source changes to 20 V: nothing until 20 V held for 2 s (the smoothing takes a few samples to reach 18.5 V)
    const uint32_t change = t + 500;
    for (t = change; t < change + 10000; t += 500)
    {
        f.sample(t, 20100);
        if (f.stableVolts() == 20)
            break;
    }
    EXPECT_EQ(20, f.stableVolts());
    EXPECT_GE(t, change + 2000);
}

TEST(SupplySenseTest, InvalidOrOutOfBandResetsTheDebounce)
{
    SupplyFilter f;
    for (uint32_t t = 0; t <= 2000; t += 500)
        f.sample(t, 15000);
    ASSERT_EQ(15, f.stableVolts());
    f.sample(2500, 0);
    EXPECT_EQ(0, f.stableVolts());
    f.sample(3000, 15000);
    EXPECT_EQ(0, f.stableVolts());  // the 2 s start again
    f.sample(5000, 15000);
    EXPECT_EQ(15, f.stableVolts());
    SupplyFilter g;
    for (uint32_t t = 0; t <= 4000; t += 500)
        g.sample(t, 27000);  // valid but outside every window
    EXPECT_EQ(0, g.stableVolts());
    SupplyFilter h;
    for (uint32_t t = 0; t <= 4000; t += 500)
        h.sample(t, 17500);  // in the gap between 15 and 20 V
    EXPECT_EQ(0, h.stableVolts());
}

TEST(SupplySenseTest, ALeadAcidOrLiFePO4BatteryNeverToggles)
{
    // 12 V battery wandering 13.2-14.4 V while applied 12: never reports 15.
    SupplyFilter f;
    uint32_t t = 0;
    for (int i = 0; i < 80; ++i, t += 500)
    {
        const int32_t mv = 13200 + (i % 5) * 300;  // 13.2 .. 14.4
        f.sample(t, mv, 12);
        EXPECT_NE(15, f.stableVolts()) << i;
    }
    // The same battery seen with 15 applied settles back to 12 once it is clearly below the edge.
    SupplyFilter g;
    for (int i = 0; i < 12; ++i, t += 500)
        g.sample(t, 13200, 15);
    EXPECT_EQ(12, g.stableVolts());
}

TEST(SupplySenseTest, LeavesTheAppliedValueOnlyPastTheMargin)
{
    SupplyFilter f;
    uint32_t t = 0;
    for (int i = 0; i < 12; ++i, t += 500)
        f.sample(t, 14400, 12);
    EXPECT_EQ(0, f.stableVolts());  // 14.4 V: inside 15 but not 0.3 V past the edge
    for (int i = 0; i < 12; ++i, t += 500)
        f.sample(t, 14600, 12);
    EXPECT_EQ(15, f.stableVolts());
}
