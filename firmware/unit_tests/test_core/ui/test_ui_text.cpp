#include <gtest/gtest.h>
#include <cstddef>
#include <cstring>
#include "core/ui/UiText.hpp"

TEST(UiTextTest, AllTextIdsHaveBothLanguages)
{
    for (size_t i = 0; i < static_cast<size_t>(core::TextId::Count); ++i)
    {
        for (core::Lang lang : {core::Lang::EN, core::Lang::TR})
        {
            const char *s = core::uiText(static_cast<core::TextId>(i), lang);
            ASSERT_NE(nullptr, s) << i;
            ASSERT_GT(strlen(s), 0u) << i;
            ASSERT_LE(strlen(s), 20u) << i;
            for (const char *p = s; *p; ++p)
                ASSERT_TRUE(*p >= 0x20 && *p <= 0x7E) << i;
        }
    }
}

TEST(UiTextTest, TurkishExamples)
{
    EXPECT_STREQ("Burayi takip et", core::uiText(core::TextId::TrackHere, core::Lang::TR));
    EXPECT_STREQ("Ayarlar", core::uiText(core::TextId::Settings, core::Lang::TR));
    EXPECT_STREQ("DUR", core::uiText(core::TextId::Stop, core::Lang::TR));
    EXPECT_STREQ("Tarih/saat ayarla", core::uiText(core::TextId::SetDateTime, core::Lang::TR));
    EXPECT_STREQ("STOP", core::uiText(core::TextId::Stop, core::Lang::EN));
    EXPECT_STREQ("Settings", core::uiText(core::TextId::Settings, core::Lang::EN));
    EXPECT_STREQ("Hold: back", core::uiText(core::TextId::HintHoldBack, core::Lang::EN));
    EXPECT_STREQ("Click/hold: back", core::uiText(core::TextId::HintClickOrHoldBack, core::Lang::EN));
    EXPECT_STREQ("Gidiyor", core::uiText(core::TextId::StateSlewing, core::Lang::TR));
    EXPECT_STREQ("Parkta", core::uiText(core::TextId::StateParked, core::Lang::TR));
    EXPECT_STREQ("Tikla: sec", core::uiText(core::TextId::HintClickSelect, core::Lang::TR));
    EXPECT_STREQ("", core::uiText(core::TextId::Count, core::Lang::EN));
}
