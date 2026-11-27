#include <gtest/gtest.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include "UiHarness.hpp"
#include "core/ui/SkyMath.hpp"

// L3/L4 + point-adjust: menu tree, Point, Setup, Settings, long press = back, Turkish, width lint.

namespace {

const char kDeg[] = "\xC2\xB0";

// From the status screen: open the menu, move to `row`, click it.
void menu(Ui &u, int row)
{
    u.click();
    if (row)
        u.turn(static_cast<int8_t>(row));
    u.click();
}

void clicks(Ui &u, int n)
{
    for (int i = 0; i < n; ++i)
        u.click();
}

// Status -> Settings -> Setup list.
void setup(Ui &u)
{
    menu(u, 3);
    u.click();
}

// Label + value right-aligned at column 19 (value may hold the 2-byte degree sign).
std::string row(const std::string &label, const std::string &value)
{
    int vc = 0;
    for (char ch : value)
        vc += (ch & 0xC0) != 0x80;
    return label + std::string(19 - label.size() - vc, ' ') + value;
}

std::string deg(int d)
{
    return std::to_string(d) + kDeg;
}

bool starts(const Ui &u, const std::string &p)
{
    for (const auto &x : u.c.texts)
        if (x.s.compare(0, p.size(), p) == 0)
            return true;
    return false;
}

UiAction get(Ui &u, Kind k, bool &found)
{
    UiAction a{};
    found = Ui::find(u.acts(), k, a);
    return a;
}

// Status -> Settings -> toggle language -> status.
void toggleLang(Ui &u)
{
    menu(u, 3);
    u.turn(1);
    u.click();
    u.hold();
    u.hold();
}

// Point through both editors to the confirm screen (GO selected), with the prefilled values.
void gotoConfirm(Ui &u)
{
    menu(u, 1);
    clicks(u, 3);  // RA
    clicks(u, 4);  // Dec
}

}  // namespace

// ---------------- Menu tree ----------------

TEST(UiFlowsTest, MainMenuIsMotionPointAdjustSettings)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.tick();
    u.click();
    EXPECT_TRUE(u.drew("MENU"));
    const char *rows[] = {"Track here", "Point", "Adjust", "Settings"};
    int prev = -1;
    for (const char *r : rows)
    {
        int y = -1;
        for (const auto &x : u.c.texts)
            if (x.s == r)
                y = x.y;
        EXPECT_GE(y, 0) << r;
        EXPECT_GT(y, prev) << r;  // drawn in this order
        prev = y;
    }
    EXPECT_FALSE(u.drew("Back"));
    EXPECT_FALSE(u.drew("Jog"));
    u.turn(9);  // no wrap, no Back row: Settings is the last
    EXPECT_TRUE(u.inv("Settings"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, SetupIsUnderSettings)
{
    Ui u;
    u.tick();
    menu(u, 3);
    EXPECT_TRUE(u.drew("SETTINGS"));
    EXPECT_TRUE(u.inv("Setup"));
    u.click();
    EXPECT_TRUE(u.drew("SETUP"));
    EXPECT_TRUE(starts(u, "Date"));
    EXPECT_TRUE(starts(u, "Time"));
    EXPECT_TRUE(starts(u, "UTC offset"));
    EXPECT_TRUE(starts(u, "Latitude"));
    EXPECT_TRUE(starts(u, "Longitude"));
    u.hold();  // back to Settings, cursor on Setup
    EXPECT_TRUE(u.drew("SETTINGS"));
    EXPECT_TRUE(u.inv("Setup"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, LongPressBacksOutOfEveryScreen)
{
    // Setup -> Settings -> menu -> status.
    Ui s;
    s.tick();
    setup(s);
    s.hold();
    EXPECT_TRUE(s.drew("SETTINGS"));
    s.hold();
    EXPECT_TRUE(s.drew("MENU"));
    s.hold();
    EXPECT_TRUE(s.drew("click: menu"));
    s.actions();
    s.hold();  // the top: nothing changes
    EXPECT_TRUE(s.drew("click: menu"));
    EXPECT_TRUE(s.actions().empty());

    // Every Setup editor on its first group cancels to the Setup list; Min alt to Settings.
    for (int r = 0; r < 5; ++r)
    {
        Ui v;
        v.tick();
        setup(v);
        v.turn(static_cast<int8_t>(r));
        v.click();
        EXPECT_TRUE(v.c.has("SETUP DATE") || v.c.has("SETUP TIME") || v.c.has("SETUP UTC") || v.c.has("SETUP LAT") || v.c.has("SETUP LON")) << r;
        v.hold();
        EXPECT_TRUE(v.drew("SETUP")) << r;
        EXPECT_EQ(0, v.c.violations) << r;
    }
    Ui w;
    w.tick();
    menu(w, 3);
    w.turn(2);
    w.click();
    EXPECT_TRUE(w.drew("MIN ALT"));
    w.hold();
    EXPECT_TRUE(w.drew("SETTINGS"));

    // PointConfirm and Refused go back to the menu.
    Ui p;
    p.tick();
    gotoConfirm(p);
    EXPECT_TRUE(p.drew("CONFIRM"));
    p.hold();
    EXPECT_TRUE(p.drew("MENU"));
    Ui q;
    q.m.targetDecArcSec = -60 * 3600;
    q.tick();
    gotoConfirm(q);
    q.click(700);
    EXPECT_TRUE(q.drew("REFUSED"));
    q.hold();
    EXPECT_TRUE(q.drew("MENU"));
    EXPECT_EQ(0, s.c.violations + w.c.violations + p.c.violations + q.c.violations);
}

TEST(UiFlowsTest, LongPressInEditorGoesToPreviousField)
{
    Ui u;
    u.m.targetRaSeconds = 10 * 3600 + 20 * 60 + 30;
    u.tick();
    menu(u, 1);
    u.click();
    EXPECT_TRUE(u.inv("20"));
    u.hold();
    EXPECT_TRUE(u.inv("10"));
    clicks(u, 3);  // Dec editor, sign group
    EXPECT_TRUE(u.drew("POINT DEC"));
    u.hold();  // first group: back to the RA editor's last group
    EXPECT_TRUE(u.drew("POINT RA"));
    EXPECT_TRUE(u.inv("30"));
    u.hold();
    u.hold();
    u.hold();  // first group of the first editor: cancel
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
}

TEST(UiFlowsTest, LongPressWhileMovingIsStop)
{
    for (MountState s : {MountState::Slewing, MountState::Jogging, MountState::Homing})
    {
        Ui u;
        u.tick();
        menu(u, 3);
        EXPECT_TRUE(u.drew("SETTINGS"));
        u.actions();
        u.state(s);
        u.hold();
        const auto a = u.actions();
        EXPECT_TRUE(Ui::has(a, Kind::Stop));
        EXPECT_TRUE(Ui::has(a, Kind::BeepLong));
        EXPECT_FALSE(u.drew("SETTINGS"));  // STOP lands on the status screen, not "back"
    }
}

// ---------------- Point ----------------

TEST(UiFlowsTest, PointEditorsPrefilledWithLastTarget)
{
    Ui u;
    u.m.targetRaSeconds = 10 * 3600 + 20 * 60 + 30;
    u.m.targetDecArcSec = 12 * 3600 + 34 * 60 + 56;
    u.tick();
    menu(u, 1);
    EXPECT_TRUE(u.drew("POINT RA"));
    EXPECT_TRUE(u.inv("10"));
    EXPECT_TRUE(u.drew(":20:30"));
    clicks(u, 3);
    EXPECT_TRUE(u.drew("POINT DEC"));
    EXPECT_TRUE(u.inv("+"));
    EXPECT_TRUE(u.drew("12\xC2\xB0"
                       "34'56\""));

    // The adapter reports the target only while slewing: the UI keeps it latched.
    Ui v;
    v.m.targetRaSeconds = 10 * 3600;
    v.tick();
    v.m.hasTarget = false;
    v.m.targetRaSeconds = 0;
    v.tick(10);
    menu(v, 1);
    EXPECT_TRUE(v.inv("10"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, PointConfirmShowsAltAzSunAndSelector)
{
    Ui u;
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("POINT"));
    EXPECT_TRUE(u.drew("CONFIRM"));
    EXPECT_TRUE(u.drew("RA 05:35:17"));
    EXPECT_TRUE(u.drew("DE -05\xC2\xB0"
                       "23'28\""));
    const core::AltAz h = core::equatorialToHorizontal((u.m.lstSeconds - u.m.raSeconds) / 3600.0f, u.m.decArcSec / 3600.0f,
                                                       u.m.latArcSec / 3600.0f);
    float sra, sdec;
    core::sunRaDec(2026, 11, 10, (u.m.localSeconds - 3 * 3600) / 3600.0f, sra, sdec);
    const int sun = (int)lroundf(core::angularSeparationDeg(u.m.raSeconds / 3600.0f, u.m.decArcSec / 3600.0f, sra, sdec));
    char l[48];
    const int alt = (int)lroundf(h.altDeg), az = (int)lroundf(h.azDeg) % 360;
    snprintf(l, sizeof l, "Alt%c%02d%s Az%03d%s Sun%d", alt < 0 ? '-' : '+', alt < 0 ? -alt : alt, kDeg, az, kDeg, sun);
    EXPECT_TRUE(u.drew(l)) << l;
    EXPECT_TRUE(u.inv("GO"));  // GO is selected first
    EXPECT_TRUE(u.drew("SYNC"));
    EXPECT_FALSE(u.inv("SYNC"));
    EXPECT_TRUE(u.drew("hold: cancel"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, PointConfirmNotArmedBefore600ms)
{
    Ui u;
    u.tick();
    gotoConfirm(u);
    u.actions();
    EXPECT_FALSE(u.drew("CLICK = GO"));
    u.click(300);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_TRUE(u.drew("CONFIRM"));
    u.click(299);  // 599 ms after the screen appeared
    EXPECT_TRUE(u.actions().empty());
    EXPECT_FALSE(u.drew("CLICK = GO"));
    u.tick(1);
    EXPECT_TRUE(u.inv("CLICK = GO"));
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Goto));

    Ui v;  // SYNC is armed the same way
    v.tick();
    gotoConfirm(v);
    v.turn(1);
    v.actions();
    v.click(100);
    EXPECT_FALSE(Ui::has(v.actions(), Kind::Sync));
    EXPECT_FALSE(v.drew("CLICK = SYNC"));
    v.click(600);
    EXPECT_TRUE(Ui::has(v.actions(), Kind::Sync));
}

TEST(UiFlowsTest, PointConfirmTurnSelectsGoOrSync)
{
    Ui u;
    u.m.targetRaSeconds = 6 * 3600 + 1;
    u.m.targetDecArcSec = 10 * 3600 + 2;
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.inv("GO"));
    u.turn(1);
    EXPECT_TRUE(u.inv("SYNC"));
    EXPECT_FALSE(u.inv("GO"));
    u.turn(1);  // clamps
    EXPECT_TRUE(u.inv("SYNC"));
    u.turn(-1);
    EXPECT_TRUE(u.inv("GO"));
    u.turn(1);
    u.tick(700);
    EXPECT_TRUE(u.inv("CLICK = SYNC"));
    u.actions();
    u.click();
    bool f;
    UiAction a = get(u, Kind::Sync, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(6 * 3600 + 1, a.a);
    EXPECT_EQ(10 * 3600 + 2, a.b);
    EXPECT_TRUE(u.drew("SYNCED"));
    EXPECT_TRUE(u.drew("click: menu"));  // status screen

    Ui v;  // GO with the same values
    v.m.targetRaSeconds = 6 * 3600 + 1;
    v.m.targetDecArcSec = 10 * 3600 + 2;
    v.tick();
    gotoConfirm(v);
    v.actions();
    v.click(700);
    const auto acts = v.acts();
    ASSERT_TRUE(Ui::find(acts, Kind::Goto, a));
    EXPECT_EQ(6 * 3600 + 1, a.a);
    EXPECT_EQ(10 * 3600 + 2, a.b);
    UiAction none{};
    EXPECT_FALSE(Ui::find(acts, Kind::Sync, none));
    EXPECT_TRUE(v.drew("TRACKING"));
}

TEST(UiFlowsTest, PointSyncShowsOffset)
{
    Ui u;
    u.tick();
    menu(u, 1);
    EXPECT_TRUE(u.drew("POINT RA"));
    EXPECT_TRUE(u.inv("05"));  // prefilled with the current position
    u.click();
    u.turn(12);  // minutes 35 -> 47
    u.click();
    u.click();
    EXPECT_TRUE(u.drew("POINT DEC"));
    u.click();
    u.turn(1);  // degrees 5 -> 6 (south)
    clicks(u, 3);
    u.turn(1);  // SYNC
    EXPECT_TRUE(u.drew("POINT"));
    EXPECT_TRUE(u.drew("CONFIRM"));
    EXPECT_TRUE(u.drew("RA 05:47:17"));
    EXPECT_TRUE(u.drew((std::string("Move dRA+0:12 dDE-1") + kDeg).c_str()));
    EXPECT_FALSE(u.drew("BIG JUMP - CHECK"));
    u.tick(600);  // armed
    EXPECT_TRUE(u.inv("CLICK = SYNC"));
    u.actions();
    u.click();
    bool f;
    const UiAction a = get(u, Kind::Sync, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(5 * 3600 + 47 * 60 + 17, a.a);
    EXPECT_EQ(-(6 * 3600 + 23 * 60 + 28), a.b);
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, PointSyncBigJumpFrameAbove5deg)
{
    Ui u;
    u.tick();
    menu(u, 1);
    clicks(u, 4);
    u.turn(6);  // degrees 5 -> 11: 6 degrees
    clicks(u, 3);
    EXPECT_FALSE(u.drew("BIG JUMP - CHECK"));  // GO selected: no frame
    u.turn(1);  // SYNC
    EXPECT_TRUE(u.drew("BIG JUMP - CHECK"));
    u.tick(600);  // armed
    EXPECT_TRUE(u.inv("CLICK = SYNC"));
    EXPECT_TRUE(u.inv("SYNC"));
    EXPECT_EQ(0, u.c.violations);

    Ui v;
    v.tick();
    menu(v, 1);
    clicks(v, 4);
    v.turn(4);  // 4 degrees: no frame
    clicks(v, 3);
    v.turn(1);
    EXPECT_TRUE(v.drew("CONFIRM"));
    EXPECT_FALSE(v.drew("BIG JUMP - CHECK"));
}

TEST(UiFlowsTest, PointConfirmedCoordinatesBecomeTheTarget)
{
    Ui u;
    u.m.hasTarget = false;
    u.tick();
    menu(u, 1);
    u.click();
    u.turn(12);  // minutes 35 -> 47
    clicks(u, 6);
    u.turn(1);  // SYNC
    u.actions();
    u.click(700);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Sync));
    u.tick(2600);
    menu(u, 1);  // the editors open on the confirmed values
    EXPECT_TRUE(u.drew("POINT RA"));
    u.click();
    EXPECT_TRUE(u.inv("47"));
}

TEST(UiFlowsTest, PointRefusedBelowMinAltKeepsValues)
{
    Ui u;
    u.m.targetDecArcSec = -60 * 3600;  // far south from 41 N
    u.tick();
    gotoConfirm(u);
    u.actions();
    u.click(700);
    EXPECT_TRUE(u.drew("GOTO"));
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("BELOW HORIZON"));
    EXPECT_TRUE(starts(u, "Alt -"));
    EXPECT_TRUE(u.drew("click: edit"));
    u.click(1000);
    EXPECT_TRUE(u.drew("POINT RA"));
    EXPECT_TRUE(u.inv("05"));
    clicks(u, 3);
    EXPECT_TRUE(u.drew("60\xC2\xB0"
                       "00'00\""));  // value kept
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, PointRefusedNearSun)
{
    Ui u;
    float sra, sdec;
    core::sunRaDec(2026, 11, 10, (u.m.localSeconds - 3 * 3600) / 3600.0f, sra, sdec);
    u.m.targetRaSeconds = (int32_t)(sra * 3600);
    u.m.targetDecArcSec = (int32_t)(sdec * 3600);
    u.tick();
    gotoConfirm(u);
    u.click(700);
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("TOO CLOSE TO SUN"));
    EXPECT_TRUE(starts(u, "Sun 0"));
    EXPECT_TRUE(u.drew("click: edit"));
    u.click(1000);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, PointRefusedInvalid)
{
    Ui u;
    u.m.year = 2150;  // impossible for the sun model: fail closed
    u.tick();
    gotoConfirm(u);
    u.click(700);
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("INVALID INPUT"));
    u.click(1000);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
}

