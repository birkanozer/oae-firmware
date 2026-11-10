#ifdef ARDUINO
    #include "../../Configuration.hpp"
#endif

#if USE_LCD_UI == 1

    #include "LcdUi.hpp"
    #include "Knob.hpp"
    #include "U8g2Canvas.hpp"
    #include "UiMountAdapter.hpp"

extern Mount mount;

namespace
{
constexpr uint32_t kIdleFrameGapMs = 250;
constexpr uint32_t kSlewFrameGapMs = 500;

U8g2Canvas canvas;
core::UiController ui(canvas);
Knob knob;
UiMountAdapter adapter(mount);
bool framePending    = false;
uint32_t lastFrameMs = 0;
}  // namespace

void lcdUiSetup()
{
    canvas.begin();
    knob.begin();
    adapter.begin();
}

void lcdUiUpdate()
{
    const uint32_t now = millis();
    knob.update(now, ui);

    core::MountSnapshot snap{};
    adapter.fill(snap);
    framePending |= ui.tick(now, snap);

    core::UiAction a;
    while (ui.popAction(a))
    {
        using Kind = core::UiAction::Kind;
        if (a.kind == Kind::Beep || a.kind == Kind::BeepError || a.kind == Kind::BeepLong)  // not a range: later kinds are mount actions
            knob.beep(a.kind);
        else
            adapter.apply(a);
    }

    const bool slewing = snap.state == core::MountState::Slewing || snap.state == core::MountState::Parking
                         || snap.state == core::MountState::Homing;
    if (framePending && now - lastFrameMs >= (slewing ? kSlewFrameGapMs : kIdleFrameGapMs))
    {
        canvas.flush();
        framePending = false;
        lastFrameMs  = millis();
    }
}

#endif
