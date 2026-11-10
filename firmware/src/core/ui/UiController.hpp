#pragma once

#include <cstddef>
#include <cstdint>
#include "core/ui/Canvas.hpp"
#include "core/ui/FieldEditor.hpp"
#include "core/ui/SkyMath.hpp"
#include "core/ui/UiSettings.hpp"
#include "core/ui/UiText.hpp"

namespace core {

enum class MountState : uint8_t
{
    Idle,
    Tracking,
    Slewing,
    Jogging,
    Parking,
    Homing,
    Parked,
    Fault
};

// What the UI needs from the mount, filled by the adapter before every tick().
// dateSet/timeSet: set since this boot. rebooted: the last reset was unplanned (only selects the
// warning page and text; the date+time lock comes from dateSet/timeSet).
// faultCode: 0 none, 1 RA driver, 2 DEC driver, 3 another driver, other = mount fault.
struct MountSnapshot
{
    int32_t raSeconds, decArcSec;  // current, celestial
    int32_t targetRaSeconds, targetDecArcSec;
    bool hasTarget;
    int32_t lstSeconds, localSeconds;  // 0..86399
    int16_t year;
    int8_t month, day, utcOffsetHours;
    int32_t latArcSec, lonArcSec;
    MountState state;
    uint8_t slewRate;  // 1..4
    bool dateSet, timeSet, rebooted, synced;
    uint8_t faultCode;  // 0 = none
    uint8_t supplyVoltage;  // 12 or 24 (0 = unknown, shown as the default)
    // Adjust page. Axis order RA, DEC, AZ, Focus. axisPos: current step position (slew-mode steps).
    // stepsPerDeg: RA slew, DEC slew, AZ (0 = unknown).
    bool hasAz, hasFocus;  // that motor is configured
    bool azFault, focusFault;  // its driver failed the UART test: never move it
    int32_t axisPos[4];
    float stepsPerDeg[3];
};

struct UiAction
{
    enum class Kind : uint8_t
    {
        Stop,
        TrackOn,
        TrackOff,
        JogStart,
        JogStop,
        SetSlewRate,
        Goto,
        Sync,
        SetDate,
        SetTime,
        SetUtcOffset,
        SetLatitude,
        SetLongitude,
        GoHome,
        SetHome,
        Park,
        Unpark,
        Beep,
        BeepError,
        BeepLong,
        SetSupplyVoltage,  // a = volts (12 or 24)
        MoveSteps  // a = signed steps, b = axis (0 RA, 1 DEC, 2 AZ, 3 Focus)
    };
    Kind kind;
    // JogStart/JogStop: direction bits N1 E2 S4 W8; Goto/Sync: raSeconds; SetDate: yyyymmdd; SetTime: seconds of day;
    // SetLatitude/SetLongitude: arc-seconds; SetUtcOffset: hours; SetSlewRate: 1..4; SetSupplyVoltage: volts;
    // MoveSteps: steps.
    int32_t a;
    int32_t b;  // Goto/Sync: decArcSec; MoveSteps: axis
};

// The 128x64 LCD user interface: screens, knob input, safety rules, actions for the mount.
// Adding a screen: a value in Screen, a case in renderScreen/clickScreen/holdScreen/turnScreen.
// Safety (STOP while moving, fault page, saver, 500 ms click ignore) sits in front of those
// switches, so every screen inherits it. The flow screens live in UiScreens.cpp.
class UiController
{
  public:
    explicit UiController(Canvas &canvas);
    // detents are raw; fast = the knob was turning quickly (menus and editors apply kFastFactor, Adjust never does).
    void onTurn(int8_t detents, uint32_t nowMs, bool fast = false);
    static constexpr int8_t kFastFactor = 5;
    void onClick(uint32_t nowMs);
    void onLongPress(uint32_t nowMs);
    bool tick(uint32_t nowMs, const MountSnapshot &m);  // returns true when the canvas changed
    bool popAction(UiAction &out);  // FIFO, capacity 8, drops oldest
    void setLastSavedDate(int32_t yyyymmdd);  // prefill (S1b)

