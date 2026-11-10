#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include "core/ui/FieldEditor.hpp"

namespace {

using core::FieldEditor;
using core::FieldKind;

std::string fmt(const FieldEditor &e)
{
    char buf[24];
    e.format(buf, sizeof(buf));
    return buf;
}

}  // namespace

TEST(FieldEditorTest, TimeWrapsWithinGroup)
{
    FieldEditor e;
    e.begin(FieldKind::Time, 12 * 3600 + 34 * 60 + 59);
    e.click();
    e.click();  // seconds
    e.turn(1);
    EXPECT_EQ(12 * 3600 + 34 * 60, e.value());
    e.turn(-1);
    EXPECT_EQ(12 * 3600 + 34 * 60 + 59, e.value());
}

TEST(FieldEditorTest, RaMaxIs235959)
{
    FieldEditor e;
    e.begin(FieldKind::RA, 86399);
    EXPECT_EQ("23:59:59", fmt(e));
    e.turn(1);  // hours 23 -> 0, no carry, never past 23:59:59
    EXPECT_EQ(3599, e.value());
    e.turn(-1);
    EXPECT_EQ(86399, e.value());
    EXPECT_FALSE(e.click());
    EXPECT_FALSE(e.click());
    e.turn(1);  // seconds 59 -> 0
    EXPECT_EQ(86340, e.value());
    e.begin(FieldKind::RA, 999999);
    EXPECT_EQ(86399, e.value());
    e.begin(FieldKind::RA, -5);
    EXPECT_EQ(0, e.value());
}

TEST(FieldEditorTest, DecSignGroupFlips)
{
    FieldEditor e;
    e.begin(FieldKind::Dec, 5 * 3600);
    EXPECT_EQ(0, e.group());
    e.turn(1);
    EXPECT_EQ(-5 * 3600, e.value());
    e.turn(1);
    EXPECT_EQ(5 * 3600, e.value());
    e.begin(FieldKind::Dec, 0);
    e.turn(1);
    EXPECT_EQ("-00\xC2\xB0"
              "00'00\"", fmt(e));
}

TEST(FieldEditorTest, DecClampsAt90)
{
    FieldEditor e;
    e.begin(FieldKind::Dec, 89 * 3600 + 59 * 60 + 59);
    e.turn(1);  // sign flips
    e.turn(1);  // back to +
    e.click();
    e.turn(1);  // degrees 89 -> 90, minutes/seconds must clamp to 0
    EXPECT_EQ(324000, e.value());
    e.click();
    e.turn(1);  // minutes 0 -> 1 would be > 90 deg
    EXPECT_EQ(324000, e.value());
    e.click();
    e.turn(1);
    EXPECT_EQ(324000, e.value());
    e.begin(FieldKind::Dec, -400000);
    EXPECT_EQ(-324000, e.value());
}

TEST(FieldEditorTest, LatitudeClampsAt90)
{
    FieldEditor e;
    e.begin(FieldKind::Latitude, 89 * 3600 + 59 * 60 + 30);  // seconds dropped
    EXPECT_EQ(89 * 3600 + 59 * 60, e.value());
    EXPECT_FALSE(e.click());
    e.turn(1);  // degrees 90 with 59 minutes -> clamp
    EXPECT_EQ(324000, e.value());
    EXPECT_FALSE(e.click());  // minutes
    e.turn(1);
    EXPECT_EQ(324000, e.value());
    EXPECT_TRUE(e.click());
    // negative side
    e.begin(FieldKind::Latitude, -(89 * 3600 + 59 * 60));
    e.click();
    e.turn(1);
    EXPECT_EQ(-324000, e.value());
    e.begin(FieldKind::Latitude, 500000);
    EXPECT_EQ(324000, e.value());
}

TEST(FieldEditorTest, LongitudeClampsAt180)
{
    FieldEditor e;
    e.begin(FieldKind::Longitude, 179 * 3600 + 59 * 60);
    e.click();
    e.turn(1);
    EXPECT_EQ(648000, e.value());
    e.click();
    e.turn(1);
    EXPECT_EQ(648000, e.value());
    e.begin(FieldKind::Longitude, -(179 * 3600 + 59 * 60));
    e.click();
    e.turn(1);
    EXPECT_EQ(-648000, e.value());
    e.begin(FieldKind::Longitude, -(74 * 3600));
    EXPECT_EQ("-074*00", fmt(e));
}