TEST(UiFlowsTest, PointLockedWithoutDateTime)
{
    for (int missing = 0; missing < 3; ++missing)  // date, time, both
    {
        Ui u;
        u.m.state = MountState::Idle;
        u.m.dateSet = missing == 1;
        u.m.timeSet = missing == 0;
        u.tick();
        u.click();  // the alert goes to the date editor
        u.hold();   // cancel: menu
        EXPECT_TRUE(u.drew("MENU")) << missing;
        u.turn(1);
        u.actions();
        u.click(1000);
        const auto a = u.actions();
        EXPECT_EQ(1u, a.size()) << missing;
        EXPECT_TRUE(Ui::has(a, Kind::BeepError)) << missing;
        EXPECT_TRUE(u.drew("MENU")) << missing;
        EXPECT_TRUE(u.inv("Set date+time first")) << missing;
        EXPECT_EQ(0, u.c.violations) << missing;
    }
}

TEST(UiFlowsTest, PointLockedOnFault)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.faultCode = 1;
    u.tick();
    u.click();  // fault page
    u.click();  // menu
    u.turn(1);
    u.actions();
    u.click(1000);
    const auto a = u.actions();
    EXPECT_EQ(1u, a.size());
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    EXPECT_TRUE(u.inv("RA DRIVER FAULT"));
    EXPECT_TRUE(u.drew("MENU"));
}

TEST(UiFlowsTest, SyncedHeader2s)
{
    Ui u;
    u.tick();
    gotoConfirm(u);
    u.turn(1);
    u.click(700);
    EXPECT_TRUE(u.drew("SYNCED"));
    u.tick(1990);
    EXPECT_TRUE(u.drew("SYNCED"));
    u.tick(20);
    EXPECT_FALSE(u.drew("SYNCED"));
    EXPECT_TRUE(u.drew("TRACKING"));
}

// ---------------- L3: Setup ----------------

