#include <gtest/gtest.h>
#include <cmath>
#include <cstdio>
#include <string>
#include "UiHarness.hpp"
#include "core/ui/SkyMath.hpp"

// L3/L4: GoTo, Sync, Setup, Jog, Home/Park, Settings, Turkish, width lint.

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
    menu(u, 7);
    u.click();
    u.hold();
    u.hold();
}

// GoTo through both editors to the confirm/refused screen, with the prefilled values.
void gotoConfirm(Ui &u)
{
    menu(u, 2);
    clicks(u, 3);  // RA
    clicks(u, 4);  // Dec
}

}  // namespace

// ---------------- L3: GoTo ----------------

TEST(UiFlowsTest, GotoEditorsPrefilledWithLastTarget)
{
    Ui u;
    u.m.targetRaSeconds = 10 * 3600 + 20 * 60 + 30;
    u.m.targetDecArcSec = 12 * 3600 + 34 * 60 + 56;
    u.tick();
    menu(u, 2);
    EXPECT_TRUE(u.drew("GOTO RA"));
    EXPECT_TRUE(u.inv("10"));
    EXPECT_TRUE(u.drew(":20:30"));
    clicks(u, 3);
    EXPECT_TRUE(u.drew("GOTO DEC"));
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
    menu(v, 2);
    EXPECT_TRUE(v.inv("10"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, HoldGoesToPreviousGroup)
{
    Ui u;
    u.m.targetRaSeconds = 10 * 3600 + 20 * 60 + 30;
    u.tick();
    menu(u, 2);
    u.click();
    EXPECT_TRUE(u.inv("20"));
    u.hold();
    EXPECT_TRUE(u.inv("10"));
    clicks(u, 3);  // Dec editor, sign group
    EXPECT_TRUE(u.drew("GOTO DEC"));
    u.hold();  // first group: back to the RA editor's last group
    EXPECT_TRUE(u.drew("GOTO RA"));
    EXPECT_TRUE(u.inv("30"));
    u.hold();
    u.hold();
    u.hold();  // first group of the first editor: cancel
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
}

TEST(UiFlowsTest, GotoConfirmShowsAltAzSun)
{
    Ui u;
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("GOTO"));
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
    EXPECT_TRUE(u.drew("hold: cancel"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, GoNotArmedBefore600ms)
{
    Ui u;
    u.tick();
    gotoConfirm(u);
    u.actions();
    EXPECT_FALSE(u.drew("CLICK = GO"));
    u.click(300);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_TRUE(u.drew("CONFIRM"));
    u.tick(290);
    EXPECT_FALSE(u.drew("CLICK = GO"));
    u.tick(10);  // 600 ms after the screen appeared
    EXPECT_TRUE(u.inv("CLICK = GO"));
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Goto));
}

TEST(UiFlowsTest, GotoEmitsActionAfterConfirm)
{
    Ui u;
    u.m.targetRaSeconds = 6 * 3600 + 1;
    u.m.targetDecArcSec = 10 * 3600 + 2;
    u.tick();
    gotoConfirm(u);
    u.actions();
    u.click(700);
    bool f;
    const UiAction a = get(u, Kind::Goto, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(6 * 3600 + 1, a.a);
    EXPECT_EQ(10 * 3600 + 2, a.b);
    EXPECT_TRUE(u.drew("TRACKING"));  // back on the status screen
    EXPECT_TRUE(u.drew("click: menu"));
}

TEST(UiFlowsTest, GotoRefusedBelowHorizonKeepsValues)
{
    Ui u;
    u.m.targetDecArcSec = -60 * 3600;  // far south from 41 N
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("GOTO"));
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("BELOW HORIZON"));
    EXPECT_TRUE(starts(u, "Alt -"));
    EXPECT_TRUE(u.drew("click: edit"));
    u.click(1000);
    EXPECT_TRUE(u.drew("GOTO RA"));
    EXPECT_TRUE(u.inv("05"));
    clicks(u, 3);
    EXPECT_TRUE(u.drew("60\xC2\xB0"
                       "00'00\""));  // value kept
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, GotoRefusedNearSun)
{
    Ui u;
    float sra, sdec;
    core::sunRaDec(2026, 11, 10, (u.m.localSeconds - 3 * 3600) / 3600.0f, sra, sdec);
    u.m.targetRaSeconds = (int32_t)(sra * 3600);
    u.m.targetDecArcSec = (int32_t)(sdec * 3600);
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("TOO CLOSE TO SUN"));
    EXPECT_TRUE(starts(u, "Sun 0"));
    EXPECT_TRUE(u.drew("click: edit"));
    u.click(1000);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, GotoRefusedInvalid)
{
    Ui u;
    u.m.year = 2150;  // impossible for the sun model: fail closed
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("REFUSED"));
    EXPECT_TRUE(u.drew("INVALID INPUT"));
    u.click(1000);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Goto));
}

