#include <gtest/gtest.h>
#include "UiHarness.hpp"


TEST(UiControllerTest, BootShowsStatusWithAlert)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.dateSet = false;
    u.m.timeSet = false;
    EXPECT_TRUE(u.tick());
    EXPECT_TRUE(u.drew("IDLE"));
    EXPECT_TRUE(u.inv("SET DATE & TIME"));
    EXPECT_TRUE(u.drew("!"));
    EXPECT_TRUE(u.drew("--:--:--"));
    u.m.dateSet = true;
    u.tick(10);
    EXPECT_TRUE(u.inv("SET TIME"));
    u.m.dateSet = false;
    u.m.timeSet = true;
    u.tick(10);
    EXPECT_TRUE(u.inv("SET DATE"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, StatusTrackingLayout)
{
    Ui u;
    u.tick();
    EXPECT_TRUE(u.drew("TRACKING"));
    EXPECT_TRUE(u.drew("23:41:07"));
    EXPECT_TRUE(u.drew("RA 05:35:17 HA-00:37"));
    EXPECT_TRUE(u.drew("DE -05\xC2\xB0"
                       "23'28\""));
    EXPECT_TRUE(u.drew("LST 04:58:02 UTC+3"));
    EXPECT_TRUE(u.drew("click: menu"));
    EXPECT_FALSE(u.drew("!"));
    u.m.state = MountState::Slewing;
    u.tick(10);
    EXPECT_TRUE(u.drew("> 05:35:17 -05\xC2\xB0"
                       "23'"));
    EXPECT_TRUE(u.inv("click: STOP"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, AlertClickOpensDateEditor)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.dateSet = false;
    u.m.timeSet = false;
    u.ui.setLastSavedDate(20261110);
    u.tick();
    u.click();
    EXPECT_TRUE(u.drew("SETUP DATE"));
    EXPECT_TRUE(u.inv("2026"));
    EXPECT_TRUE(u.drew("-11-10"));
    EXPECT_TRUE(u.drew("year"));
    u.click();
    EXPECT_TRUE(u.inv("11"));
    EXPECT_TRUE(u.drew("month"));
    EXPECT_TRUE(u.actions().size() <= 2u);  // only ticks, no SetDate in L2
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, MenuOpensAtRow0)
{
    Ui u;
    u.tick();
    u.click();
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_TRUE(u.inv("Stop tracking"));
    u.turn(3);
    EXPECT_TRUE(u.inv("Settings"));
    u.hold();
    EXPECT_TRUE(u.drew("click: menu"));
    u.click();
    EXPECT_TRUE(u.inv("Stop tracking"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, Row0LabelFollowsState)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.tick();
    u.click();
    EXPECT_TRUE(u.drew("Track here"));
    u.state(MountState::Tracking, 1000);
    EXPECT_TRUE(u.drew("Stop tracking"));
    u.state(MountState::Slewing, 1000);
    EXPECT_TRUE(u.drew("STOP"));
    EXPECT_FALSE(u.drew("Track here"));
}

TEST(UiControllerTest, ClickIgnoredAfterSlotChange)
{
    Ui u;
    u.tick();
    u.click(1000);  // menu, row 0 = Stop tracking
    u.actions();
    // LX200 ends tracking: label becomes Track here; a click aimed at Stop tracking must not track.
    u.state(MountState::Idle, 1000);
    u.click(200);
    EXPECT_TRUE(u.actions().empty());
    u.click(301);  // 501 ms after the change
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::TrackOn));

    // Track here -> STOP: the click stops, never tracks.
    Ui v;
    v.m.state = MountState::Idle;
    v.tick();
    v.click(1000);
    v.actions();
    v.state(MountState::Slewing, 1000);
    v.click(100);
    a = v.actions();
    EXPECT_TRUE(Ui::has(a, Kind::Stop));
    EXPECT_FALSE(Ui::has(a, Kind::TrackOn));

    // STOP -> Track here as the slew ends: the click aimed at STOP does nothing.
    Ui w;
    w.m.state = MountState::Idle;
    w.tick();
    w.click(1000);
    w.state(MountState::Slewing, 1000);
    EXPECT_TRUE(w.inv("STOP"));
    w.state(MountState::Idle, 2000);  // arrival beeps
    w.actions();
    w.click(100);
    EXPECT_TRUE(w.actions().empty());
    w.click(401);
    EXPECT_TRUE(Ui::has(w.actions(), Kind::TrackOn));
}

TEST(UiControllerTest, AnyClickStopsLx200Motion)
{
    Ui u;
    u.tick();
    u.click();
    u.turn(3);  // Settings
    u.click(1000);
    EXPECT_TRUE(u.drew("SETTINGS"));
    u.actions();
    u.state(MountState::Slewing, 1000);
    EXPECT_TRUE(u.inv("click: STOP"));  // footer of every screen while moving
    u.click();
    auto a = u.actions();
    ASSERT_EQ(2u, a.size());
    EXPECT_EQ(Kind::Stop, a[0]);
    EXPECT_EQ(Kind::BeepLong, a[1]);
    u.state(MountState::Idle, 10);
    EXPECT_TRUE(u.drew("STOPPED"));
    u.tick(1900);
    EXPECT_TRUE(u.drew("STOPPED"));
    u.tick(200);
    EXPECT_FALSE(u.drew("STOPPED"));
    EXPECT_TRUE(u.drew("IDLE"));

    // In an editor and on the fault page too.
    Ui e;
    e.m.state = MountState::Idle;
    e.m.timeSet = false;
    e.tick();
    e.click();  // alert -> date editor
    EXPECT_TRUE(e.drew("SETUP DATE"));
    e.state(MountState::Slewing, 1000);
    e.actions();
    e.click();
    EXPECT_TRUE(Ui::has(e.actions(), Kind::Stop));
    e.m.faultCode = 1;
    e.tick(10);
    e.actions();
    e.click();
    EXPECT_TRUE(Ui::has(e.actions(), Kind::Stop));
}

TEST(UiControllerTest, StopThenSlowDecelerationIsNotArrived)
{
    Ui u;
    u.m.state = MountState::Slewing;
    u.tick();
    u.click();  // STOP
    u.tick(2500);  // still decelerating after STOPPED expired
    u.actions();
    u.state(MountState::Idle, 10);
    EXPECT_FALSE(u.drew("ARRIVED"));
    EXPECT_TRUE(u.actions().empty());
}

TEST(UiControllerTest, ClickStampedBeforeSlotChangeTickIsIgnored)
{
    Ui u;
    u.tick();
    u.click(1000);  // menu, Stop tracking
    u.state(MountState::Idle, 1000);
    u.actions();
    u.ui.onClick(u.t - 5);  // input queued before the tick that saw the change
    u.tick();
    EXPECT_TRUE(u.actions().empty());
}

TEST(UiControllerTest, LongPressAlsoStopsWhileMoving)
{
    for (MountState s : {MountState::Slewing, MountState::Jogging, MountState::Parking, MountState::Homing})
    {
        Ui u;
        u.m.state = s;
        u.tick();
        u.hold();
        auto a = u.actions();
        EXPECT_TRUE(Ui::has(a, Kind::Stop));
        EXPECT_TRUE(Ui::has(a, Kind::BeepLong));
    }
}

TEST(UiControllerTest, TurnDoesNothingWhileMoving)
{
    Ui u;
    u.tick();
    u.click();
    u.turn(2);
    u.state(MountState::Slewing, 1000);
    EXPECT_TRUE(u.inv("STOP"));  // cursor moved to row 0 as motion started
    u.actions();
    u.turn(3);
    EXPECT_TRUE(u.inv("STOP"));
    EXPECT_TRUE(u.actions().empty());
}

TEST(UiControllerTest, LockedRowDoubleBeepAndMessage)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.dateSet = false;
    u.m.timeSet = false;
    u.tick();
    u.click();  // alert -> date editor
    u.hold();   // cancel -> menu
    EXPECT_TRUE(u.drew("MENU"));
    EXPECT_TRUE(u.inv("Track here   (time)"));
    u.actions();
    u.click(1000);
    auto a = u.actions();
    ASSERT_EQ(1u, a.size());
    EXPECT_EQ(Kind::BeepError, a[0]);
    EXPECT_TRUE(u.inv("Set date+time first"));
    u.tick(2001);
    EXPECT_FALSE(u.drew("Set date+time first"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, UnlockNeedsDateAndTime)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.dateSet = true;
    u.m.timeSet = false;
    u.tick();
    u.click();
    u.hold();
    u.actions();
    u.click(1000);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::BeepError));
    u.m.dateSet = false;
    u.m.timeSet = true;
    u.click(1000);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::BeepError));
    u.m.dateSet = true;
    u.tick(10);
    u.click(1000);
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::TrackOn));
    EXPECT_FALSE(Ui::has(a, Kind::BeepError));
}