TEST(UiFlowsTest, FastPathDateThenTime)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.dateSet = false;
    u.m.timeSet = false;
    u.ui.setLastSavedDate(20261110);
    u.tick();
    u.click();  // alert
    EXPECT_TRUE(u.drew("SETUP DATE"));
    u.click();
    u.click();
    u.turn(2);  // day 12
    u.click();
    EXPECT_TRUE(u.drew("SETUP TIME"));
    EXPECT_TRUE(u.inv("23"));
    EXPECT_TRUE(u.drew(":41"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::SetDate));  // applied together at the end
    u.click();
    EXPECT_TRUE(u.drew("click at :00 sec"));
    u.click();
    const auto a = u.acts();
    UiAction d{}, t{};
    ASSERT_TRUE(Ui::find(a, Kind::SetDate, d));
    ASSERT_TRUE(Ui::find(a, Kind::SetTime, t));
    EXPECT_EQ(20261112, d.a);
    EXPECT_EQ(23 * 3600 + 41 * 60, t.a);
    EXPECT_TRUE(u.drew("click: menu") || u.drew("SET DATE & TIME"));  // status
    EXPECT_FALSE(u.drew("SETUP TIME"));

    Ui v;  // hold on the time editor's first group returns to the date
    v.m.state = MountState::Idle;
    v.m.timeSet = false;
    v.tick();
    v.click();
    clicks(v, 3);
    v.hold();
    EXPECT_TRUE(v.drew("SETUP DATE"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, TimeSecondsZeroAtClick)
{
    Ui u;
    u.m.localSeconds = 10 * 3600 + 5 * 60 + 59;
    u.tick();
    setup(u);
    EXPECT_TRUE(u.drew("SETUP"));
    u.turn(1);
    u.click();  // Time
    EXPECT_TRUE(u.drew("SETUP TIME"));
    u.click();
    u.turn(1);
    u.click();
    bool f;
    const UiAction a = get(u, Kind::SetTime, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(10 * 3600 + 6 * 60, a.a);  // hh:mm, seconds 00
    EXPECT_TRUE(u.drew("SETUP"));  // back on the list
}

TEST(UiFlowsTest, DatePrefilledFromLastSaved)
{
    Ui u;  // date not set this boot: the alert opens the date editor with the last saved date
    u.m.state = MountState::Idle;
    u.m.dateSet = false;
    u.ui.setLastSavedDate(20250102);
    u.tick();
    u.click();
    EXPECT_TRUE(u.drew("SETUP DATE"));
    EXPECT_TRUE(u.inv("2025"));
    EXPECT_TRUE(u.drew("-01-02"));
    clicks(u, 5);  // date, then time
    bool f;
    const UiAction a = get(u, Kind::SetDate, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(20250102, a.a);

    Ui v;  // date set this boot: the Setup list prefills from the mount
    v.ui.setLastSavedDate(20250102);
    v.tick();
    setup(v);
    v.click();
    EXPECT_TRUE(v.inv("2026"));
    EXPECT_TRUE(v.drew("-11-10"));
}

TEST(UiFlowsTest, ClearsSyncConfirmBeforeTimeChange)
{
    Ui u;
    u.m.synced = true;
    u.tick();
    setup(u);
    u.turn(1);
    u.click();  // Time
    clicks(u, 2);
    EXPECT_TRUE(u.drew("Clears Sync. Sure?"));
    EXPECT_TRUE(u.inv("click = yes"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::SetTime));
    u.hold();  // no
    EXPECT_TRUE(u.drew("SETUP"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::SetTime));
    u.click();  // Time again
    clicks(u, 2);
    u.click();  // yes
    EXPECT_TRUE(Ui::has(u.actions(), Kind::SetTime));

    // Date, UTC offset and longitude ask too; latitude does not.
    for (int r : {0, 2, 4})
    {
        Ui v;
        v.m.synced = true;
        v.tick();
        setup(v);
        v.turn(static_cast<int8_t>(r));
        v.click();
        clicks(v, r == 2 ? 2 : 3);  // the UTC editor has two groups
        EXPECT_TRUE(v.drew("Clears Sync. Sure?")) << r;
    }
    Ui w;
    w.m.synced = true;
    w.tick();
    setup(w);
    w.turn(3);
    w.click();
    clicks(w, 3);
    EXPECT_FALSE(w.drew("Clears Sync. Sure?"));
    EXPECT_TRUE(Ui::has(w.actions(), Kind::SetLatitude));

    // Fast path with a Sync: both values wait for the confirm.
    Ui f;
    f.m.state = MountState::Idle;
    f.m.synced = true;
    f.m.timeSet = false;
    f.tick();
    f.click();
    clicks(f, 5);
    EXPECT_TRUE(f.drew("Clears Sync. Sure?"));
    auto a = f.actions();
    EXPECT_FALSE(Ui::has(a, Kind::SetDate));
    EXPECT_FALSE(Ui::has(a, Kind::SetTime));
    f.click();
    a = f.actions();
    EXPECT_TRUE(Ui::has(a, Kind::SetDate));
    EXPECT_TRUE(Ui::has(a, Kind::SetTime));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, AbandonedSureNeverRidesAlong)
{
    // "Sure?" left by STOP: the pending SetTime must not go out with the next edit.
    Ui u;
    u.m.synced = true;
    u.tick();
    setup(u);
    u.turn(1);
    u.click();  // Time
    clicks(u, 2);
    EXPECT_TRUE(u.drew("Clears Sync. Sure?"));
    u.state(MountState::Slewing);
    u.click();  // STOP
    u.state(MountState::Tracking, 1000);
    u.m.synced = false;
    u.tick(600);  // past the row-0 click ignore
    setup(u);
    u.turn(3);
    u.click();  // Latitude
    u.actions();
    clicks(u, 3);
    const auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::SetLatitude));
    EXPECT_FALSE(Ui::has(a, Kind::SetTime));
}

TEST(UiFlowsTest, LatLonUseHemisphereLetters)
{
    Ui u;
    u.tick();
    setup(u);
    EXPECT_TRUE(u.drew(row("Latitude", std::string("41") + kDeg + "01'N").c_str()));
    EXPECT_TRUE(u.drew(row("Longitude", std::string("028") + kDeg + "58'E").c_str()));
    u.turn(3);
    u.click();
    EXPECT_TRUE(u.drew("SETUP LAT"));
    clicks(u, 2);
    EXPECT_TRUE(u.inv("N"));
    EXPECT_TRUE(u.drew("north / south"));
    u.turn(1);
    EXPECT_TRUE(u.inv("S"));
    u.click();
    bool f;
    UiAction a = get(u, Kind::SetLatitude, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(-(41 * 3600 + 60), a.a);

    u.turn(1);
    u.click();  // Longitude
    EXPECT_TRUE(u.drew("SETUP LON"));
    clicks(u, 2);
    EXPECT_TRUE(u.inv("E"));
    u.turn(1);
    EXPECT_TRUE(u.inv("W"));
    u.click();
    a = get(u, Kind::SetLongitude, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(-(28 * 3600 + 58 * 60), a.a);

    Ui t;  // Turkish letters: K/G, D/B
    t.tick();
    toggleLang(t);
    setup(t);
    EXPECT_TRUE(t.drew(row("Enlem", std::string("41") + kDeg + "01'K").c_str()));
    EXPECT_TRUE(t.drew(row("Boylam", std::string("028") + kDeg + "58'D").c_str()));
    t.turn(3);
    t.click();
    clicks(t, 2);
    t.turn(1);
    EXPECT_TRUE(t.inv("G"));
    t.click();
    t.turn(1);
    t.click();
    clicks(t, 2);
    t.turn(1);
    EXPECT_TRUE(t.inv("B"));
    EXPECT_EQ(0, u.c.violations + t.c.violations);
}

TEST(UiFlowsTest, UtcOffsetRange)
{
    Ui u;
    u.tick();
    setup(u);
    u.turn(2);
    u.click();
    EXPECT_TRUE(u.drew("SETUP UTC"));
    EXPECT_TRUE(u.inv("+"));
    EXPECT_TRUE(u.drew("3"));
    u.turn(1);  // sign -
    u.click();
    u.turn(9);  // -12
    EXPECT_TRUE(u.inv("12"));
    u.turn(1);  // -13 is out of range: clamped
    EXPECT_TRUE(u.inv("12"));
    u.click();
    bool f;
    UiAction a = get(u, Kind::SetUtcOffset, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(-12, a.a);

    u.click();  // the cursor stayed on UTC offset
    u.click();
    u.turn(11);  // +14
    EXPECT_TRUE(u.inv("14"));
    u.click();
    a = get(u, Kind::SetUtcOffset, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(14, a.a);
}

// ---------------- Status while parking / homing ----------------

TEST(UiFlowsTest, StatusShowsParkingAndHomingAndStopWorks)
{
    Ui u;
    u.state(MountState::Parking);
    EXPECT_TRUE(u.drew("PARKING"));
    EXPECT_TRUE(u.drew("> park position"));
    EXPECT_TRUE(u.inv("click: STOP"));
    u.actions();
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));

    Ui v;
    v.m.state = MountState::Homing;
    v.tick();
    EXPECT_TRUE(v.drew("HOMING"));
    EXPECT_TRUE(v.drew("> home position"));
    EXPECT_EQ(0, u.c.violations + v.c.violations);
}

// ---------------- L4: Settings ----------------

TEST(UiFlowsTest, SettingsFromTable)
{
    Ui u;
    u.tick();
    menu(u, 3);
    EXPECT_TRUE(u.drew("SETTINGS"));
    EXPECT_TRUE(u.inv("Setup"));
    EXPECT_TRUE(u.drew(row("Dil/Lang", "English").c_str()));
    EXPECT_TRUE(u.drew(row("Min alt", deg(0)).c_str()));
    EXPECT_TRUE(u.drew(row("Beep", "On").c_str()));
    u.turn(6);  // Back, with the Supply and Network rows above it
    EXPECT_TRUE(u.drew(row("Supply", "24 V").c_str()));
    EXPECT_TRUE(u.drew("Network"));
    EXPECT_TRUE(u.drew("Back"));
    u.click();  // Back
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, LanguageSwitchRedrawsTurkish)
{
    Ui u;
    u.tick();
    menu(u, 3);
    u.turn(1);
    u.click();
    EXPECT_TRUE(u.drew("AYARLAR"));
    EXPECT_TRUE(u.inv(row("Dil/Lang", "Turkce").c_str()));
    EXPECT_TRUE(u.drew(row("Bip", "Acik").c_str()));
    EXPECT_TRUE(u.drew("Kurulum"));
    u.hold();
    EXPECT_TRUE(u.drew("Ayarlar"));
    u.turn(-3);
    EXPECT_TRUE(u.drew("Hedef"));
    EXPECT_TRUE(u.drew("Ayar"));
    u.hold();
    EXPECT_TRUE(u.drew("TAKIPTE"));
    EXPECT_TRUE(u.drew("tikla: menu"));
    toggleLang(u);
    EXPECT_TRUE(u.drew("TRACKING"));
}

TEST(UiFlowsTest, MinAltUsedByPoint)
{
    Ui u;
    u.m.targetDecArcSec = -40 * 3600;  // about +9 degrees from 41 N near the meridian
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("CONFIRM"));
    u.hold();
    u.hold();  // status
    menu(u, 3);
    u.turn(2);
    u.click();  // Min alt editor
    EXPECT_TRUE(u.drew("MIN ALT"));
    u.click();
    u.turn(30);
    u.click();
    EXPECT_TRUE(u.drew(row("Min alt", deg(30)).c_str()));
    u.hold();
    u.hold();
    gotoConfirm(u);
    u.click(700);
    EXPECT_TRUE(u.drew("BELOW HORIZON"));
    EXPECT_TRUE(u.drew((std::string("Alt +9") + kDeg + " (min 30" + kDeg + ")").c_str()));
}

TEST(UiFlowsTest, BeepOffSilencesTicksOnly)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.timeSet = false;
    u.tick();
    u.click();  // alert -> date editor
    u.hold();   // menu
    u.turn(7);
    u.click();  // Settings
    u.turn(3);
    u.click();  // Beep -> Off
    EXPECT_TRUE(u.inv(row("Beep", "Off").c_str()));
    u.actions();
    u.hold();
    u.click(1000);  // locked Track here: the error beep stays
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    u.turn(5, 2100);
    u.click();  // Settings: no click tick
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Beep));
    u.state(MountState::Slewing);
    u.click();  // STOP keeps its long beep
    a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::Stop));
    EXPECT_TRUE(Ui::has(a, Kind::BeepLong));
    u.m.timeSet = true;
    u.state(MountState::Tracking, 3000);  // not an arrival (stopped), but the alarm beeps stay:
    u.m.decArcSec = -60 * 3600;
    u.tick(10);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Beep));  // BELOW MIN ALT beeps
}