// ---------------- L3: Sync ----------------

TEST(UiFlowsTest, SyncConfirmShowsOffset)
{
    Ui u;
    u.tick();
    menu(u, 3);
    EXPECT_TRUE(u.drew("SYNC RA"));
    EXPECT_TRUE(u.inv("05"));  // prefilled with the current position
    u.click();
    u.turn(12);  // minutes 35 -> 47
    u.click();
    u.click();
    EXPECT_TRUE(u.drew("SYNC DEC"));
    u.click();
    u.turn(1);  // degrees 5 -> 6 (south)
    clicks(u, 3);
    EXPECT_TRUE(u.drew("SYNC"));
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

TEST(UiFlowsTest, SyncBigJumpFrameAbove5deg)
{
    Ui u;
    u.tick();
    menu(u, 3);
    clicks(u, 4);
    u.turn(6);  // degrees 5 -> 11: 6 degrees
    clicks(u, 3);
    EXPECT_TRUE(u.drew("BIG JUMP - CHECK"));
    u.tick(600);  // armed
    EXPECT_TRUE(u.inv("CLICK = SYNC"));
    EXPECT_EQ(0, u.c.violations);

    Ui v;
    v.tick();
    menu(v, 3);
    clicks(v, 4);
    v.turn(4);  // 4 degrees: no frame
    clicks(v, 3);
    EXPECT_TRUE(v.drew("CONFIRM"));
    EXPECT_FALSE(v.drew("BIG JUMP - CHECK"));
}

TEST(UiFlowsTest, SyncOnTargetUsesLastTarget)
{
    Ui u;
    u.m.targetRaSeconds = 7 * 3600 + 8 * 60 + 9;
    u.m.targetDecArcSec = 20 * 3600;
    u.tick();
    u.m.hasTarget = false;  // arrived: the adapter stops reporting it
    u.tick(10);
    menu(u, 4);
    EXPECT_TRUE(u.drew("CONFIRM"));
    EXPECT_TRUE(starts(u, "07:08:09 +20"));  // a big jump: coordinates on one line
    u.actions();
    u.click(700);
    bool f;
    const UiAction a = get(u, Kind::Sync, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(7 * 3600 + 8 * 60 + 9, a.a);
    EXPECT_EQ(20 * 3600, a.b);

    Ui v;  // hold cancels
    v.tick();
    menu(v, 4);
    v.hold();
    EXPECT_TRUE(v.drew("MENU"));
    EXPECT_FALSE(Ui::has(v.actions(), Kind::Sync));
}

TEST(UiFlowsTest, SyncedHeader2s)
{
    Ui u;
    u.tick();
    menu(u, 4);
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
    menu(u, 5);
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
    menu(v, 5);
    v.click();
    EXPECT_TRUE(v.inv("2026"));
    EXPECT_TRUE(v.drew("-11-10"));
}

TEST(UiFlowsTest, ClearsSyncConfirmBeforeTimeChange)
{
    Ui u;
    u.m.synced = true;
    u.tick();
    menu(u, 5);
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
        menu(v, 5);
        v.turn(static_cast<int8_t>(r));
        v.click();
        clicks(v, r == 2 ? 2 : 3);  // the UTC editor has two groups
        EXPECT_TRUE(v.drew("Clears Sync. Sure?")) << r;
    }
    Ui w;
    w.m.synced = true;
    w.tick();
    menu(w, 5);
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
    menu(u, 5);
    u.turn(1);
    u.click();  // Time
    clicks(u, 2);
    EXPECT_TRUE(u.drew("Clears Sync. Sure?"));
    u.state(MountState::Slewing);
    u.click();  // STOP
    u.state(MountState::Tracking, 1000);
    u.m.synced = false;
    u.tick(600);  // past the row-0 click ignore
    menu(u, 5);
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
    menu(u, 5);
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
    menu(t, 5);
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
    menu(u, 5);
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

// ---------------- L4: Jog ----------------

TEST(UiFlowsTest, JogStartsAndAutoStops300ms)
{
    Ui u;
    u.tick();
    menu(u, 1);
    EXPECT_TRUE(u.drew("JOG"));
    u.click();  // Jog RA
    EXPECT_TRUE(u.drew("JOG RA"));
    u.actions();
    u.turn(1);
    bool f;
    UiAction a = get(u, Kind::JogStart, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(2, a.a);  // right = east
    u.state(MountState::Jogging, 0);
    EXPECT_TRUE(u.drew("JOG RA"));  // stays on the jog screen while moving
    u.turn(2, 100);  // still turning: no new start, no stop
    auto v = u.actions();
    EXPECT_FALSE(Ui::has(v, Kind::JogStart));
    EXPECT_FALSE(Ui::has(v, Kind::JogStop));
    u.tick(299);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::JogStop));
    u.tick(2);  // 301 ms after the last detent
    a = get(u, Kind::JogStop, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(2, a.a);  // stops the direction it started
    u.state(MountState::Tracking, 10);

    u.turn(-1, 600);  // left = west
    a = get(u, Kind::JogStart, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(8, a.a);
    u.turn(1);  // reversal: stop west first, then east
    v = u.actions();
    ASSERT_EQ(2u, v.size());
    EXPECT_EQ(Kind::JogStop, v[0]);
    EXPECT_EQ(Kind::JogStart, v[1]);
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, StaleTurnsDropped)
{
    Ui u;
    u.tick();
    menu(u, 1);
    u.click();
    u.actions();
    // Spun while the UI loop was blocked: the merged turn carries the newest stamp, still > 300 ms old.
    u.ui.onTurn(1, u.t + 10);
    u.ui.onTurn(3, u.t + 50);
    u.tick(1000);
    EXPECT_TRUE(u.actions().empty());
    u.turn(1);  // a fresh detent jogs
    EXPECT_TRUE(Ui::has(u.actions(), Kind::JogStart));
}

TEST(UiFlowsTest, JogClickTogglesAxis)
{
    Ui u;
    u.tick();
    menu(u, 1);
    u.click();
    EXPECT_TRUE(u.drew("turn: E-W click:axis"));
    u.click();
    EXPECT_TRUE(u.drew("JOG DEC"));
    EXPECT_TRUE(u.drew("turn: N-S click:axis"));
    u.actions();
    u.turn(1);
    bool f;
    UiAction a = get(u, Kind::JogStart, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(1, a.a);  // north
    u.state(MountState::Jogging, 0);
    u.click();  // moving: click is STOP (M1), not an axis toggle
    auto v = u.actions();
    EXPECT_TRUE(Ui::has(v, Kind::Stop));
    EXPECT_TRUE(Ui::has(v, Kind::BeepLong));
}

TEST(UiFlowsTest, JogHoldReturnsToSubmenu)
{
    Ui u;
    u.tick();
    menu(u, 1);
    u.turn(1);
    u.click();  // Jog DEC
    EXPECT_TRUE(u.drew("JOG DEC"));
    u.hold();
    EXPECT_TRUE(u.inv("Jog DEC"));
    u.hold();
    EXPECT_TRUE(u.drew("MENU"));
}

TEST(UiFlowsTest, SpeedCyclesFourRates)
{
    Ui u;
    u.m.slewRate = 1;
    u.tick();
    menu(u, 1);
    u.turn(2);
    const char *names[] = {"SLOW", "CENTER", "FIND", "MAX"};
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(u.inv(row("Speed", names[i]).c_str())) << i;
        u.actions();
        u.click();
        bool f;
        const UiAction a = get(u, Kind::SetSlewRate, f);
        ASSERT_TRUE(f);
        EXPECT_EQ(i == 3 ? 1 : i + 2, a.a);
        u.m.slewRate = static_cast<uint8_t>(a.a);  // the mount applies it
        u.tick(10);
    }
    EXPECT_TRUE(u.inv(row("Speed", "SLOW").c_str()));
    u.click(10);
    u.m.slewRate = 2;
    u.turn(-2);
    u.click();  // jog screen shows the rate
    EXPECT_TRUE(u.drew("CENTER"));
}

// ---------------- L4: Home / Park ----------------

TEST(UiFlowsTest, ParkedShowsOnlyUnpark)
{
    Ui u;
    u.m.state = MountState::Parked;
    u.tick();
    menu(u, 6);
    EXPECT_TRUE(u.drew("HOME / PARK"));
    EXPECT_TRUE(u.inv("Unpark"));
    EXPECT_FALSE(u.drew("Go home"));
    EXPECT_FALSE(u.drew("Park"));
    u.actions();
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Unpark));

    Ui v;
    v.tick();
    menu(v, 6);
    EXPECT_TRUE(v.inv("Go home"));
    EXPECT_TRUE(v.drew("Set home here"));
    EXPECT_TRUE(v.drew("Park"));
    EXPECT_FALSE(v.drew("Unpark"));
    v.actions();
    v.click();
    EXPECT_TRUE(Ui::has(v.actions(), Kind::GoHome));
}

TEST(UiFlowsTest, SetHomeNeedsConfirm)
{
    Ui u;
    u.tick();
    menu(u, 6);
    u.turn(1);
    u.click();
    EXPECT_TRUE(u.drew("HOME"));
    EXPECT_TRUE(u.drew("Set home here?"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::SetHome));
    u.hold();  // cancel
    EXPECT_TRUE(u.drew("HOME / PARK"));
    EXPECT_FALSE(Ui::has(u.actions(), Kind::SetHome));
    u.click();
    u.click();  // yes
    EXPECT_TRUE(Ui::has(u.actions(), Kind::SetHome));
    EXPECT_TRUE(u.drew("HOME SET"));
    u.tick(2010);
    EXPECT_FALSE(u.drew("HOME SET"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, ParkingScreenAndStop)
{
    Ui u;
    u.tick();
    menu(u, 6);
    u.turn(2);
    u.actions();
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Park));
    u.state(MountState::Parking);  // started, then polled
    EXPECT_TRUE(u.drew("PARKING"));
    EXPECT_TRUE(u.drew("> park position"));
    EXPECT_TRUE(u.inv("click: STOP"));
    u.click();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));

    Ui v;
    v.m.state = MountState::Homing;
    v.tick();
    EXPECT_TRUE(v.drew("HOMING"));
    EXPECT_TRUE(v.drew("> home position"));
    EXPECT_EQ(0, u.c.violations + v.c.violations);
}