TEST(FieldEditorTest, DateFeb29LeapYear)
{
    FieldEditor e;
    e.begin(FieldKind::Date, 20280228);
    e.click();
    e.click();  // day
    e.turn(1);
    EXPECT_EQ(20280229, e.value());
    e.turn(1);  // wraps within 29 days
    EXPECT_EQ(20280201, e.value());

    e.begin(FieldKind::Date, 20260229);
    EXPECT_EQ(20260228, e.value());
    e.begin(FieldKind::Date, 20280229);
    e.turn(-1);  // year 2027: day clamps
    EXPECT_EQ(20270228, e.value());
    e.begin(FieldKind::Date, 20260131);
    e.click();
    e.turn(1);  // February: day clamps
    EXPECT_EQ(20260228, e.value());
}

TEST(FieldEditorTest, DateBeginNormalises)
{
    FieldEditor e;
    for (int32_t bad : {20261350, 0, 20260100, 20260000, 20260230, 99999999, -5})
    {
        e.begin(FieldKind::Date, bad);
        const int32_t v = e.value();
        EXPECT_GE(v, 20000101) << bad;
        EXPECT_LE(v, 20991231) << bad;
        EXPECT_GE(v / 100 % 100, 1) << bad;
        EXPECT_LE(v / 100 % 100, 12) << bad;
        EXPECT_GE(v % 100, 1) << bad;
        EXPECT_LE(v % 100, 31) << bad;
    }
    e.begin(FieldKind::Date, 20260230);
    EXPECT_EQ(20260228, e.value());
    e.begin(FieldKind::Date, 20260431);
    EXPECT_EQ(20260430, e.value());  // 30-day month
    e.begin(FieldKind::Date, 0);
    EXPECT_EQ(20000101, e.value());
}

TEST(FieldEditorTest, IntMinAtLeastZero)
{
    FieldEditor e;
    e.begin(FieldKind::Int, 5, 1, 10);
    e.turn(1);  // sign group cannot make it negative
    EXPECT_EQ(5, e.value());
    EXPECT_EQ("+5", fmt(e));
    e.click();
    e.turn(5);
    EXPECT_EQ(10, e.value());
    e.turn(1);  // wraps within 0..10, then clamps up to min 1
    EXPECT_EQ(1, e.value());
    e.begin(FieldKind::Int, 3, 10, 1);  // swapped bounds
    EXPECT_EQ(3, e.value());
    e.begin(FieldKind::Int, 0, INT32_MIN, 5);
    EXPECT_LE(e.value(), 5);
}

TEST(FieldEditorTest, IntNegativeStartWithNonNegativeMin)
{
    FieldEditor e;
    e.begin(FieldKind::Int, -5, 0, 10);
    EXPECT_EQ(0, e.value());
    EXPECT_EQ("+0", fmt(e));
    e.click();
    e.turn(1);
    EXPECT_EQ(1, e.value());
    EXPECT_EQ("+1", fmt(e));
    e.begin(FieldKind::Int, INT32_MIN, -12, 14);
    EXPECT_EQ(-12, e.value());
    e.begin(FieldKind::Int, INT32_MAX, -12, 14);
    EXPECT_EQ(14, e.value());
    e.begin(FieldKind::Dec, INT32_MIN);
    EXPECT_EQ(-324000, e.value());
}

TEST(FieldEditorTest, IntRespectsMinMax)
{
    FieldEditor e;
    e.begin(FieldKind::Int, 3, -12, 14);
    EXPECT_EQ("+3", fmt(e));
    e.click();
    e.turn(11);  // 14
    EXPECT_EQ(14, e.value());
    e.turn(1);  // wraps to 0, never 15
    EXPECT_EQ(0, e.value());
    e.turn(-1);  // 14
    e.turn(-2);  // 12
    EXPECT_EQ(12, e.value());
    for (int i = 0; i < 40; ++i)
    {
        e.turn(1);
        EXPECT_GE(e.value(), -12);
        EXPECT_LE(e.value(), 14);
    }
    e.begin(FieldKind::Int, -12, -12, 14);
    e.click();
    e.turn(-1);
    EXPECT_GE(e.value(), -12);
    e.begin(FieldKind::Int, 14, -12, 14);
    e.turn(1);  // sign: -14 is below min -> clamped to -12
    EXPECT_EQ(-12, e.value());
}