TEST(UiFlowsTest, SupplyRowEmitsAction)
{
    Ui u;
    u.tick();
    menu(u, 3);
    u.turn(4);
    u.actions();
    u.click();
    bool f;
    UiAction a = get(u, Kind::SetSupplyVoltage, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(12, a.a);  // from the default 24 V it wraps to 12 V
    // 12 -> 15 -> 20 -> 24 -> 12, each step from the value the firmware reports
    const int volts[] = {12, 15, 20, 24};
    for (int i = 0; i < 4; ++i)
    {
        u.m.supplyVoltage = static_cast<uint8_t>(volts[i]);
        u.tick(10);
        EXPECT_TRUE(u.drew(row("Supply", std::to_string(volts[i]) + " V").c_str())) << volts[i];
        u.click();
        a = get(u, Kind::SetSupplyVoltage, f);
        ASSERT_TRUE(f);
        EXPECT_EQ(volts[(i + 1) % 4], a.a);
    }
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, SupplyRowUnknownVoltageGoesTo12)
{
    Ui u;
    u.m.supplyVoltage = 18;  // not a choice: treated as 24, the next is 12
    u.tick();
    menu(u, 3);
    u.turn(4);
    u.actions();
    u.click();
    bool f;
    const UiAction a = get(u, Kind::SetSupplyVoltage, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(12, a.a);
}

TEST(UiFlowsTest, SupplyRowShowsSnapshotValue)
{
    Ui u;
    u.m.supplyVoltage = 12;
    u.tick();
    menu(u, 3);
    u.turn(4);
    EXPECT_TRUE(u.drew(row("Supply", "12 V").c_str()));
    u.click();  // asks for 24 V; the row waits for the firmware
    EXPECT_TRUE(u.inv(row("Supply", "12 V").c_str()));
    u.m.supplyVoltage = 24;
    u.tick(10);
    EXPECT_TRUE(u.inv(row("Supply", "24 V").c_str()));
}

// ---------------- persistent settings ----------------

using core::SettingId;
using core::UiSettings;

TEST(UiFlowsTest, SettingsBlockRoundTrip)
{
    UiSettings s;
    s.set(SettingId::Language, 1);
    s.set(SettingId::MinAlt, -10);
    s.set(SettingId::Beep, 0);
    s.set(SettingId::Supply, 1);
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    EXPECT_EQ(UiSettings::kStoreMarker, b[0]);
    UiSettings r;
    ASSERT_TRUE(r.load(b));
    for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i)
        EXPECT_EQ(s.get(static_cast<SettingId>(i)), r.get(static_cast<SettingId>(i))) << int(i);
}

TEST(UiFlowsTest, BlankOrClearedStoreKeepsDefaults)
{
    for (uint8_t fill : {uint8_t(0x00), uint8_t(0xFF)})  // cleared store, blank chip (or a failed read)
    {
        uint8_t b[UiSettings::kStoreBytes];
        memset(b, fill, sizeof b);
        UiSettings s, d;
        EXPECT_FALSE(s.load(b)) << int(fill);
        for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i)
            EXPECT_EQ(d.get(static_cast<SettingId>(i)), s.get(static_cast<SettingId>(i))) << int(fill) << " " << int(i);
    }
}

TEST(UiFlowsTest, StoredValueOutOfRangeKeepsItsDefault)
{
    UiSettings s;
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    b[1 + static_cast<uint8_t>(SettingId::Language)] = 1;
    b[1 + static_cast<uint8_t>(SettingId::MinAlt)] = 100;
    b[1 + static_cast<uint8_t>(SettingId::Supply)] = 9;
    UiSettings r;
    ASSERT_TRUE(r.load(b));
    EXPECT_EQ(1, r.get(SettingId::Language));
    EXPECT_EQ(0, r.get(SettingId::MinAlt));
    EXPECT_EQ(0, r.get(SettingId::Supply));  // Auto
}

TEST(UiFlowsTest, LoadedSettingsApplyBeforeTheFirstFrame)
{
    UiSettings s;
    s.set(SettingId::Language, 1);
    s.set(SettingId::Supply, 1);  // 12 V
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    Ui u;
    EXPECT_EQ(12, u.ui.loadSettings(b));  // the stored voltage, for the adapter to apply
    u.m.supplyVoltage = 12;
    u.tick();
    EXPECT_TRUE(u.drew("TAKIPTE"));
    EXPECT_EQ(1, u.ui.settings().get(SettingId::Language));
    b[1 + static_cast<uint8_t>(SettingId::Supply)] = 7;  // invalid: keep SUPPLY_VOLTAGE
    EXPECT_EQ(0, u.ui.loadSettings(b));
    s.save(b);
    b[0] = 0xFF;  // no block
    EXPECT_EQ(0, u.ui.loadSettings(b));
}

TEST(UiFlowsTest, SupplySettingFollowsTheFirmware)
{
    // The stored Supply is the firmware's voltage, not the row's default: a 12 V build must not store 24 V.
    Ui u;
    u.m.supplyVoltage = 15;
    u.tick();
    EXPECT_EQ(2, u.ui.settings().get(SettingId::Supply));
    u.m.supplyVoltage = 18;  // not a choice: unchanged
    u.tick(10);
    EXPECT_EQ(2, u.ui.settings().get(SettingId::Supply));
}

// ---------------- Supply Auto (supply sense) ----------------

namespace {

// Status -> Settings, cursor on the Supply row.
void supplyRow(Ui &u)
{
    menu(u, 3);
    u.turn(4);
}

void loadBlock(Ui &u, int lang, int supply)
{
    UiSettings s;
    s.set(SettingId::Language, lang);
    s.set(SettingId::Supply, supply);
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    u.ui.loadSettings(b);
}

}  // namespace

TEST(UiFlowsTest, SupplyDefaultsToAutoWithSense)
{
    Ui u;
    u.m.supplySense = true;
    u.tick();
    EXPECT_EQ(0, u.ui.settings().get(SettingId::Supply));  // not mirrored from the firmware's 24 V
    supplyRow(u);
    EXPECT_TRUE(u.inv(row("Supply", "Auto 24 --").c_str()));  // no reading yet: the applied value only
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, SupplyWithoutSenseMirrorsTheFirmware)
{
    Ui u;  // no sense: the Auto default turns into the firmware's voltage on the first tick
    u.m.supplyVoltage = 12;
    u.tick();
    EXPECT_EQ(1, u.ui.settings().get(SettingId::Supply));
}

TEST(UiFlowsTest, SupplyCyclesAutoAndTheVoltages)
{
    Ui u;
    u.m.supplySense = true;
    u.m.supplyVoltage = 12;
    u.tick();
    supplyRow(u);
    u.actions();
    bool f;
    const int volts[] = {12, 15, 20, 24};
    for (int i = 0; i < 4; ++i)  // Auto -> 12 (the applied value) -> 15 -> 20 -> 24
    {
        u.click();
        const UiAction a = get(u, Kind::SetSupplyVoltage, f);
        EXPECT_EQ(i > 0, f) << i;  // leaving Auto applies nothing new
        if (f)
        {
            EXPECT_EQ(volts[i], a.a);
        }
        EXPECT_EQ(i + 1, u.ui.settings().get(SettingId::Supply));
        u.m.supplyVoltage = static_cast<uint8_t>(volts[i]);
        u.tick(10);
        EXPECT_TRUE(u.inv(row("Supply", std::to_string(volts[i]) + " V").c_str())) << volts[i];
    }
    u.click();  // 24 -> Auto: nothing to apply until a reading is stable
    get(u, Kind::SetSupplyVoltage, f);
    EXPECT_FALSE(f);
    EXPECT_EQ(0, u.ui.settings().get(SettingId::Supply));
    EXPECT_TRUE(u.inv(row("Supply", "Auto 24 --").c_str()));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, LeavingAutoStartsAtTheAppliedVoltage)
{
    Ui u;
    u.m.supplySense = true;
    u.m.supplyVoltage = 15;
    u.tick();
    supplyRow(u);
    u.actions();
    bool f;
    // Auto -> 15 (nothing applied) -> 20 -> 24 -> 12 -> Auto: every value is reachable, a full lap
    const int volts[] = {15, 20, 24, 12};
    for (int i = 0; i < 4; ++i)
    {
        u.click();
        const UiAction a = get(u, Kind::SetSupplyVoltage, f);
        EXPECT_EQ(i > 0, f) << i;
        if (f)
        {
            EXPECT_EQ(volts[i], a.a) << i;
        }
        u.m.supplyVoltage = static_cast<uint8_t>(volts[i]);
        u.tick(10);
        EXPECT_TRUE(u.inv(row("Supply", std::to_string(volts[i]) + " V").c_str())) << volts[i];
    }
    u.click();
    get(u, Kind::SetSupplyVoltage, f);
    EXPECT_FALSE(f);
    EXPECT_EQ(0, u.ui.settings().get(SettingId::Supply));
}

TEST(UiFlowsTest, SupplyRowShowsTheMeasuredVoltage)
{
    for (int lang : {0, 1})
    {
        Ui u;
        u.m.supplySense = true;
        u.m.supplyVoltage = 15;
        loadBlock(u, lang, 0);
        u.m.supplyMv = 15149;
        u.tick();
        supplyRow(u);
        const char *label = lang ? "Besleme" : "Supply";
        EXPECT_TRUE(u.inv(row(label, lang ? "Oto 15 15.1" : "Auto 15 15.1").c_str())) << lang;
        u.m.supplyVoltage = 24;
        u.m.supplyMv = 20049;  // the applied choice is shown next to the measurement
        u.tick(10);
        EXPECT_TRUE(u.inv(row(label, lang ? "Oto 24 20.0" : "Auto 24 20.0").c_str())) << lang;
        u.m.supplyVoltage = 15;
        u.m.supplyMv = 0;
        u.tick(10);
        EXPECT_TRUE(u.inv(row(label, lang ? "Oto 15 --" : "Auto 15 --").c_str())) << lang;
        u.m.supplyVoltage = 0;  // applied value unknown
        u.tick(10);
        EXPECT_TRUE(u.inv(row(label, lang ? "Oto --" : "Auto --").c_str())) << lang;
        u.m.supplyVoltage = 12;
        u.click();  // manual 12 V
        u.m.supplyMv = 29960;
        u.tick(10);
        EXPECT_TRUE(u.inv(row(label, "12 V (30.0)").c_str())) << lang;
        u.m.supplyVoltage = 24;
        loadBlock(u, lang, 4);
        u.tick(10);
        EXPECT_TRUE(u.inv(row(label, "24 V (30.0)").c_str())) << lang;
        EXPECT_EQ(0, u.c.violations) << lang;
    }
}