TEST(UiControllerTest, FaultPageOverridesEverything)
{
    Ui u;
    u.tick();
    u.click();
    u.turn(4);
    u.actions();
    u.m.faultCode = 1;
    u.tick(10);
    EXPECT_TRUE(u.inv("FAULT"));
    EXPECT_TRUE(u.drew("RA DRIVER FAULT"));
    EXPECT_TRUE(u.drew("Motors stopped"));
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::Stop));
    EXPECT_TRUE(Ui::has(a, Kind::BeepLong));
    u.turn(1);
    u.hold();
    EXPECT_TRUE(u.drew("Motors stopped"));
    u.click(1000);  // ok
    EXPECT_TRUE(u.inv("RA DRIVER FAULT"));  // stays as the status alert
    EXPECT_TRUE(u.drew("!"));
    EXPECT_EQ(0, u.c.violations);

    // Also over the saver.
    Ui v;
    v.tick();
    v.tick(120000);
    EXPECT_TRUE(v.drew("turn to wake"));
    v.m.faultCode = 2;
    v.tick(10);
    EXPECT_TRUE(v.drew("DEC DRIVER FAULT"));
    EXPECT_FALSE(v.drew("turn to wake"));
}

TEST(UiControllerTest, RebootedLocksAgain)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.rebooted = true;
    u.m.dateSet = false;
    u.m.timeSet = false;
    u.tick();
    EXPECT_TRUE(u.drew("WARNING"));
    EXPECT_TRUE(u.drew("REBOOTED - SET TIME"));
    EXPECT_TRUE(u.drew("click: set time"));
    u.click();
    EXPECT_TRUE(u.drew("SETUP DATE"));
    u.hold();  // cancel -> menu
    u.actions();
    u.click(1000);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::BeepError));
    u.hold();
    EXPECT_TRUE(u.inv("REBOOTED - SET TIME"));  // status alert
    // Date and time set again: unlocked although the reset flag stays.
    u.m.dateSet = true;
    u.m.timeSet = true;
    u.tick(10);
    EXPECT_FALSE(u.drew("REBOOTED - SET TIME"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, SaverAfter120sIdleOnly)
{
    Ui u;
    u.tick();
    u.tick(119999);
    EXPECT_FALSE(u.drew("turn to wake"));
    u.tick(1);
    EXPECT_TRUE(u.drew("turn to wake"));
    EXPECT_TRUE(u.drew("23:41"));
    EXPECT_TRUE(u.drew("TRACKING"));
    EXPECT_EQ(0, u.c.violations);

    Ui p;
    p.m.state = MountState::Parked;
    p.tick();
    p.tick(200000);
    EXPECT_FALSE(p.drew("turn to wake"));

    Ui w;  // warning replaces the RA/Dec line
    w.m.state = MountState::Idle;
    w.m.timeSet = false;
    w.tick();
    w.tick(120000);
    EXPECT_TRUE(w.drew("--:--"));
    EXPECT_TRUE(w.inv("SET TIME"));

    Ui i;  // input restarts the wait
    i.tick();
    i.turn(1, 100000);
    i.tick(100000);
    EXPECT_FALSE(i.drew("turn to wake"));
}

TEST(UiControllerTest, SaverNeverWhileMoving)
{
    Ui u;
    u.m.state = MountState::Slewing;
    u.tick();
    u.tick(200000);
    EXPECT_FALSE(u.drew("turn to wake"));

    Ui v;
    v.tick();
    v.tick(120000);
    EXPECT_TRUE(v.drew("turn to wake"));
    v.state(MountState::Slewing);  // LX200 GoTo under the saver: leave at once
    EXPECT_FALSE(v.drew("turn to wake"));
    EXPECT_TRUE(v.inv("click: STOP"));
    v.click();
    EXPECT_TRUE(Ui::has(v.actions(), Kind::Stop));
}

TEST(UiControllerTest, FirstInputOnlyWakes)
{
    Ui u;
    u.tick();
    u.tick(120000);
    u.click();
    EXPECT_FALSE(u.drew("turn to wake"));
    EXPECT_FALSE(u.drew("MENU"));
    EXPECT_TRUE(u.actions().empty());
    u.click();
    EXPECT_TRUE(u.drew("MENU"));

    Ui v;
    v.tick();
    v.click();  // menu
    v.tick(120000);
    v.turn(2);
    EXPECT_TRUE(v.inv("Stop tracking"));  // the turn only woke the screen
}

TEST(UiControllerTest, ArrivedHeaderThenTracking)
{
    Ui u;
    u.m.state = MountState::Slewing;
    u.tick();
    u.state(MountState::Tracking, 5000);
    EXPECT_TRUE(u.drew("ARRIVED"));
    u.tick(2999);
    EXPECT_TRUE(u.drew("ARRIVED"));
    u.tick(2);
    EXPECT_FALSE(u.drew("ARRIVED"));
    EXPECT_TRUE(u.drew("TRACKING"));
}

TEST(UiControllerTest, RedrawOnlyOnChange)
{
    Ui u;
    EXPECT_TRUE(u.tick());
    EXPECT_FALSE(u.tick(100));
    u.ui.onTurn(1, u.t);  // nothing to turn on the status screen
    EXPECT_FALSE(u.tick(100));
    u.m.localSeconds += 1;
    EXPECT_TRUE(u.tick(100));
    EXPECT_FALSE(u.tick(100));
}

TEST(UiControllerTest, ActionQueueDropsOldest)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.timeSet = false;
    u.tick();
    u.click();
    u.hold();
    u.click(1000);  // locked: BeepError
    u.state(MountState::Slewing, 10);
    for (int i = 0; i < 4; ++i)
        u.click(10);  // 4 x (Stop, BeepLong)
    auto a = u.actions();
    ASSERT_EQ(8u, a.size());
    EXPECT_FALSE(Ui::has(a, Kind::BeepError));
    EXPECT_EQ(Kind::Stop, a[0]);
    EXPECT_EQ(Kind::BeepLong, a[7]);
}