TEST(UiFlowsTest, HomeParkMotionLockedDuringFault)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.faultCode = 1;
    u.tick();
    u.click();  // ok
    menu(u, 6);
    for (int i = 0; i < 3; ++i)  // Go home, Set home here, Park
    {
        u.actions();
        u.click(2100);
        const auto a = u.actions();
        EXPECT_TRUE(Ui::has(a, Kind::BeepError)) << i;
        EXPECT_FALSE(Ui::has(a, Kind::GoHome) || Ui::has(a, Kind::Park)) << i;
        EXPECT_FALSE(u.drew("Set home here?")) << i;
        u.turn(1);
    }
    Ui p;
    p.m.state = MountState::Parked;
    p.m.faultCode = 2;
    p.tick();
    p.click();
    menu(p, 6);
    p.actions();
    p.click();
    EXPECT_FALSE(Ui::has(p.actions(), Kind::Unpark));
}

// ---------------- L4: Settings ----------------

TEST(UiFlowsTest, SettingsFromTable)
{
    Ui u;
    u.tick();
    menu(u, 7);
    EXPECT_TRUE(u.drew("SETTINGS"));
    EXPECT_TRUE(u.inv(row("Dil/Lang", "English").c_str()));
    EXPECT_TRUE(u.drew(row("Min alt", deg(0)).c_str()));
    EXPECT_TRUE(u.drew(row("Beep", "On").c_str()));
    EXPECT_TRUE(u.drew(row("Supply", "24 V").c_str()));
    EXPECT_TRUE(u.drew("Back"));
    u.turn(4);
    u.click();  // Back
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiFlowsTest, LanguageSwitchRedrawsTurkish)
{
    Ui u;
    u.tick();
    menu(u, 7);
    u.click();
    EXPECT_TRUE(u.drew("AYARLAR"));
    EXPECT_TRUE(u.inv(row("Dil/Lang", "Turkce").c_str()));
    EXPECT_TRUE(u.drew(row("Bip", "Acik").c_str()));
    u.hold();
    u.turn(7);
    EXPECT_TRUE(u.drew("Ayarlar"));
    u.turn(-7);
    EXPECT_TRUE(u.drew("Motorla kaydir"));
    u.hold();
    EXPECT_TRUE(u.drew("TAKIPTE"));
    EXPECT_TRUE(u.drew("tikla: menu"));
    toggleLang(u);
    EXPECT_TRUE(u.drew("TRACKING"));
}