TEST(UiFlowsTest, SupplyAutoWaitsForTheMoveItsOwnClickStarted)
{
    Ui u;
    u.m.supplySense = true;
    u.tick();
    gotoConfirm(u);
    u.actions();
    u.m.supplyAuto = 15;  // becomes stable in the very tick of the GO click
    u.click(700);
    const auto acts = u.acts();
    UiAction g;
    ASSERT_TRUE(Ui::find(acts, Kind::Goto, g));
    for (const UiAction &a : acts)
        EXPECT_NE(Kind::SetSupplyVoltage, a.kind);
    u.m.state = MountState::Slewing;  // the mount reports the slew: still not applied
    u.tick(10);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::SetSupplyVoltage));
    u.m.state = MountState::Tracking;
    u.tick(10);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::SetSupplyVoltage));
}

TEST(UiFlowsTest, SupplyAutoAppliesOnlyWhileNotMoving)
{
    Ui u;
    u.m.supplySense = true;
    u.m.supplyAuto = 15;
    u.m.state = MountState::Slewing;
    u.tick();
    bool f;
    for (MountState s : {MountState::Slewing, MountState::Jogging, MountState::Homing, MountState::Parking})
    {
        u.state(s);
        get(u, Kind::SetSupplyVoltage, f);
        EXPECT_FALSE(f) << static_cast<int>(s);
    }
    u.state(MountState::Tracking);  // tracking is fine
    UiAction a = get(u, Kind::SetSupplyVoltage, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(15, a.a);
    u.m.supplyVoltage = 15;  // applied
    u.tick(10);
    get(u, Kind::SetSupplyVoltage, f);
    EXPECT_FALSE(f);
    EXPECT_EQ(0, u.ui.settings().get(SettingId::Supply));  // still Auto: switching sources stores nothing
    u.m.supplyAuto = 0;  // no stable reading: keep 15 V
    u.state(MountState::Idle);
    get(u, Kind::SetSupplyVoltage, f);
    EXPECT_FALSE(f);
}

TEST(UiFlowsTest, SupplyManualIgnoresTheSense)
{
    Ui u;
    u.m.supplySense = true;
    loadBlock(u, 0, 3);  // 20 V
    u.m.supplyVoltage = 20;
    u.m.supplyAuto = 15;
    u.state(MountState::Idle);
    bool f;
    get(u, Kind::SetSupplyVoltage, f);
    EXPECT_FALSE(f);
    EXPECT_EQ(3, u.ui.settings().get(SettingId::Supply));
}

TEST(UiFlowsTest, LegacySettingsBlockStillLoads)
{
    // Before Auto: marker 0xA1 and Supply stored as an index into 12/15/20/24.
    const int volts[] = {12, 15, 20, 24};
    for (uint8_t old = 0; old < 4; ++old)
    {
        uint8_t b[UiSettings::kStoreBytes];
        b[0] = 0xA1;
        b[1 + static_cast<uint8_t>(SettingId::Language)] = 1;
        b[1 + static_cast<uint8_t>(SettingId::MinAlt)] = static_cast<uint8_t>(-5);
        b[1 + static_cast<uint8_t>(SettingId::Beep)] = 0;
        b[1 + static_cast<uint8_t>(SettingId::Supply)] = old;
        b[1 + static_cast<uint8_t>(SettingId::OwnWifi)] = 2;
        Ui u;
        EXPECT_EQ(volts[old], u.ui.loadSettings(b)) << int(old);
        const UiSettings &s = u.ui.settings();
        EXPECT_EQ(old + 1, s.get(SettingId::Supply));
        EXPECT_EQ(1, s.get(SettingId::Language));
        EXPECT_EQ(-5, s.get(SettingId::MinAlt));
        EXPECT_EQ(0, s.get(SettingId::Beep));
        EXPECT_EQ(2, s.get(SettingId::OwnWifi));
        uint8_t n[UiSettings::kStoreBytes];
        s.save(n);
        EXPECT_EQ(UiSettings::kStoreMarker, n[0]);  // stored again in the new layout on the next change
        EXPECT_NE(0xA1, n[0]);
    }
}

TEST(UiFlowsTest, StoredAutoAppliesNoVoltageAtBoot)
{
    Ui u;
    UiSettings s;
    s.set(SettingId::Supply, 0);
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    EXPECT_EQ(0, u.ui.loadSettings(b));  // keeps SUPPLY_VOLTAGE until a reading is stable
    u.m.supplySense = true;
    u.tick();
    EXPECT_EQ(0, u.ui.settings().get(SettingId::Supply));
}

// ---------------- Settings > Network ----------------

namespace {

// Status -> Settings -> Network page.
void network(Ui &u)
{
    menu(u, 3);
    u.turn(5);
    u.click();
}

// The bridge reported now (the adapter copies what the :XN commands stored).
void bridge(Ui &u, uint8_t mode, const char *ssid)
{
    u.m.net.seen = true;
    u.m.net.atMs = u.t;
    u.m.net.mode = mode;
    u.m.net.clients = 2;
    u.m.net.rssi = -58;
    const uint8_t ip[4] = {172, 20, 10, 2};
    memcpy(u.m.net.ip, ip, 4);
    snprintf(u.m.net.ssid, sizeof u.m.net.ssid, "%s", ssid);
}

}  // namespace

TEST(UiFlowsTest, NetworkRowSitsAboveBack)
{
    Ui u;
    u.tick();
    menu(u, 3);
    u.turn(5);
    EXPECT_TRUE(u.inv("Network"));
    u.turn(1);
    EXPECT_TRUE(u.inv("Back"));
}

TEST(UiFlowsTest, NetworkPageWithoutBridge)
{
    Ui u;
    u.tick();
    network(u);
    EXPECT_TRUE(u.drew("NETWORK"));
    EXPECT_TRUE(u.drew("Mode: No bridge"));
    EXPECT_FALSE(starts(u, "IP"));
    EXPECT_FALSE(starts(u, "Clients"));
    EXPECT_TRUE(u.inv("Own WiFi: Auto"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, NetworkPageShowsWhatTheBridgeReports)
{
    Ui u;
    bridge(u, 1, "My iPhone");
    u.tick();
    network(u);
    EXPECT_TRUE(u.drew("Mode: Hotspot"));
    EXPECT_TRUE(u.drew("My iPhone"));
    EXPECT_TRUE(u.drew("IP: 172.20.10.2"));
    EXPECT_TRUE(u.drew("Clients: 2  -58 dBm"));  // the signal only in Hotspot mode
    u.m.net.mode = 2;
    u.tick(10);
    EXPECT_TRUE(u.drew("Mode: Own WiFi"));
    EXPECT_TRUE(u.drew("Clients: 2"));
    u.m.net.mode = 3;
    u.tick(10);
    EXPECT_TRUE(u.drew("Mode: Connecting"));
    u.m.net.mode = 0;
    memset(u.m.net.ip, 0, 4);
    u.tick(10);
    EXPECT_TRUE(u.drew("Mode: Off"));
    EXPECT_TRUE(u.drew("IP: -"));
    u.m.net.mode = 9;  // never from the parser: shown as Off
    u.tick(10);
    EXPECT_TRUE(u.drew("Mode: Off"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, NetworkPageShowsNoBridgeAfterFiveSeconds)
{
    Ui u;
    u.tick();
    network(u);
    bridge(u, 1, "My iPhone");
    u.tick(4990);
    EXPECT_TRUE(u.drew("Mode: Hotspot"));
    u.tick(10);  // 5 s without a status
    EXPECT_TRUE(u.drew("Mode: No bridge"));
    EXPECT_FALSE(u.drew("My iPhone"));
    EXPECT_FALSE(starts(u, "IP"));
    u.m.net.atMs = u.t;
    u.tick(10);
    EXPECT_TRUE(u.drew("Mode: Hotspot"));
}

TEST(UiFlowsTest, NetworkOwnWifiCyclesAndLongPressGoesBack)
{
    Ui u;
    u.tick();
    network(u);
    u.actions();
    u.click();
    EXPECT_TRUE(u.inv("Own WiFi: On"));
    EXPECT_EQ(1, u.ui.settings().get(SettingId::OwnWifi));
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Beep));  // the click tick
    u.click();
    EXPECT_TRUE(u.inv("Own WiFi: Off"));
    u.click();
    EXPECT_TRUE(u.inv("Own WiFi: Auto"));
    EXPECT_EQ(0, u.ui.settings().get(SettingId::OwnWifi));
    u.hold();
    EXPECT_TRUE(u.drew("SETTINGS"));
    EXPECT_TRUE(u.inv("Network"));  // back on the row it came from
}

TEST(UiFlowsTest, NetworkPageClickWhileMovingStops)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.tick();
    network(u);
    u.state(MountState::Slewing);
    EXPECT_TRUE(u.inv("click: STOP"));
    u.actions();
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));
    EXPECT_EQ(0, u.ui.settings().get(SettingId::OwnWifi));  // not changed
    EXPECT_TRUE(u.drew("STOPPED"));
}

TEST(UiFlowsTest, OwnWifiIsStored)
{
    Ui u;
    u.tick();
    network(u);
    u.click();
    u.click();  // Off
    uint8_t b[UiSettings::kStoreBytes];
    u.ui.settings().save(b);
    UiSettings r;
    ASSERT_TRUE(r.load(b));
    EXPECT_EQ(2, r.get(SettingId::OwnWifi));
}

// ---------------- width lint ----------------