TEST(UiControllerTest, BelowMinAltAlertWhileTracking)
{
    Ui u;
    u.m.decArcSec = -60 * 3600;  // far south from latitude 41 N: below the horizon
    u.tick();
    EXPECT_TRUE(u.inv("BELOW MIN ALT"));
    EXPECT_TRUE(u.drew("!"));
}

TEST(UiControllerTest, L2ScreensFitWidth)
{
    // Width lint across L2 screens (EN); L4 adds the language switch and the TR pass.
    Ui u;
    u.m.state = MountState::Idle;
    u.m.dateSet = false;
    u.m.timeSet = false;
    u.m.rebooted = true;
    u.tick();
    u.click();
    u.hold();
    for (int i = 0; i < 9; ++i)
        u.turn(1);
    u.click(1000);
    u.hold();
    u.m.faultCode = 3;
    u.tick(10);
    EXPECT_TRUE(u.drew("DRIVER FAULT"));  // another driver (AZ, focus ...)
    u.m.faultCode = 4;
    u.tick(10);
    EXPECT_TRUE(u.drew("MOUNT FAULT"));
    EXPECT_EQ(0, u.c.violations);
}

TEST(UiControllerTest, InputBeforeTickSeesNewSnapshot)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.tick();
    u.click();  // menu, row 0 = Track here
    u.actions();
    u.t += 10;
    u.ui.onClick(u.t);  // dispatched before the tick that reports an LX200 GoTo
    u.state(MountState::Slewing);
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::Stop));
    EXPECT_FALSE(Ui::has(a, Kind::TrackOn));

    Ui v;  // input before the first tick is ignored
    v.ui.onClick(0);
    v.tick();
    EXPECT_TRUE(v.drew("click: menu"));
    EXPECT_TRUE(v.actions().empty());
}

