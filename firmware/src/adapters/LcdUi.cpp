#ifdef ARDUINO
    #include "../../Configuration.hpp"
#endif

#if USE_LCD_UI == 1

    #include <string.h>
    #include "LcdUi.hpp"
    #include "Knob.hpp"
    #include "U8g2Canvas.hpp"
    #include "UiMountAdapter.hpp"
    #include "EPROMStore.hpp"

extern Mount mount;

namespace
{
constexpr uint32_t kIdleFrameGapMs = 250;
constexpr uint32_t kSlewFrameGapMs = 500;
constexpr uint32_t kInputFrameGapMs = 30;  // knob input: redraw as soon as the frame is ready

U8g2Canvas canvas;
core::UiController ui(canvas);
Knob knob;
UiMountAdapter adapter(mount);
bool framePending    = false;
bool inputPending    = false;
uint32_t lastFrameMs = 0;
core::SettingsStoreLatch settingsLatch;  // the block last stored (or seen at boot)
core::NetStatus net{};
static_assert(core::UiSettings::kStoreBytes <= EEPROMStore::UI_SETTINGS_BYTES, "LCD Settings block outgrew its EEPROM slot");
}  // namespace

namespace
{
// Loads the stored Settings block into the UI; a failed read means defaults. Returns the supply volts to apply, 0 = none.
int loadStoredSettings()
{
    uint8_t block[core::UiSettings::kStoreBytes];
    if (!EEPROMStore::getUiSettings(block, sizeof block))
        memset(block, 0, sizeof block);  // no marker: all defaults
    return ui.loadSettings(block);
}
}  // namespace

void lcdUiSetup()
{
    canvas.begin();
    knob.begin();
    adapter.begin();
    const int volts = loadStoredSettings();
    if (volts != 0)
        mount.applySupplyVoltage(static_cast<uint8_t>(volts));  // the stored voltage wins over SUPPLY_VOLTAGE
}

void lcdUiReloadSettings()
{
    loadStoredSettings();       // after :XFR the store is blank: defaults apply
    settingsLatch.reset();      // the next pass records the block (and the mirrored supply) without writing
}

void lcdUiUpdate()
{
    const uint32_t now = millis();
    knob.update(now, ui);
    inputPending |= knob.takeInput();

    core::MountSnapshot snap{};
    adapter.fill(snap);
    core::expireNet(net, now);
    snap.net = net;
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

    // Settings are stored on change only: the first pass just records them, so a boot never writes.
    uint8_t block[core::UiSettings::kStoreBytes];
    ui.settings().save(block);
    if (settingsLatch.observe(block))
        EEPROMStore::storeUiSettings(block, sizeof block);

    const bool slewing = snap.state == core::MountState::Slewing || snap.state == core::MountState::Parking
                         || snap.state == core::MountState::Homing;
    if (framePending && now - lastFrameMs >= (inputPending ? kInputFrameGapMs : (slewing ? kSlewFrameGapMs : kIdleFrameGapMs)))
    {
        canvas.flush();
        framePending = false;
        inputPending = false;
        lastFrameMs  = millis();
    }
    if (!framePending)
        inputPending = false;  // input that changed nothing must not speed up a later status redraw
}

core::NetStatus &lcdUiNet()
{
    return net;
}

uint8_t lcdUiOwnWifi()
{
    return static_cast<uint8_t>(ui.settings().get(core::SettingId::OwnWifi));
}

#endif