namespace {

int walk(Ui &u)
{
    // Worst-case values everywhere.
    u.m = base();
    u.m.state = MountState::Idle;
    u.m.utcOffsetHours = -12;
    u.m.latArcSec = -(89 * 3600 + 59 * 60);
    u.m.lonArcSec = -(179 * 3600 + 59 * 60);
    u.m.decArcSec = -(89 * 3600 + 59 * 60 + 59);
    u.m.raSeconds = 23 * 3600 + 59 * 60 + 59;
    u.m.targetRaSeconds = 11 * 3600 + 59 * 60 + 59;
    u.m.targetDecArcSec = 89 * 3600 + 59 * 60 + 59;
    u.m.slewRate = 2;  // CENTER / ORTALA, the longest
    u.m.supplyVoltage = 24;
    u.m.synced = true;
    u.tick(1000);
    for (MountState s : {MountState::Slewing, MountState::Jogging, MountState::Parking, MountState::Homing, MountState::Parked,
                         MountState::Tracking, MountState::Idle})
        u.state(s, 1000);
    u.m.timeSet = false;
    u.tick(10);
    u.m.timeSet = true;
    u.tick(10);

    for (int r = 0; r < 4; ++r)  // menu, every row in view
    {
        u.click(1000);
        u.turn(static_cast<int8_t>(r));
        u.hold();
    }

    // Point: editors, confirm (armed) with GO and SYNC, back. Latitude 89:59 S: a southern target is up.
    u.m.targetDecArcSec = -60 * 3600;
    u.tick(10);
    menu(u, 1);
    for (int i = 0; i < 7; ++i)
        u.click();
    u.tick(700);
    u.turn(1);  // SYNC
    u.tick(10);
    u.hold();
    u.hold();
    // Refusals.
    u.m.targetDecArcSec = 89 * 3600;
    u.m.targetRaSeconds = 0;
    u.tick(10);
    gotoConfirm(u);  // below
    u.click(700);
    u.hold();
    u.hold();
    float sra, sdec;
    core::sunRaDec(2026, 11, 10, 20.0f, sra, sdec);
    u.m.targetRaSeconds = (int32_t)(sra * 3600);
    u.m.targetDecArcSec = (int32_t)(sdec * 3600) + 14 * 3600;  // 14 degrees away: two digits
    u.m.utcOffsetHours = 3;
    u.m.localSeconds = 23 * 3600;
    u.tick(10);
    gotoConfirm(u);
    u.click(700);
    u.hold();
    u.hold();
    u.m.year = 2150;
    u.tick(10);
    gotoConfirm(u);  // invalid
    u.click(700);
    u.hold();
    u.hold();
    u.m.year = 2026;
    u.m.utcOffsetHours = -12;

    // Sync with the largest offset: RA +12 h, Dec -89:59:59 -> +89:59:59.
    menu(u, 1);
    u.turn(12);
    clicks(u, 3);
    u.turn(1);
    clicks(u, 4);
    u.turn(1);  // SYNC
    u.click(700);  // armed

    // Setup list and every editor, Clears Sync.
    u.tick(10);
    setup(u);
    for (int r = 0; r < 5; ++r)
    {
        u.click();
        for (int i = 0; i < 3; ++i)
            u.click();
        if (u.drew("Clears Sync. Sure?") || u.drew("Esitleme silinsin?"))
            u.hold();
        u.turn(1);
    }
    u.hold();  // settings
    u.hold();  // menu
    u.hold();  // status

    // Settings with min alt -10, its editor.
    menu(u, 3);
    u.turn(2);
    u.click();
    u.turn(1);
    u.click();
    u.turn(10);
    u.click();
    u.turn(1);
    u.click();  // beep off
    u.click();  // beep on
    u.hold();  // menu
    u.hold();  // status

    // Network: no bridge, every mode with the longest name, address and signal, Own WiFi through its values, the STOP bar.
    menu(u, 3);
    u.turn(5);
    u.click();
    u.m.net.seen = true;
    u.m.net.clients = 9;
    u.m.net.rssi = -127;
    memset(u.m.net.ip, 255, 4);
    snprintf(u.m.net.ssid, sizeof u.m.net.ssid, "%s", "WWWWWWWWWWWWWWWWWWWW");
    for (uint8_t md = 0; md < 4; ++md)
    {
        u.m.net.mode = md;
        u.m.net.atMs = u.t;
        u.tick(10);
    }
    clicks(u, 3);
    u.state(MountState::Slewing);
    u.click();  // STOP: status
    u.state(MountState::Idle);
    u.tick(600);

    // Adjust: every axis with every step size, the options, Set home confirm, locks, moving, a failed driver.
    u.m.hasAz = u.m.hasFocus = true;
    u.m.stepsPerDeg[0] = 1000.0f;  // 1 step = 0.001 deg: arc-seconds
    u.m.stepsPerDeg[1] = 100000.0f;
    u.m.stepsPerDeg[2] = 0.5f;  // 10000 steps = 20000 deg: no hint
    u.m.axisPos[0] = -2147483647;
    u.m.axisPos[1] = 2147483647;
    u.m.axisPos[2] = -123456;
    u.m.axisPos[3] = 7;
    u.tick(10);
    menu(u, 2);
    for (int ax = 0; ax < 4; ++ax)
    {
        for (int i = 0; i < 9; ++i)
        {
            u.hold();  // options (cursor on Step)
            u.click();  // next size
            u.hold();  // main menu
            u.turn(2);
            u.click();  // Adjust
        }
        u.click();  // next axis
    }
    u.hold();  // options
    u.turn(2);
    u.click();  // Set home here: confirm
    u.hold();
    u.state(MountState::Tracking);
    u.tick(600);  // past the row 0 click ignore
    u.click();  // locked: stop first
    u.turn(-1);
    u.click();  // Sync on target: emits, status
    u.state(MountState::Idle);
    u.tick(600);
    menu(u, 2);
    for (int i = 0; i < 4 && !u.inv("AZ"); ++i)  // the axis is kept between visits
        u.click();
    u.m.azFault = true;
    u.turn(1);  // refused: DRIVER FAULT
    u.m.azFault = false;
    u.tick(2100);
    u.hold();
    u.hold();  // status
    menu(u, 2);
    u.state(MountState::Slewing);  // STOP bar on Adjust
    u.click();
    u.state(MountState::Idle);
    u.tick(600);
    menu(u, 2);
    u.hold();
    u.state(MountState::Slewing);  // and on the options
    u.click();
    u.state(MountState::Idle);
    u.tick(600);

    // Locked rows and messages.
    u.click(2100);  // menu
    u.m.timeSet = false;
    u.tick(10);
    for (int r = 0; r < 3; ++r)
    {
        u.click(2100);
        u.turn(1);
    }
    u.hold();
    u.m.timeSet = true;
    u.state(MountState::Idle);
    u.m.rebooted = true;
    u.m.dateSet = false;
    u.tick(10);  // rebooted page
    u.hold();
    u.m.dateSet = true;
    u.m.faultCode = 2;
    u.tick(10);  // fault page
    u.click();
    menu(u, 1);  // locked: fault message
    u.hold();
    u.m.faultCode = 0;
    u.m.rebooted = false;
    u.tick(200000);  // saver
    u.click();
    return u.c.violations;
}

}  // namespace

TEST(UiFlowsTest, AllScreensFit)
{
    Ui u;
    u.tick();
    EXPECT_EQ(0, walk(u));
    // The walk really reached every screen.
    for (const char *s : {"POINT RA", "POINT DEC", "CLICK = GO", "CLICK = SYNC", "BELOW HORIZON", "TOO CLOSE TO SUN", "INVALID INPUT",
                          "BIG JUMP - CHECK", "SYNCED", "SETUP", "SETUP DATE", "SETUP TIME", "SETUP UTC", "SETUP LAT", "SETUP LON",
                          "Clears Sync. Sure?", "SETTINGS", "MIN ALT", "PARKING", "HOMING", "> park position",
                          "Set date+time first", "REBOOTED - SET TIME", "DEC DRIVER FAULT", "turn to wake", "click: STOP", "ADJUST", "FOCUS",
                          "Set home here?", "Stop the mount first", "click:axis hold:more", "DRIVER FAULT", "Sync on target", "NETWORK",
                          "Mode: No bridge", "Mode: Connecting", "Own WiFi: Off", "IP: 255.255.255.255", "Clients: 9  -127 dBm"})
        EXPECT_EQ(1u, u.c.seen.count(s)) << s;
    u.c.seen.clear();
    toggleLang(u);
    EXPECT_TRUE(u.drew("tikla: menu") || u.drew("TARIH/SAAT GIR"));
    EXPECT_EQ(0, walk(u));
    for (const char *s : {"HEDEF RA", "HEDEF DEC", "TIKLA = GIT", "TIKLA = ESITLE", "UFKUN ALTINDA", "GUNESE COK YAKIN", "GECERSIZ",
                          "BUYUK FARK-KONTROL", "ESITLENDI", "KURULUM BOYLAM", "Esitleme silinsin?", "AYARLAR", "MIN YUKS",
                          "PARK EDIYOR", "EVE GIDIYOR", "Once tarih/saat gir", "cevir: uyan", "tikla: DURDUR", "AYAR", "ODAK",
                          "Evi buraya ayarla?", "Once durdur", "tikla:eksen tut:ek", "SURUCU HATASI", "Hedefe esitle", "AG",
                          "Mod: Kopru yok", "Mod: Baglaniyor", "Kendi WiFi: Otomatik", "Istemci: 9  -127 dBm"})
        EXPECT_EQ(1u, u.c.seen.count(s)) << s;

    // The selector cells and menu rows in both languages.
    for (int lang = 0; lang < 2; ++lang)
    {
        Ui v;
        v.tick();
        if (lang)
            toggleLang(v);
        gotoConfirm(v);
        EXPECT_TRUE(v.inv(lang ? "GIT" : "GO"));
        EXPECT_TRUE(v.drew(lang ? "ESITLE" : "SYNC"));
        v.turn(1);
        EXPECT_TRUE(v.inv(lang ? "ESITLE" : "SYNC"));
        v.hold();
        EXPECT_TRUE(v.drew(lang ? "Hedef" : "Point"));
        EXPECT_TRUE(v.drew(lang ? "Ayar" : "Adjust"));
        EXPECT_EQ(0, v.c.violations);
    }

    // A header state can never run into the clock: FakeCanvas flags same-row overlap.
    FakeCanvas c;
    c.text(core::Font::F6x10, 1, 1, "KAYDIRILIYOR!", false);
    c.text(core::Font::F6x10, 78, 1, "23:41:07", false);
    EXPECT_EQ(1, c.violations);
    c.text(core::Font::F6x10, 2, 13, "12345678901234567890", false);  // 20 cols in a menu row
    EXPECT_EQ(2, c.violations);
}

// ---------------- final review fixes ----------------