TEST(UiControllerTest, MillisWraparound)
{
    Ui u;
    u.t = 0xFFFFF000u;
    u.tick();
    u.click();  // menu, Stop tracking
    u.t = 0xFFFFFF00u;
    u.state(MountState::Idle, 0);  // slot change 256 ms before the wrap
    u.actions();
    u.click(200);  // past the wrap, inside the window
    EXPECT_TRUE(u.actions().empty());
    u.click(301);
    EXPECT_TRUE(Ui::has(u.actions(), Kind::TrackOn));

    Ui v;
    v.t = 0xFFFFFC00u;
    v.m.state = MountState::Slewing;
    v.tick();
    v.click();
    v.state(MountState::Idle, 10);
    v.tick(1900);
    EXPECT_TRUE(v.drew("STOPPED"));
    v.tick(200);
    EXPECT_FALSE(v.drew("STOPPED"));
}

TEST(UiControllerTest, TwentyFiveDayIdleRun)
{
    Ui u;
    u.m.state = MountState::Slewing;
    u.m.hasTarget = false;
    u.tick();
    u.click();  // STOP: STOPPED latch
    u.state(MountState::Idle, 1000);  // row 0 changes: ignore latch
    u.click(1000);  // menu
    u.m.timeSet = false;
    u.tick(10);
    u.turn(1);
    u.click(10);  // Point locked: message latch
    EXPECT_TRUE(u.inv("Set date+time first"));
    for (int i = 0; i < 25 * 24 * 60; ++i)
        u.tick(60000);
    EXPECT_TRUE(u.drew("turn to wake"));
    EXPECT_FALSE(u.drew("STOPPED"));
    u.click();  // wake
    EXPECT_FALSE(u.drew("Set date+time first"));
    u.hold();  // menu -> status
    EXPECT_FALSE(u.drew("STOPPED"));
    u.m.timeSet = true;
    u.tick(10);
    u.actions();
    u.click();  // menu
    u.click();  // Track here, not swallowed by a stale window
    EXPECT_TRUE(Ui::has(u.actions(), Kind::TrackOn));
}