  private:
    enum class Screen : uint8_t
    {
        Status,
        Menu,
        Rebooted,
        Editor,  // big-digit editor; _edit says which value
        PointConfirm,  // GO or SYNC with the entered coordinates, the knob picks
        Refused,
        Setup,  // under Settings
        ClearsSync,
        Settings,
        Adjust,  // step moves per detent on the selected axis
        AdjustOptions,  // step size, sync on target, set home, go home
        SetHomeConfirm
    };
    enum class Edit : uint8_t
    {
        PointRa,
        PointDec,
        Date,
        Time,
        Utc,
        Lat,
        Lon,
        MinAlt
    };
    // A list screen row: text (label + right-aligned value) and whether it is locked.
    struct Item
    {
        char s[32];
        bool lock;
    };
    static constexpr uint8_t kItems = 9;
    // Main menu rows, fixed indices (row 0 = motion slot).
    enum Row : uint8_t
    {
        RowMotion,
        RowPoint,
        RowAdjust,
        RowSettings,
        RowCount
    };

    // input
    struct Input
    {
        enum Kind : uint8_t
        {
            Click,
            Hold,
            Turn
        } kind;
        int8_t detents;
        int16_t extra;  // accelerated surplus on top of detents
        uint32_t ms;
    };
    static constexpr uint8_t kInputs = 8;
    void queueInput(Input::Kind k, int8_t detents, uint32_t nowMs, int16_t extra = 0);
    void applyClick(uint32_t nowMs);
    void applyHold(uint32_t nowMs);
    void applyTurn(int8_t detents, int16_t extra, uint32_t nowMs);
    bool mountMoving() const;  // from the snapshot
    bool moving() const { return mountMoving() || _jogDir != 0 || _stepLatch; }  // an own jog or step move not reported yet counts
    void stop(uint32_t nowMs);
    void clickScreen(uint32_t nowMs);
    void holdScreen();
    void turnScreen(int8_t detents, uint32_t nowMs);
    void clickMenu(uint32_t nowMs);
    void openMenu();
    void openDateEditor();
    void clickTick();  // click feedback, follows the Beep setting

    // L3/L4 screens (UiScreens.cpp)
    void openEditor(Edit e, int32_t value, bool lastGroup = false);
    void editorDone();
    void editorBack();
    void pend(UiAction::Kind k, int32_t a)
    {
        if (_pendingN < 2)
            _pending[_pendingN++] = {k, a, 0};
    }
    void commit(Screen ret);  // applies the pending setup actions; S5 "Clears Sync. Sure?" first after a Sync
    void evalGoto();  // _check, _alt, _az, _sun from the current snapshot
    void openPointConfirm();  // after the Dec editor: GO selected, armed 600 ms later
    void clickList(uint32_t nowMs);
    uint8_t items(Item *out) const;  // rows of the current list screen
    void clickSetup(uint8_t row);
    void clickSettings(uint8_t row);
    void clickAdjustOptions(uint8_t row);
    void turnAdjust(int8_t detents, uint32_t nowMs);
    TextId optionLock(uint8_t row) const;  // Adjust options: Count = unlocked, else message id
    bool axisFaulted() const { return (_axis == 2 && _m.azFault) || (_axis == 3 && _m.focusFault); }
    void jogStop();
    TextId speedText() const;
    int supplyVolts() const { return _m.supplyVoltage ? _m.supplyVoltage : (_settings.get(SettingId::Supply) ? 24 : 12); }
    char letter(bool positive, bool lat) const;
    void formatEdit(char *out, size_t n, FieldKind k, int32_t v) const;
    // state helpers
    void push(UiAction::Kind k, int32_t a = 0, int32_t b = 0);
    void showTransient(TextId id, uint32_t ms, uint32_t nowMs);  // SYNCED / HOME SET
    void showMessage(TextId id, uint32_t ms, uint32_t nowMs);
    bool messageActive() const { return _message != TextId::Count && within(_now, _messageFrom, _messageMs); }
    // Signed difference: an input stamped just before the tick that set `from` still counts as within.
    static bool within(uint32_t now, uint32_t from, uint32_t ms) { return static_cast<int32_t>(now - from) < static_cast<int32_t>(ms); }
    bool dateTimeSet() const { return _m.dateSet && _m.timeSet; }
    bool faultActive() const { return _m.faultCode != 0 || _m.state == MountState::Fault; }
    TextId warning() const;  // TextId::Count = none
    TextId row0Label() const;
    TextId rowLabel(uint8_t row) const;
    TextId rowLock(uint8_t row) const;  // Count = unlocked, else message id
    TextId stateWord() const;
    TextId faultText() const;
    void altAz(int &alt, int &az) const;

