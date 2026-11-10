#pragma once

#include <vector>
#include "FakeCanvas.hpp"
#include "core/ui/UiController.hpp"

// Shared driver for the UiController tests (header only, one copy per test file).
namespace {

using core::MountSnapshot;
using core::MountState;
using core::UiAction;
using core::UiController;
using Kind = UiAction::Kind;

MountSnapshot base()
{
    MountSnapshot m{};
    m.raSeconds = 5 * 3600 + 35 * 60 + 17;
    m.decArcSec = -(5 * 3600 + 23 * 60 + 28);
    m.targetRaSeconds = m.raSeconds;
    m.targetDecArcSec = m.decArcSec;
    m.hasTarget = true;
    m.lstSeconds = 4 * 3600 + 58 * 60 + 2;
    m.localSeconds = 23 * 3600 + 41 * 60 + 7;
    m.year = 2026;
    m.month = 11;
    m.day = 10;
    m.utcOffsetHours = 3;
    m.latArcSec = 41 * 3600 + 60;
    m.lonArcSec = 28 * 3600 + 58 * 60;
    m.state = MountState::Tracking;
    m.slewRate = 2;
    m.dateSet = true;
    m.timeSet = true;
    m.supplyVoltage = 24;
    return m;
}

// Drives the controller with a moving clock; every input is followed by a tick.
struct Ui
{
    FakeCanvas c;
    UiController ui{c};
    MountSnapshot m = base();
    uint32_t t = 0;

    bool tick(uint32_t dt = 0)
    {
        t += dt;
        return ui.tick(t, m);
    }
    void click(uint32_t dt = 10)
    {
        t += dt;
        ui.onClick(t);
        tick();
    }
    void hold(uint32_t dt = 10)
    {
        t += dt;
        ui.onLongPress(t);
        tick();
    }
    void turn(int8_t d, uint32_t dt = 10)
    {
        t += dt;
        ui.onTurn(d, t);
        tick();
    }
    void state(MountState s, uint32_t dt = 10)
    {
        m.state = s;
        tick(dt);
    }
    bool drew(const char *s) const { return c.has(s); }
    bool inv(const char *s) const
    {
        for (const auto &x : c.texts)
            if (x.s == s && x.inverted)
                return true;
        return false;
    }
    std::vector<Kind> actions()
    {
        std::vector<Kind> v;
        for (const UiAction &a : acts())
            v.push_back(a.kind);
        return v;
    }
    std::vector<UiAction> acts()
    {
        std::vector<UiAction> v;
        UiAction a;
        while (ui.popAction(a))
            v.push_back(a);
        return v;
    }
    // First action of kind k; false when there is none.
    static bool find(const std::vector<UiAction> &v, Kind k, UiAction &out)
    {
        for (const UiAction &a : v)
            if (a.kind == k)
            {
                out = a;
                return true;
            }
        return false;
    }
    static bool has(const std::vector<Kind> &v, Kind k)
    {
        for (Kind x : v)
            if (x == k)
                return true;
        return false;
    }
};

}  // namespace