TEST(UiControllerTest, QueueNeverDropsStop)
{
    Ui u;
    u.m.state = MountState::Slewing;
    u.tick();
    u.click();  // Stop, BeepLong
    u.state(MountState::Idle, 1000);
    u.tick(600);  // past the slot-change window
    for (int i = 0; i < 10; ++i)
    {
        u.click(10);  // menu: Beep
        u.hold(10);   // status
    }
    auto a = u.actions();
    ASSERT_EQ(8u, a.size());
    EXPECT_EQ(Kind::Stop, a[0]);
    EXPECT_EQ(Kind::Beep, a[7]);
}

TEST(UiControllerTest, FaultLocksMotionRows)
{
    Ui u;
    u.m.state = MountState::Idle;
    u.m.faultCode = 1;
    u.tick();
    u.click();  // ok
    u.click();  // menu
    u.actions();
    u.click();  // Track here
    auto a = u.actions();
    EXPECT_TRUE(Ui::has(a, Kind::BeepError));
    EXPECT_FALSE(Ui::has(a, Kind::TrackOn));
    EXPECT_TRUE(u.inv("RA DRIVER FAULT"));
    for (int i = 1; i <= 2; ++i)  // Point, Adjust
    {
        u.turn(1, 2100);
        u.click();
        EXPECT_TRUE(Ui::has(u.actions(), Kind::BeepError)) << i;
    }
    u.turn(1, 2100);  // Settings stays live
    u.click();
    EXPECT_FALSE(Ui::has(u.actions(), Kind::BeepError));
    EXPECT_TRUE(u.drew("SETTINGS"));

    Ui v;  // stopping tracking stays live
    v.m.faultCode = 2;
    v.tick();
    v.click();
    v.click();
    v.actions();
    v.click();
    EXPECT_TRUE(Ui::has(v.actions(), Kind::TrackOff));
}

TEST(UiControllerTest, BelowMinAltHysteresis)
{
    Ui u;
    u.m.latArcSec = 0;
    u.m.decArcSec = 0;
    u.m.lstSeconds = 12 * 3600;
    u.m.raSeconds = 12 * 3600 - (6 * 3600 + 4 * 60);  // HA 6h04m: alt -1
    u.tick();
    EXPECT_TRUE(u.inv("BELOW MIN ALT"));
    EXPECT_EQ(3u, u.actions().size());
    u.m.raSeconds = 6 * 3600;  // alt 0: still below min + 1
    u.tick(10);
    EXPECT_TRUE(u.inv("BELOW MIN ALT"));
    u.m.raSeconds = 6 * 3600 + 4 * 60;  // alt +1: cleared
    u.tick(10);
    EXPECT_FALSE(u.drew("BELOW MIN ALT"));
    u.m.raSeconds = 6 * 3600;  // alt 0 is not below min
    u.tick(10);
    EXPECT_FALSE(u.drew("BELOW MIN ALT"));
    EXPECT_TRUE(u.actions().empty());
}

TEST(UiControllerTest, LongPressStopsOnFaultPageAndSaverWhileMoving)
{
    Ui u;
    u.m.state = MountState::Slewing;
    u.m.faultCode = 1;
    u.tick();
    u.actions();
    EXPECT_TRUE(u.drew("Motors stopped"));
    u.hold();
    EXPECT_TRUE(Ui::has(u.actions(), Kind::Stop));

    Ui v;
    v.tick();
    v.tick(120000);
    EXPECT_TRUE(v.drew("turn to wake"));
    v.t += 10;
    v.ui.onLongPress(v.t);  // pressed as an LX200 GoTo starts under the saver
    v.state(MountState::Slewing, 0);
    EXPECT_TRUE(Ui::has(v.actions(), Kind::Stop));
}

TEST(UiControllerTest, SnapshotDefaultsHaveNoAzFocus)
{
    const core::MountSnapshot s{};
    EXPECT_FALSE(s.hasAz);
    EXPECT_FALSE(s.hasFocus);
    for (int i = 0; i < 4; i++)
        EXPECT_EQ(s.axisPos[i], 0);
    EXPECT_GT(static_cast<int>(core::UiAction::Kind::MoveSteps), static_cast<int>(core::UiAction::Kind::SetSupplyVoltage));
}