    // rendering
    void render(Canvas &c) const;
    void renderScreen(Canvas &c) const;
    void renderStatus(Canvas &c) const;
    void renderMenu(Canvas &c) const;
    void list(Canvas &c, TextId title, const Item *it, uint8_t n, uint8_t sel) const;
    void renderRebooted(Canvas &c) const;
    void renderFault(Canvas &c) const;
    void renderSaver(Canvas &c) const;
    void renderEditor(Canvas &c, TextId title, TextId sub, TextId foot) const;
    void renderEdit(Canvas &c) const;
    void renderList(Canvas &c) const;
    void renderConfirm(Canvas &c) const;
    void renderRefused(Canvas &c) const;
    void renderAdjust(Canvas &c) const;
    void stepText(char *out, size_t n) const;  // "1000" or "JOG SLOW"
    void renderSmallConfirm(Canvas &c, TextId hdr, TextId msg) const;
    void header(Canvas &c, const char *left, const char *right, bool inverted = false) const;
    void footer(Canvas &c, const char *hint) const;
    void bar(Canvas &c, int16_t y0, int16_t y1, int16_t ty, const char *s) const;
    const char *t(TextId id) const { return uiText(id, _lang); }

    Canvas &_canvas;
    Lang _lang = Lang::EN;
    MountSnapshot _m{};
    bool _started = false;
    uint32_t _now = 0;

    Screen _screen = Screen::Status;
    uint8_t _menuSel = 0;
    uint8_t _sel = 0;  // cursor on list screens other than the main menu
    FieldEditor _editor;
    Edit _edit = Edit::Date;
    bool _fastPath = false;  // status alert: Date then Time, applied together
    int32_t _ra = 0, _dec = 0;  // Point values being edited or confirmed
    bool _pointSync = false;  // PointConfirm: false = GO, true = SYNC
    bool _haveTarget = false;  // last GoTo target latched (the snapshot only reports it while slewing)
    int32_t _tgtRa = 0, _tgtDec = 0;
    int32_t _pendingDate = 0;
    uint32_t _screenFrom = 0;  // tick that opened the confirm screen (GO armed 600 ms later)
    TargetCheck _check = TargetCheck::Ok;
    int _alt = 0, _az = 0, _sun = 0;
    bool _bigJump = false;
    int32_t _dRa = 0, _dDec = 0;
    UiAction _pending[2];
    uint8_t _pendingN = 0;
    Screen _ret = Screen::Status;  // after ClearsSync
    uint8_t _axis = 0;  // Adjust: 0 RA, 1 DEC, 2 AZ, 3 Focus
    uint8_t _stepSel = 2;  // Adjust: 0..4 = 1, 10, 100, 1000, 10000 steps; 5..8 = JOG SLOW..MAX (RA/DEC only)
    uint8_t _jogDir = 0;  // own jog in progress (0 = none)
    uint32_t _jogMs = 0;
    bool _stepLatch = false;  // own MoveSteps emitted, snapshot not moving yet (cleared on motion or after kStepLatchMs)
    uint32_t _stepMs = 0;
    UiSettings _settings;
    int32_t _savedDate;

    TextId _row0 = TextId::TrackHere;
    bool _ignoreArmed = false;
    uint32_t _ignoreFrom = 0;
    TextId _transient = TextId::Count;
    uint32_t _transientFrom = 0, _transientMs = 0;
    TextId _message = TextId::Count;
    uint32_t _messageFrom = 0, _messageMs = 0;

    bool _faultPage = false, _faultSeen = false, _rebootSeen = false, _low = false;
    bool _saver = false;
    uint32_t _lastInputMs = 0;
    MountState _prevState = MountState::Idle;

    UiAction _queue[8];
    uint8_t _qCount = 0;
    Input _inputs[kInputs];
    uint8_t _inCount = 0;

    bool _drawn = false;
    uint32_t _hash = 0;
};

}  // namespace core