TEST(FieldEditorTest, ClickReturnsTrueOnLastGroup)
{
    FieldEditor e;
    e.begin(FieldKind::Time, 0);
    EXPECT_FALSE(e.click());
    EXPECT_EQ(1, e.group());
    EXPECT_FALSE(e.click());
    EXPECT_EQ(2, e.group());
    EXPECT_TRUE(e.click());
    e.begin(FieldKind::Dec, 0);
    EXPECT_FALSE(e.click());
    EXPECT_FALSE(e.click());
    EXPECT_FALSE(e.click());
    EXPECT_TRUE(e.click());
}

TEST(FieldEditorTest, FormatShapes)
{
    FieldEditor e;
    e.begin(FieldKind::Time, 12 * 3600 + 34 * 60 + 56);
    EXPECT_EQ("12:34:56", fmt(e));
    e.begin(FieldKind::RA, 5 * 3600);
    EXPECT_EQ("05:00:00", fmt(e));
    e.begin(FieldKind::Dec, 12 * 3600 + 34 * 60 + 56);
    EXPECT_EQ("+12\xC2\xB0"
              "34'56\"", fmt(e));
    e.begin(FieldKind::Latitude, 41 * 3600);
    EXPECT_EQ("+41*00", fmt(e));
    e.begin(FieldKind::Longitude, -74 * 3600);
    EXPECT_EQ("-074*00", fmt(e));
    e.begin(FieldKind::Date, 20261110);
    EXPECT_EQ("2026-11-10", fmt(e));
    e.begin(FieldKind::Int, 3, -12, 14);
    EXPECT_EQ("+3", fmt(e));
    e.begin(FieldKind::Int, -12, -12, 14);
    EXPECT_EQ("-12", fmt(e));
}

TEST(FieldEditorTest, BackMovesToPreviousGroup)
{
    FieldEditor e;
    e.begin(FieldKind::Time, 0);
    e.click();
    e.click();
    EXPECT_EQ(2, e.group());
    EXPECT_FALSE(e.back());
    EXPECT_EQ(1, e.group());
}

TEST(FieldEditorTest, BackOnFirstGroupCancels)
{
    FieldEditor e;
    e.begin(FieldKind::Time, 0);
    EXPECT_TRUE(e.back());
    EXPECT_EQ(0, e.group());
}

TEST(FieldEditorTest, TimeHMFormatAndSecondsZero)
{
    FieldEditor e;
    e.begin(FieldKind::TimeHM, 23 * 3600 + 41 * 60 + 59);
    EXPECT_EQ("23:41", fmt(e));
    EXPECT_EQ(0, e.value() % 60);
    e.turn(1);
    EXPECT_EQ("00:41", fmt(e));
    e.click();
    e.turn(-42);
    EXPECT_EQ("00:59", fmt(e));
    EXPECT_EQ(59 * 60, e.value());
    EXPECT_TRUE(e.click());
}

TEST(FieldEditorTest, LatitudeLettersFormat)
{
    FieldEditor e;
    e.begin(FieldKind::Latitude, 41 * 3600 + 60);
    e.setLetters('N', 'S');
    EXPECT_EQ("41\xC2\xB0" "01'N", fmt(e));
    e.begin(FieldKind::Latitude, -(41 * 3600 + 60));
    e.setLetters('N', 'S');
    EXPECT_EQ("41\xC2\xB0" "01'S", fmt(e));
}

TEST(FieldEditorTest, LongitudeLettersFormat)
{
    FieldEditor e;
    e.begin(FieldKind::Longitude, 28 * 3600 + 58 * 60);
    e.setLetters('E', 'W');
    EXPECT_EQ("028\xC2\xB0" "58'E", fmt(e));
}

TEST(FieldEditorTest, LetterGroupFlipsSign)
{
    FieldEditor e;
    e.begin(FieldKind::Longitude, 28 * 3600 + 58 * 60);
    e.setLetters('E', 'W');
    e.click();
    e.click();
    EXPECT_EQ(2, e.group());
    e.turn(1);
    EXPECT_EQ("028\xC2\xB0" "58'W", fmt(e));
    EXPECT_EQ(-(28 * 3600 + 58 * 60), e.value());
    e.turn(-1);
    EXPECT_EQ(28 * 3600 + 58 * 60, e.value());
    EXPECT_TRUE(e.click());
}

TEST(FieldEditorTest, LettersKeepRangeClamp)
{
    FieldEditor e;
    e.begin(FieldKind::Latitude, 89 * 3600 + 59 * 60);
    e.setLetters('N', 'S');
    e.turn(1);  // degrees 89 -> 90 clamps minutes to 00
    EXPECT_EQ(90 * 3600, e.value());
    EXPECT_EQ("90\xC2\xB0" "00'N", fmt(e));
}