TEST(UiFlowsTest, FaultWhileTrackingLocksMotionRowsButNotStop)
{
    Ui u;  // tracking, driver fault
    u.m.faultCode = 1;
    u.tick();
    u.click();  // fault page
    u.click();  // menu
    u.actions();
    for (int i = 1; i <= 2; ++i)  // Point, Adjust
    {
        u.turn(1, 2100);
        u.click();
        const auto a = u.actions();
        EXPECT_TRUE(Ui::has(a, Kind::BeepError)) << i;
        EXPECT_EQ(1u, a.size()) << i;
        EXPECT_TRUE(u.inv("RA DRIVER FAULT")) << i;
    }
    Ui v;
    v.m.faultCode = 1;
    v.tick();
    v.click();
    v.click();
    v.actions();
    v.click();  // row 0: Stop tracking stays usable
    EXPECT_TRUE(Ui::has(v.actions(), Kind::TrackOff));
}

TEST(UiFlowsTest, GoRechecksTargetAtClick)
{
    Ui u;
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("CONFIRM"));
    u.actions();
    u.m.lstSeconds = (u.m.lstSeconds + 12 * 3600) % 86400;  // time passes: the target is now far below the horizon
    u.tick(10);
    u.click(700);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("BELOW HORIZON"));
    EXPECT_EQ(0, u.c.violations);
}

// ---------------- Adjust ----------------

namespace {

// Status -> Adjust (RA, step 100).
void adjust(Ui &u)
{
    menu(u, 2);
}

// Adjust options list with the cursor on `row`.
void options(Ui &u, int row)
{
    u.hold();
    if (row)
        u.turn(static_cast<int8_t>(row));
}

// Set the step size by index in {1, 10, 100, 1000, 10000, JOG SLOW, JOG CENTER, JOG FIND, JOG MAX} (default 2), back on Adjust.
void setStep(Ui &u, int idx)
{
    u.hold();  // options, cursor on Step
    clicks(u, (idx - 2 + 9) % 9);
    u.hold();  // main menu
    u.turn(2);
    u.click();
}

bool contains(const Ui &u, const std::string &p)
{
    for (const auto &x : u.c.texts)
        if (x.s.find(p) != std::string::npos)
            return true;
    return false;
}

}  // namespace

TEST(UiFlowsTest, AdjustTurnEmitsMoveSteps)
{
    Ui u;
    u.tick();
    adjust(u);
    EXPECT_TRUE(u.drew("ADJUST"));
    u.actions();
    u.turn(1);
    bool f;
    UiAction a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(100, a.a);
    EXPECT_EQ(0, a.b);
    u.turn(-1);
    a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(-100, a.a);
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, AdjustDetentsCoalescePerTick)
{
    Ui u;
    u.tick();
    adjust(u);
    setStep(u, 4);
    u.actions();
    for (int i = 0; i < 7; ++i)
    {
        u.t += 10;
        u.ui.onTurn(1, u.t);
    }
    u.tick();
    const auto v = u.acts();
    int n = 0;
    for (const UiAction &a : v)
        if (a.kind == Kind::MoveSteps)
        {
            ++n;
            EXPECT_EQ(70000, a.a);
        }
    EXPECT_EQ(1, n);
}

TEST(UiFlowsTest, AdjustStopIsNeverDroppedUnderTurnFlood)
{
    Ui u;
    u.tick();
    adjust(u);
    u.state(MountState::Slewing);
    u.actions();
    u.click();  // STOP
    for (int i = 0; i < 20; ++i)
        u.ui.onTurn(1, u.t);
    u.tick();
    EXPECT_TRUE(u.drew("STOPPED"));
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));
}

TEST(UiFlowsTest, AdjustClickCyclesAxes)
{
    Ui u;
    u.m.hasAz = u.m.hasFocus = true;
    u.tick();
    adjust(u);
    EXPECT_TRUE(u.inv("RA"));
    for (const char *ax : {"DEC", "AZ", "FOCUS", "RA"})
    {
        u.click();
        EXPECT_TRUE(u.inv(ax)) << ax;
    }
    Ui v;
    v.tick();
    adjust(v);
    v.click();
    EXPECT_TRUE(v.inv("DEC"));
    EXPECT_FALSE(v.drew("AZ"));
    v.click();
    EXPECT_TRUE(v.inv("RA"));
}

TEST(UiFlowsTest, AdjustJogUsesJogStartStop)
{
    Ui u;
    u.m.hasAz = true;
    u.tick();
    adjust(u);
    setStep(u, 6);  // JOG CENTER
    EXPECT_TRUE(contains(u, "JOG CENTER"));
    u.actions();
    u.turn(1);
    auto v = u.acts();
    ASSERT_GE(v.size(), 2u);
    EXPECT_EQ(Kind::SetSlewRate, v[0].kind);
    EXPECT_EQ(2, v[0].a);
    EXPECT_EQ(Kind::JogStart, v[1].kind);
    EXPECT_EQ(2, v[1].a);  // E
    u.state(MountState::Jogging);
    u.turn(1);  // same direction: nothing new
    EXPECT_TRUE(u.acts().empty());
    u.turn(-1);  // reverses: stop E, start W
    v = u.acts();
    bool stopE = false, startW = false;
    for (const UiAction &a : v)
    {
        stopE |= a.kind == Kind::JogStop && a.a == 2;
        startW |= a.kind == Kind::JogStart && a.a == 8;
    }
    EXPECT_TRUE(stopE && startW);
    u.tick(300);
    UiAction s;
    EXPECT_TRUE(Ui::find(u.acts(), Kind::JogStop, s));
    EXPECT_EQ(8, s.a);
    // DEC: N/S.
    u.state(MountState::Tracking);
    u.click();
    u.actions();
    u.turn(1);
    EXPECT_TRUE(Ui::find(u.acts(), Kind::JogStart, s));
    EXPECT_EQ(1, s.a);
    u.tick(300);
    u.actions();
    u.state(MountState::Tracking);
    // AZ has no JOG sizes: the step clamps to 10000 and turning moves by steps.
    u.click();
    u.actions();
    u.turn(1);
    bool f;
    const UiAction a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(10000, a.a);
    EXPECT_EQ(2, a.b);
}

TEST(UiFlowsTest, AdjustLongPressOpensOptions)
{
    Ui u;
    u.tick();
    adjust(u);
    u.hold();
    EXPECT_TRUE(starts(u, "Step"));
    EXPECT_TRUE(u.drew("Sync on target"));
    EXPECT_TRUE(u.drew("Set home here"));
    EXPECT_TRUE(u.drew("Go home"));
    u.hold();  // back to the main menu
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, AdjustLongPressWhileMovingIsStop)
{
    for (MountState s : {MountState::Jogging, MountState::Slewing})
    {
        Ui u;
        u.tick();
        adjust(u);
        u.state(s);
        u.actions();
        u.hold();
        EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));
        EXPECT_FALSE(u.drew("Sync on target"));
        EXPECT_TRUE(u.drew("STOPPED"));
        Ui w;  // the options list too
        w.tick();
        adjust(w);
        w.hold();
        w.state(s);
        w.actions();
        w.click();
        EXPECT_TRUE(Ui::has(w.actions(), Kind::Stop));
        EXPECT_TRUE(w.drew("STOPPED"));
    }
}

TEST(UiFlowsTest, AdjustRefusesFailedDriver)
{
    Ui u;
    u.m.hasAz = u.m.hasFocus = true;
    u.m.azFault = true;
    u.tick();
    adjust(u);
    u.click();
    u.click();  // AZ
    u.actions();
    u.turn(1);
    const auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    EXPECT_FALSE(Ui::has(a, Kind::MoveSteps));
    EXPECT_TRUE(u.drew("DRIVER FAULT"));
    u.tick(2100);
    EXPECT_FALSE(u.drew("DRIVER FAULT"));

    Ui v;  // Focus
    v.m.hasFocus = true;
    v.m.focusFault = true;
    v.tick();
    adjust(v);
    clicks(v, 2);  // RA -> DEC -> Focus
    v.actions();
    v.turn(-1);
    EXPECT_FALSE(Ui::has(v.actions(), Kind::MoveSteps));
    EXPECT_TRUE(v.drew("DRIVER FAULT"));
}

TEST(UiFlowsTest, AdjustCyclesAltInDisplayOrder)
{
    struct Case
    {
        bool az, alt, focus;
        std::vector<const char *> order;  // after RA, one click each
    };
    const Case cases[] = {
        {true, true, true, {"DEC", "AZ", "ALT", "FOCUS", "RA"}},
        {false, true, true, {"DEC", "ALT", "FOCUS", "RA"}},
        {true, true, false, {"DEC", "AZ", "ALT", "RA"}},
        {false, true, false, {"DEC", "ALT", "RA"}},
        {true, false, true, {"DEC", "AZ", "FOCUS", "RA"}},
        {false, false, false, {"DEC", "RA"}},
    };
    for (const Case &k : cases)
    {
        Ui u;
        u.m.hasAz = k.az;
        u.m.hasAlt = k.alt;
        u.m.hasFocus = k.focus;
        u.tick();
        adjust(u);
        EXPECT_TRUE(u.inv("RA"));
        EXPECT_EQ(k.alt, u.drew("ALT"));  // the strip lists ALT only when fitted
        for (const char *ax : k.order)
        {
            u.click();
            EXPECT_TRUE(u.inv(ax)) << ax;
        }
        EXPECT_EQ(0, u.c.violations);  // five axes fit the strip
    }
}

TEST(UiFlowsTest, AdjustAltMovesAxis4AndItsFaultLocksOnlyAlt)
{
    Ui u;
    u.m.hasAz = u.m.hasAlt = true;
    u.m.altFault = true;
    u.tick();
    adjust(u);
    clicks(u, 3);  // RA -> DEC -> AZ -> ALT
    ASSERT_TRUE(u.inv("ALT"));
    u.actions();
    u.turn(1);
    const auto refused = u.actions();
    EXPECT_TRUE(Ui::has(refused, Kind::BeepError));
    EXPECT_FALSE(Ui::has(refused, Kind::MoveSteps));
    EXPECT_TRUE(u.drew("ALT DRIVER FAULT"));
    u.tick(2100);
    u.click();  // ALT -> RA: still moves
    u.actions();
    u.turn(1);
    bool f;
    UiAction a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(0, a.b);
    u.tick(400);  // past the step latch
    clicks(u, 2);  // RA -> DEC -> AZ: still moves
    ASSERT_TRUE(u.inv("AZ"));
    u.actions();
    u.turn(1);
    a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(2, a.b);
    u.tick(400);
    u.m.altFault = false;  // a healthy ALT moves as axis 4
    u.click();
    ASSERT_TRUE(u.inv("ALT"));
    u.actions();
    u.turn(-1);
    a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(4, a.b);
    EXPECT_EQ(-100, a.a);
}