TEST(UiFlowsTest, MinAltUsedByGoto)
{
    Ui u;
    u.m.targetDecArcSec = -40 * 3600;  // about +9 degrees from 41 N near the meridian
    u.tick();
    gotoConfirm(u);
    EXPECT_TRUE(u.drew("CONFIRM"));
    u.hold();
    u.hold();  // status
    menu(u, 7);
    u.turn(1);
    u.click();  // Min alt editor
    EXPECT_TRUE(u.drew("MIN ALT"));
    u.click();
    u.turn(30);
    u.click();
    EXPECT_TRUE(u.drew(row("Min alt", deg(30)).c_str()));
    u.hold();
    u.hold();
    gotoConfirm(u);
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
    u.turn(2);
    u.click();  // Beep -> Off
    EXPECT_TRUE(u.inv(row("Beep", "Off").c_str()));
    u.actions();
    u.hold();
    u.click(1000);  // locked Track here: the error beep stays
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    u.turn(5, 2100);
    u.click();  // Setup: no click tick
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
    menu(u, 7);
    u.turn(3);
    u.actions();
    u.click();
    bool f;
    UiAction a = get(u, Kind::SetSupplyVoltage, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(12, a.a);
    u.m.supplyVoltage = 12;
    u.click();
    a = get(u, Kind::SetSupplyVoltage, f);
    ASSERT_TRUE(f);
    EXPECT_EQ(24, a.a);
}

TEST(UiFlowsTest, SupplyRowShowsSnapshotValue)
{
    Ui u;
    u.m.supplyVoltage = 12;
    u.tick();
    menu(u, 7);
    EXPECT_TRUE(u.drew(row("Supply", "12 V").c_str()));
    u.turn(3);
    u.click();  // asks for 24 V; the row waits for the firmware
    EXPECT_TRUE(u.inv(row("Supply", "12 V").c_str()));
    u.m.supplyVoltage = 24;
    u.tick(10);
    EXPECT_TRUE(u.inv(row("Supply", "24 V").c_str()));
}

// ---------------- L4: width lint ----------------

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

    for (int r = 0; r < 9; ++r)  // menu, every row in view
    {
        u.click(1000);
        u.turn(static_cast<int8_t>(r));
        u.hold();
    }
    menu(u, 1);  // jog menu, jog screens
    u.click();
    u.click(600);
    u.hold();
    u.hold();
    u.hold();

    // GoTo: editors, confirm (armed), back. Latitude 89:59 S: a southern target is up.
    u.m.targetDecArcSec = -60 * 3600;
    u.tick(10);
    menu(u, 2);
    for (int i = 0; i < 7; ++i)
        u.click();
    u.tick(700);
    u.hold();
    u.hold();
    // Refusals.
    u.m.targetDecArcSec = 89 * 3600;
    u.m.targetRaSeconds = 0;
    u.tick(10);
    gotoConfirm(u);  // below
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
    u.hold();
    u.hold();
    u.m.year = 2150;
    u.tick(10);
    gotoConfirm(u);  // invalid
    u.hold();
    u.hold();
    u.m.year = 2026;
    u.m.utcOffsetHours = -12;

    // Sync with the largest offset: RA +12 h, Dec -89:59:59 -> +89:59:59.
    menu(u, 3);
    u.turn(12);
    clicks(u, 3);
    u.turn(1);
    clicks(u, 4);
    u.click(700);  // armed
    u.tick(10);
    menu(u, 4);  // sync on target
    u.hold();
    u.hold();

    // Setup list and every editor, Clears Sync.
    menu(u, 5);
    for (int r = 0; r < 5; ++r)
    {
        u.click();
        for (int i = 0; i < 3; ++i)
            u.click();
        if (u.drew("Clears Sync. Sure?") || u.drew("Esitleme silinsin?"))
            u.hold();
        u.turn(1);
    }
    u.hold();  // menu
    u.hold();  // status

    // Home / Park both variants, Set home confirm.
    menu(u, 6);
    u.turn(1);
    u.click();
    u.hold();
    u.hold();
    u.hold();
    u.state(MountState::Parked);
    menu(u, 6);
    u.hold();
    u.hold();
    u.state(MountState::Idle);

    // Settings with min alt -10, its editor, every message.
    menu(u, 7);
    u.turn(1);
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

    // Locked rows and messages ("No GoTo target yet" needs a fresh controller: see AllScreensFit).
    u.click(2100);  // menu
    u.m.timeSet = false;
    u.tick(10);
    for (int r = 0; r < 5; ++r)
    {
        u.click(2100);
        u.turn(1);
    }
    u.hold();
    u.m.timeSet = true;
    u.state(MountState::Parked);
    u.click(2100);
    u.click();  // Track here while parked
    u.hold(2100);
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
    for (const char *s : {"GOTO RA", "GOTO DEC", "CLICK = GO", "BELOW HORIZON", "TOO CLOSE TO SUN", "INVALID INPUT", "SYNC RA", "SYNC DEC",
                          "BIG JUMP - CHECK", "SYNCED", "SETUP", "SETUP DATE", "SETUP TIME", "SETUP UTC", "SETUP LAT", "SETUP LON",
                          "Clears Sync. Sure?", "HOME / PARK", "Unpark", "Set home here?", "SETTINGS", "MIN ALT", "JOG", "JOG RA", "JOG DEC",
                          "PARKING", "HOMING", "> park position", "Set date+time first", "Unpark first",
                          "REBOOTED - SET TIME", "DEC DRIVER FAULT", "turn to wake", "click: STOP"})
        EXPECT_EQ(1u, u.c.seen.count(s)) << s;
    u.c.seen.clear();
    toggleLang(u);
    EXPECT_TRUE(u.drew("tikla: menu") || u.drew("TARIH/SAAT GIR"));
    EXPECT_EQ(0, walk(u));
    for (const char *s : {"GIT RA", "TIKLA = GIT", "UFKUN ALTINDA", "GUNESE COK YAKIN", "GECERSIZ", "BUYUK FARK-KONTROL", "ESITLENDI",
                          "KURULUM BOYLAM", "Esitleme silinsin?", "Evi buraya ayarla?", "AYARLAR", "MIN YUKS", "DEC KAYDIR",
                          "PARK EDIYOR", "EVE GIDIYOR", "Once tarih/saat gir", "cevir: uyan", "tikla: DURDUR"})
        EXPECT_EQ(1u, u.c.seen.count(s)) << s;

    // "No GoTo target yet" in both languages (only before the first target).
    for (int lang = 0; lang < 2; ++lang)
    {
        Ui v;
        v.m.hasTarget = false;
        v.tick();
        if (lang)
            toggleLang(v);
        menu(v, 4);
        EXPECT_TRUE(v.inv(lang ? "Henuz hedef yok" : "No GoTo target yet"));
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
    for (int i = 1; i <= 4; ++i)  // Jog, GoTo, Sync, Sync on target
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

TEST(UiFlowsTest, SyncNotArmedBefore600ms)
{
    Ui u;
    u.tick();
    menu(u, 4);  // Sync on target
    EXPECT_TRUE(u.drew("CONFIRM"));
    EXPECT_FALSE(u.drew("CLICK = SYNC"));
    u.actions();
    u.click(300);
    EXPECT_FALSE(Ui::has(u.actions(), Kind::Sync));
    EXPECT_TRUE(u.drew("CONFIRM"));
    u.tick(290);
    u.click(10);  // 610 ms after the screen appeared
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Sync));

    Ui v;  // Sync RA/Dec: same arming
    v.tick();
    menu(v, 3);
    clicks(v, 4);
    v.turn(1);
    clicks(v, 3);
    EXPECT_TRUE(v.drew("CONFIRM"));
    v.actions();
    v.click(100);
    EXPECT_FALSE(Ui::has(v.actions(), Kind::Sync));
    v.click(600);
    EXPECT_TRUE(Ui::has(v.actions(), Kind::Sync));
}

TEST(UiFlowsTest, JogAxisToggleWorksRightAfterJogEnds)
{
    Ui u;
    u.tick();
    menu(u, 1);
    u.click();  // Jog RA
    u.actions();
    u.turn(1);
    u.state(MountState::Jogging, 0);
    u.tick(301);  // auto stop
    u.actions();
    u.state(MountState::Tracking, 10);  // row 0 flips back: the 500 ms click-ignore starts
    u.click(10);
    EXPECT_TRUE(u.drew("JOG DEC"));  // the toggle works

    u.turn(1);  // north
    u.state(MountState::Jogging, 0);
    u.actions();
    u.click(10);  // still moving: STOP
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));
}

TEST(UiFlowsTest, UnparkNeedsDateAndTime)
{
    Ui u;
    u.m.state = MountState::Parked;
    u.tick();
    menu(u, 6);
    u.m.timeSet = false;  // reboot while parked: time is lost
    u.tick(10);
    u.actions();
    u.click(2100);
    const auto a = u.actions();
    EXPECT_FALSE(Ui::has(a, Kind::Unpark));
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    EXPECT_TRUE(u.drew("Set date+time first"));
    u.m.timeSet = true;
    u.tick(10);
    u.click(2100);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Unpark));
}
