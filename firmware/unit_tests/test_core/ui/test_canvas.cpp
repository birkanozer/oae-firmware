#include <gtest/gtest.h>
#include "FakeCanvas.hpp"

TEST(CanvasTest, FontMetrics)
{
    EXPECT_EQ(6, core::fontWidth(core::Font::F6x10));
    EXPECT_EQ(20, core::fontHeight(core::Font::F10x20));
}

TEST(CanvasTest, FakeCanvasRejectsOutOfBounds)
{
    FakeCanvas c;
    c.text(core::Font::F6x10, 0, 0, "123456789012345678901", false);  // 21 chars = 126 px, fits
    c.box(0, 0, 127, 63);
    EXPECT_EQ(0, c.violations);
    c.text(core::Font::F6x10, 10, 20, "123456789012345678901", false);  // ends at x = 136
    EXPECT_EQ(1, c.violations);
    c.text(core::Font::F6x10, 0, 55, "A", false);  // cell bottom at 65
    c.box(0, 0, 128, 10);
    EXPECT_EQ(3, c.violations);
}