TEST(UiFlowsTest, AdjustShowsAltPositionAndDegrees)
{
    Ui u;
    u.m.hasAlt = true;
    u.m.stepsPerDeg[4] = 1000.0f;
    u.m.axisPos[4] = 4242;
    u.tick();
    adjust(u);
    setStep(u, 3);
    clicks(u, 2);  // RA -> DEC -> ALT
    ASSERT_TRUE(u.inv("ALT"));
    EXPECT_TRUE(contains(u, "4242"));
    EXPECT_TRUE(contains(u, "1.00"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, SyncOnTargetNeedsTargetAndClock)
{
    Ui u;  // no target, never reported
    u.m.hasTarget = false;
    u.tick();
    adjust(u);
    options(u, 1);
    u.actions();
    u.click();
    const auto na = u.actions();
    EXPECT_TRUE(Ui::has(na, Kind::BeepError));
    EXPECT_FALSE(Ui::has(na, Kind::Sync));
    EXPECT_TRUE(u.drew("No GoTo target yet"));

    Ui v;  // no clock
    v.m.dateSet = v.m.timeSet = false;
    v.tick();
    v.click();  // date editor
    v.hold();  // menu
    v.turn(2);
    v.click();
    options(v, 1);
    v.actions();
    v.click();
    const auto a = v.actions();
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    EXPECT_FALSE(Ui::has(a, Kind::Sync));
    EXPECT_TRUE(v.drew("Set date+time first"));

    Ui w;  // target and clock
    w.m.targetRaSeconds = 3600;
    w.m.targetDecArcSec = 7200;
    w.tick();
    w.m.hasTarget = false;  // the adapter reports it only while slewing: the UI keeps it
    w.tick(10);
    adjust(w);
    options(w, 1);
    w.actions();
    w.click();
    bool f;
    const UiAction s = get(w, Kind::Sync, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(3600, s.a);
    EXPECT_EQ(7200, s.b);
}

TEST(UiFlowsTest, SetHomeNeedsIdle)
{
    Ui u;  // tracking: locked
    u.tick();
    adjust(u);
    options(u, 2);
    u.actions();
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::BeepError));
    EXPECT_TRUE(u.drew("Stop the mount first"));
    EXPECT_FALSE(u.drew("Set home here?"));

    Ui v;  // idle: confirm, then SetHome
    v.m.state = MountState::Idle;
    v.tick();
    adjust(v);
    options(v, 2);
    v.click();
    EXPECT_TRUE(v.drew("Set home here?"));
    v.actions();
    v.click();
    bool f;
    get(v, Kind::SetHome, f);
    EXPECT_TRUE(f);
    EXPECT_TRUE(v.drew("HOME SET"));

    Ui w;  // the confirm's long press goes back to the options
    w.m.state = MountState::Idle;
    w.tick();
    adjust(w);
    options(w, 2);
    w.click();
    w.hold();
    EXPECT_TRUE(w.drew("Go home"));
    EXPECT_EQ(0, v.c.violations + w.c.violations);
}

TEST(UiFlowsTest, GoHomeAllowedFromTracking)
{
    for (MountState s : {MountState::Tracking, MountState::Idle})
    {
        Ui u;
        u.m.state = s;
        u.tick();
        adjust(u);
        options(u, 3);
        u.actions();
        u.click();
        EXPECT_TRUE(Ui::has(u.actions(), Kind::GoHome));
    }
}

TEST(UiFlowsTest, AdjustWorksWithoutDateTime)
{
    Ui u;
    u.m.dateSet = u.m.timeSet = false;
    u.m.state = MountState::Idle;
    u.tick();
    u.click();  // alert -> date editor
    u.hold();  // menu
    u.turn(2);
    u.click();
    EXPECT_TRUE(u.drew("ADJUST"));
    u.actions();
    u.turn(1);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::MoveSteps));
    u.tick(400);  // the step move latch has expired
    options(u, 2);
    u.click();
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::SetHome));
}

TEST(UiFlowsTest, AdjustShowsDegreesForStep)
{
    Ui u;
    u.m.hasFocus = true;
    u.m.stepsPerDeg[0] = 1000.0f;
    u.m.axisPos[0] = 12345;
    u.m.axisPos[3] = -77;
    u.tick();
    adjust(u);
    setStep(u, 3);
    EXPECT_TRUE(contains(u, "1000"));
    EXPECT_TRUE(contains(u, "1.00"));
    EXPECT_TRUE(contains(u, "12345"));
    clicks(u, 2);  // Focus: steps only
    EXPECT_TRUE(u.inv("FOCUS"));
    EXPECT_TRUE(contains(u, "-77"));
    EXPECT_FALSE(contains(u, "1.00"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, TurningWhileMovingOnlyWorksOnAdjust)
{
    Ui u;  // menu: turning does nothing while moving
    u.tick();
    u.click();
    u.state(MountState::Slewing);
    u.turn(1);
    EXPECT_TRUE(u.inv("STOP"));
    Ui v;  // Adjust: turning continues a step move
    v.tick();
    adjust(v);
    v.state(MountState::Slewing);
    v.actions();
    v.turn(1);
    EXPECT_TRUE(Ui::has(v.actions(), Kind::MoveSteps));
    Ui w;  // but not while homing
    w.tick();
    adjust(w);
    w.state(MountState::Homing);
    w.actions();
    w.turn(1);
    EXPECT_FALSE(Ui::has(w.actions(), Kind::MoveSteps));
}

TEST(UiFlowsTest, AdjustClickRightAfterAMoveIsNotIgnored)
{
    Ui u;
    u.tick();
    adjust(u);
    u.state(MountState::Slewing);
    u.state(MountState::Tracking);  // the step move ended
    u.click(100);
    EXPECT_TRUE(u.inv("DEC"));
}

TEST(UiFlowsTest, StepMoveEndDoesNotShowArrived)
{
    Ui u;
    u.tick();
    adjust(u);
    u.state(MountState::Slewing);
    u.actions();
    u.state(MountState::Tracking);
    EXPECT_FALSE(u.drew("ARRIVED"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Beep));
}

TEST(UiFlowsTest, AdjustIgnoresKnobAcceleration)
{
    Ui u;
    u.tick();
    adjust(u);  // RA, step 100
    u.actions();
    u.t += 10;
    u.ui.onTurn(2, u.t, true);  // fast detents: exactly detents * step
    u.tick();
    bool f = false;
    UiAction a = get(u, Kind::MoveSteps, f);
    EXPECT_TRUE(f);
    EXPECT_EQ(200, a.a);

    Ui e;  // an editor still accelerates
    e.m.targetRaSeconds = 10 * 3600;
    e.tick();
    menu(e, 1);
    EXPECT_TRUE(e.inv("10"));
    e.t += 10;
    e.ui.onTurn(1, e.t, true);
    e.tick();
    EXPECT_TRUE(e.inv("15"));
}

TEST(UiFlowsTest, ClickRightAfterStepMoveIsStop)
{
    Ui u;
    u.tick();
    adjust(u);
    u.actions();
    u.t += 10;
    u.ui.onTurn(1, u.t);  // turn and click land in the same tick
    u.ui.onClick(u.t);
    u.tick();
    const auto v = u.actions();
    EXPECT_TRUE(Ui::has(v, Kind::MoveSteps));
    EXPECT_TRUE(Ui::has(v, Kind::Stop));
    EXPECT_TRUE(u.drew("STOPPED"));
}

TEST(UiFlowsTest, LoadFallsBackToDefaultsPerField)
{
    UiSettings d;
    uint8_t b[UiSettings::kStoreBytes];
    memset(b, 0xFF, sizeof b);  // a read that failed after the marker: MinAlt 0xFF is -1, inside its range
    b[0] = UiSettings::kStoreMarker;
    UiSettings s;
    s.set(SettingId::Language, 1);  // not the defaults before loading: load must reset them
    ASSERT_TRUE(s.load(b));
    EXPECT_EQ(d.get(SettingId::Language), s.get(SettingId::Language));
    EXPECT_EQ(d.get(SettingId::Beep), s.get(SettingId::Beep));
    // The adapter zeroes the whole block when the read failed: marker gone, everything default.
    UiSettings t;
    t.set(SettingId::MinAlt, 5);
    memset(b, 0, sizeof b);
    EXPECT_FALSE(t.load(b));
    EXPECT_EQ(d.get(SettingId::MinAlt), t.get(SettingId::MinAlt));
}

TEST(UiFlowsTest, FactoryResetReloadReturnsDefaults)
{
    Ui u;
    UiSettings s;
    s.set(SettingId::Language, 1);
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    u.ui.loadSettings(b);
    EXPECT_EQ(1, u.ui.settings().get(SettingId::Language));
    memset(b, 0, sizeof b);  // :XFR cleared the store
    EXPECT_EQ(0, u.ui.loadSettings(b));
    EXPECT_EQ(0, u.ui.settings().get(SettingId::Language));
}

TEST(UiFlowsTest, SettingsStoreLatchWritesOnChangeOnly)
{
    core::SettingsStoreLatch l;
    UiSettings s;
    uint8_t b[UiSettings::kStoreBytes];
    s.save(b);
    EXPECT_FALSE(l.observe(b));  // boot: records, never writes
    EXPECT_FALSE(l.observe(b));
    s.set(SettingId::Beep, 0);
    s.save(b);
    EXPECT_TRUE(l.observe(b));  // a change writes once
    EXPECT_FALSE(l.observe(b));
    s.set(SettingId::Beep, 1);
    s.save(b);
    l.reset();  // after :XFR: the next pass records without writing
    EXPECT_FALSE(l.observe(b));
    EXPECT_FALSE(l.observe(b));
    s.set(SettingId::Beep, 0);
    s.save(b);
    EXPECT_TRUE(l.observe(b));
}

TEST(UiFlowsTest, StaleBridgeStatusStaysExpired)
{
    core::NetStatus n{};
    n.seen = true;
    n.atMs = 1000;
    core::expireNet(n, 1000 + core::kNoBridgeMs - 1);
    EXPECT_TRUE(n.seen);
    core::expireNet(n, 1000 + core::kNoBridgeMs);
    EXPECT_FALSE(n.seen);
    core::expireNet(n, 1000 + 0x80000000u + 10);  // ~24.8 days later: must not come back
    EXPECT_FALSE(n.seen);
    n.seen = true;  // a new :XNM stamps atMs again
    n.atMs = 2000;
    core::expireNet(n, 1500);  // atMs slightly ahead of now (stamped after the tick began)
    EXPECT_TRUE(n.seen);
}
