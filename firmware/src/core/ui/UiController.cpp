#include "core/ui/UiController.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "core/ui/SkyMath.hpp"
#include "core/ui/UiDraw.hpp"

namespace core {

using namespace draw;

namespace {

constexpr uint32_t kIgnoreMs = 500;  // clicks ignored after row 0's label changes (M2)
constexpr uint32_t kStoppedMs = 2000;
constexpr uint32_t kArrivedMs = 3000;
constexpr uint32_t kSaverMs = 120000;
constexpr uint8_t kQueue = 8;
constexpr uint32_t kSyncedMs = 2000;  // SYNCED / HOME SET

int32_t buildDate()
{
    const char *d = __DATE__;  // "Nov 10 2026"
    static const char kMon[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int mon = 1;
    for (int i = 0; i < 12; ++i)
        if (strncmp(d, kMon + 3 * i, 3) == 0)
            mon = i + 1;
    return atoi(d + 7) * 10000 + mon * 100 + atoi(d + 4);
}

// FNV-1a over every drawing op, so tick() can tell whether the frame changed without keeping one.
struct HashCanvas : Canvas
{
    uint32_t h = 2166136261u;
    void mix(uint32_t v)
    {
        for (int i = 0; i < 4; ++i)
        {
            h ^= (v >> (8 * i)) & 0xFF;
            h *= 16777619u;
        }
    }
    void rect(uint32_t op, int16_t a, int16_t b, int16_t c, int16_t d)
    {
        mix(op);
        mix((uint16_t)a);
        mix((uint16_t)b);
        mix((uint16_t)c);
        mix((uint16_t)d);
    }
    void clear() override { mix(1); }
    void text(Font f, int16_t x, int16_t y, const char *s, bool inverted) override
    {
        rect(2, (int16_t)f, x, y, inverted);
        for (; *s; ++s)
            mix((uint8_t)*s);
    }
    void box(int16_t x0, int16_t y0, int16_t x1, int16_t y1) override { rect(3, x0, y0, x1, y1); }
    void frame(int16_t x0, int16_t y0, int16_t x1, int16_t y1) override { rect(4, x0, y0, x1, y1); }
    void dots(int16_t x0, int16_t x1, int16_t y) override { rect(5, x0, x1, y, 0); }
};

}  // namespace

UiController::UiController(Canvas &canvas) : _canvas(canvas), _savedDate(buildDate())
{
}

void UiController::setLastSavedDate(int32_t yyyymmdd)
{
    _savedDate = yyyymmdd;
}

// ---- actions ----

void UiController::push(UiAction::Kind k, int32_t a, int32_t b)
{
    if (_qCount == kQueue)
    {
        // Full: evict the oldest non-Stop entry. A Stop is never dropped (with eight queued Stops a
        // new entry adds nothing).
        uint8_t i = 0;
        while (i < kQueue && _queue[i].kind == UiAction::Kind::Stop)
            ++i;
        if (i == kQueue)
            return;
        for (; i + 1 < kQueue; ++i)
            _queue[i] = _queue[i + 1];
        --_qCount;
    }
    _queue[_qCount++] = {k, a, b};
}

bool UiController::popAction(UiAction &out)
{
    if (_qCount == 0)
        return false;
    out = _queue[0];
    for (uint8_t i = 1; i < _qCount; ++i)
        _queue[i - 1] = _queue[i];
    --_qCount;
    return true;
}

void UiController::showTransient(TextId id, uint32_t ms, uint32_t nowMs)
{
    _transient = id;
    _transientFrom = nowMs;
    _transientMs = ms;
}

void UiController::showMessage(TextId id, uint32_t ms, uint32_t nowMs)
{
    _message = id;
    _messageFrom = nowMs;
    _messageMs = ms;
}

// ---- input: safety first, then the screen ----

bool UiController::mountMoving() const
{
    return _m.state == MountState::Slewing || _m.state == MountState::Jogging || _m.state == MountState::Parking ||
           _m.state == MountState::Homing;
}

void UiController::clickTick()
{
    if (_settings.get(SettingId::Beep))
        push(UiAction::Kind::Beep);
}

void UiController::stop(uint32_t nowMs)
{
    push(UiAction::Kind::Stop);  // stops all motors, an own jog included
    push(UiAction::Kind::BeepLong);
    showTransient(TextId::HdrStopped, kStoppedMs, nowMs);
    _jogDir = 0;
    _stepLatch = false;
    _saver = false;
    _screen = Screen::Status;
}

// Inputs are only queued; tick() applies them after it has seen the new snapshot, so a click
// that raced an LX200 GoTo still sees the motion (STOP) and the armed 500 ms window.
void UiController::queueInput(Input::Kind k, int8_t detents, uint32_t nowMs, int16_t extra)
{
    if (!_started)  // nothing on screen yet
        return;
    if (k == Input::Turn && _inCount > 0 && _inputs[_inCount - 1].kind == Input::Turn)
    {
        Input &last = _inputs[_inCount - 1];  // merge consecutive turns, keep the newest stamp
        const int sum = last.detents + detents;
        last.detents = static_cast<int8_t>(sum > 127 ? 127 : (sum < -127 ? -127 : sum));
        last.extra = static_cast<int16_t>(last.extra + extra);
        last.ms = nowMs;
        return;
    }
    if (_inCount == kInputs)
    {
        for (uint8_t i = 1; i < kInputs; ++i)  // drop the oldest
            _inputs[i - 1] = _inputs[i];
        --_inCount;
    }
    _inputs[_inCount++] = {k, detents, extra, nowMs};
}

void UiController::onClick(uint32_t nowMs)
{
    queueInput(Input::Click, 0, nowMs);
}

void UiController::onLongPress(uint32_t nowMs)
{
    queueInput(Input::Hold, 0, nowMs);
}

void UiController::onTurn(int8_t detents, uint32_t nowMs, bool fast)
{
    queueInput(Input::Turn, detents, nowMs, fast ? static_cast<int16_t>(detents * (kFastFactor - 1)) : 0);
}

void UiController::applyClick(uint32_t nowMs)
{
    _lastInputMs = nowMs;
    if (moving())  // M1: any click on any screen stops, the Jog screen too
        return stop(nowMs);
    if (_saver)  // M3: first input only wakes
    {
        _saver = false;
        return;
    }
    if (_faultPage)
    {
        _faultPage = false;
        _screen = Screen::Status;
        clickTick();
        return;
    }
    if (_ignoreArmed && _screen != Screen::Adjust && within(nowMs, _ignoreFrom, kIgnoreMs))  // M2 (not Adjust: the axis click follows a move)
        return;
    clickScreen(nowMs);
}

void UiController::applyHold(uint32_t nowMs)
{
    _lastInputMs = nowMs;
    if (moving())
        return stop(nowMs);
    if (_saver)
    {
        _saver = false;
        return;
    }
    if (_faultPage)
        return;
    holdScreen();
}

void UiController::applyTurn(int8_t detents, int16_t extra, uint32_t nowMs)
{
    _lastInputMs = nowMs;
    // Turning does nothing while moving, except on Adjust: it continues a jog or step move (not while homing or parking).
    if (moving() && !(_screen == Screen::Adjust && _m.state != MountState::Homing && _m.state != MountState::Parking))
        return;
    if (_saver)
    {
        _saver = false;
        return;
    }
    if (_faultPage)
        return;
    const int eff = _screen == Screen::Adjust ? detents : detents + extra;  // Adjust moves exactly detents * step
    turnScreen(static_cast<int8_t>(eff > 127 ? 127 : (eff < -127 ? -127 : eff)), nowMs);
}

void UiController::openMenu()
{
    _menuSel = 0;  // M2: the menu always opens at row 0
    _screen = Screen::Menu;
}

void UiController::openDateEditor()
{
    openEditor(Edit::Date, _m.dateSet ? _m.year * 10000 + _m.month * 100 + _m.day : _savedDate);
}

void UiController::clickScreen(uint32_t nowMs)
{
    switch (_screen)
    {
        case Screen::Status:
        {
            clickTick();
            const TextId w = warning();
            if (w == TextId::AlertDateTime || w == TextId::AlertDate || w == TextId::AlertTime || w == TextId::AlertRebooted)
            {
                openDateEditor();  // S1a: the alert goes straight to Date, then Time
                _fastPath = true;
            }
            else
                openMenu();
            break;
        }
        case Screen::Menu:
            clickMenu(nowMs);
            break;
        case Screen::Rebooted:
            clickTick();
            openDateEditor();
            _fastPath = true;
            break;
        case Screen::Editor:
            clickTick();
            if (_editor.click())
                editorDone();
            break;
        case Screen::PointConfirm:
            if (within(nowMs, _screenFrom, kArmMs))  // S4: armed 600 ms after the screen appeared (GO and SYNC alike)
                break;
            if (_pointSync)
            {
                clickTick();
                push(UiAction::Kind::Sync, _ra, _dec);
                showTransient(TextId::HdrSynced, kSyncedMs, nowMs);
            }
            else
            {
                evalGoto();  // the target moved since the confirm screen opened: check again with the current snapshot
                if (_check != TargetCheck::Ok)
                {
                    push(UiAction::Kind::BeepError);
                    _screen = Screen::Refused;
                    break;
                }
                clickTick();
                push(UiAction::Kind::Goto, _ra, _dec);
            }
            _haveTarget = true, _tgtRa = _ra, _tgtDec = _dec;  // the target for Adjust -> Sync on target
            _screen = Screen::Status;
            break;
        case Screen::Adjust:  // next axis; AZ and Focus only when configured
        {
            clickTick();
            uint8_t next = _axis;
            do
                next = static_cast<uint8_t>((next + 1) % 4);
            while ((next == 2 && !_m.hasAz) || (next == 3 && !_m.hasFocus));
            _axis = next;
            if (_axis >= 2 && _stepSel >= 5)  // no JOG on AZ and Focus
                _stepSel = 4;
            break;
        }
        case Screen::SetHomeConfirm:
            if (_m.state != MountState::Idle)  // the lock again at the click
            {
                push(UiAction::Kind::BeepError);
                _screen = Screen::AdjustOptions;
                break;
            }
            clickTick();
            push(UiAction::Kind::SetHome);
            showTransient(TextId::HdrHomeSet, kSyncedMs, nowMs);
            _screen = Screen::Status;
            break;
        case Screen::Refused:  // S9: back to the RA editor, values kept
            clickTick();
            openEditor(Edit::PointRa, _ra);
            break;
        case Screen::ClearsSync:  // yes
            clickTick();
            for (uint8_t i = 0; i < _pendingN; ++i)
                push(_pending[i].kind, _pending[i].a);
            _pendingN = 0;
            _screen = _ret;
            break;
        default:  // list screens
            clickList(nowMs);
            break;
    }
}

void UiController::clickMenu(uint32_t nowMs)
{
    const TextId lock = rowLock(_menuSel);
    if (lock != TextId::Count)
    {
        push(UiAction::Kind::BeepError);
        showMessage(lock, kMessageMs, nowMs);
        return;
    }
    clickTick();
    _sel = 0;
    switch (_menuSel)
    {
        case RowMotion:
            push(_m.state == MountState::Tracking ? UiAction::Kind::TrackOff : UiAction::Kind::TrackOn);
            _screen = Screen::Status;
            break;
        case RowPoint:  // S2: prefilled with the last target, else where the mount points
            _ra = _haveTarget ? _tgtRa : _m.raSeconds;
            _dec = _haveTarget ? _tgtDec : _m.decArcSec;
            openEditor(Edit::PointRa, _ra);
            break;
        case RowAdjust:  // keeps the last axis and step size
            _screen = Screen::Adjust;
            break;
        case RowSettings:
            _screen = Screen::Settings;
            break;
        default:
            _screen = Screen::Status;
            break;
    }
}

void UiController::holdScreen()
{
    switch (_screen)
    {
        case Screen::Status:
            break;
        case Screen::Menu:
        case Screen::Rebooted:
            _screen = Screen::Status;
            break;
        case Screen::Editor:
            editorBack();
            break;
        case Screen::ClearsSync:  // no: nothing applied
            _pendingN = 0;
            _screen = _ret;
            break;
        case Screen::Setup:  // long press is back, one level up
            _screen = Screen::Settings;
            _sel = 0;
            break;
        case Screen::Adjust:  // not moving here (moving is STOP): the options
            _sel = 0;
            _screen = Screen::AdjustOptions;
            break;
        case Screen::SetHomeConfirm:
            _screen = Screen::AdjustOptions;
            break;
        case Screen::AdjustOptions:
        case Screen::Settings:
        case Screen::PointConfirm:
        case Screen::Refused:
            openMenu();
            break;
    }
}

void UiController::turnScreen(int8_t detents, uint32_t nowMs)
{
    switch (_screen)
    {
        case Screen::Menu:
        {
            const int s = _menuSel + detents;
            _menuSel = static_cast<uint8_t>(s < 0 ? 0 : (s >= RowCount ? RowCount - 1 : s));
            break;
        }
        case Screen::Editor:
            _editor.turn(detents);
            break;
        case Screen::PointConfirm:  // GO <-> SYNC, clamped
            _pointSync = detents > 0 ? true : (detents < 0 ? false : _pointSync);
            break;
        case Screen::Adjust:
            turnAdjust(detents, nowMs);
            break;
        case Screen::Setup:
        case Screen::Settings:
        case Screen::AdjustOptions:
        {
            Item it[kItems];
            const int s = _sel + detents, n = items(it);
            _sel = static_cast<uint8_t>(s < 0 ? 0 : (s >= n ? n - 1 : s));
            break;
        }
        default:
            break;
    }
}

// ---- mount state ----

TextId UiController::row0Label() const
{
    if (mountMoving() || _jogDir != 0)  // not the step latch: it would flip the row and arm the M2 click ignore
        return TextId::Stop;
    return _m.state == MountState::Tracking ? TextId::StopTracking : TextId::TrackHere;
}

TextId UiController::rowLabel(uint8_t row) const
{
    static const TextId kRows[RowCount] = {TextId::TrackHere, TextId::Point, TextId::Adjust, TextId::Settings};
    return row == RowMotion ? row0Label() : kRows[row];
}

TextId UiController::rowLock(uint8_t row) const
{
    if (moving())  // S5: only STOP and Settings live; any click is STOP anyway
        return (row == RowMotion || row == RowSettings) ? TextId::Count : TextId::FootStop;
    if (faultActive() && row <= RowAdjust && !(row == RowMotion && row0Label() == TextId::StopTracking))
        return faultText();  // fault: motion rows locked, "Stop tracking" and Settings stay live
    const bool dt = dateTimeSet();  // M6: both date and time
    switch (row)
    {
        case RowMotion:
            if (row0Label() == TextId::StopTracking)
                return TextId::Count;  // stopping is never locked
            return dt ? TextId::Count : TextId::MsgDateTimeFirst;
        case RowPoint:  // GO and SYNC both need the clock
            return dt ? TextId::Count : TextId::MsgDateTimeFirst;
        default:
            return TextId::Count;
    }
}

TextId UiController::stateWord() const
{
    static const TextId kWords[] = {TextId::HdrIdle,    TextId::HdrTracking, TextId::HdrGoto,   TextId::HdrJogging,
                                    TextId::HdrParking, TextId::HdrHoming,   TextId::HdrParked, TextId::HdrFault};
    return kWords[static_cast<uint8_t>(_m.state)];
}

TextId UiController::faultText() const
{
    switch (_m.faultCode)
    {
        case 1:
            return TextId::FaultRa;
        case 2:
            return TextId::FaultDec;
        case 3:
            return TextId::FaultDriver;
        default:
            return TextId::FaultMount;
    }
}

void UiController::altAz(int &alt, int &az) const
{
    int32_t ha = _m.lstSeconds - _m.raSeconds;
    ha = (ha % 86400 + 86400) % 86400;
    const AltAz h = equatorialToHorizontal(ha / 3600.0f, _m.decArcSec / 3600.0f, _m.latArcSec / 3600.0f);
    alt = static_cast<int>(lroundf(h.altDeg));
    az = static_cast<int>(lroundf(h.azDeg)) % 360;
    if (az < 0)
        az += 360;
}

// One warning slot with a fixed priority: alert bar, header '!' and saver line all use it.
TextId UiController::warning() const
{
    if (faultActive())
        return faultText();
    if (!dateTimeSet())
    {
        if (_m.rebooted)
            return TextId::AlertRebooted;
        if (!_m.dateSet && !_m.timeSet)
            return TextId::AlertDateTime;
        return _m.dateSet ? TextId::AlertTime : TextId::AlertDate;
    }
    return _low ? TextId::AlertBelowMinAlt : TextId::Count;
}

bool UiController::tick(uint32_t nowMs, const MountSnapshot &m)
{
    _now = nowMs;
    const MountState prev = _started ? _m.state : m.state;
    const TextId prevRow0 = _row0;
    _m = m;
    if (m.hasTarget)  // the adapter reports the target only while slewing: keep the last one
        _haveTarget = true, _tgtRa = m.targetRaSeconds, _tgtDec = m.targetDecArcSec;
    _row0 = row0Label();
    if (!_started)
    {
        _started = true;
        _lastInputMs = nowMs;
    }
    else if (_row0 != prevRow0)  // M2: row 0 changed under the cursor
    {
        _ignoreArmed = true;
        _ignoreFrom = nowMs;
        if (_row0 == TextId::Stop)
            _menuSel = 0;
    }

    for (uint8_t i = 0; i < _inCount; ++i)
    {
        const Input &in = _inputs[i];
        if (in.kind == Input::Click)
            applyClick(in.ms);
        else if (in.kind == Input::Hold)
            applyHold(in.ms);
        else
            applyTurn(in.detents, in.extra, in.ms);
    }
    _inCount = 0;

    if (_jogDir && !within(nowMs, _jogMs, kJogStopMs))  // M4: stop 300 ms after the last detent
        jogStop();
    if (_stepLatch && (mountMoving() || !within(nowMs, _stepMs, kStepLatchMs)))  // the snapshot caught up, or the move was too short to show
        _stepLatch = false;

    // Expire latches (after the inputs, which may carry older stamps), so a signed difference never
    // sees them again after 2^31 ms.
    if (_ignoreArmed && !within(nowMs, _ignoreFrom, kIgnoreMs))
        _ignoreArmed = false;
    if (_transient != TextId::Count && !within(nowMs, _transientFrom, _transientMs))
        _transient = TextId::Count;
    if (_message != TextId::Count && !within(nowMs, _messageFrom, _messageMs))
        _message = TextId::Count;

    const bool stopped = _transient == TextId::HdrStopped && within(nowMs, _transientFrom, _transientMs);
    if (prev == MountState::Slewing && m.state == MountState::Tracking && !stopped && _screen != Screen::Adjust)  // S8: GoTo ends tracking; STOP ends idle
    {
        showTransient(TextId::HdrArrived, kArrivedMs, nowMs);
        push(UiAction::Kind::Beep);
        push(UiAction::Kind::Beep);
    }

    const bool fault = faultActive();
    if (fault && !_faultSeen)  // S14: fault page over everything, motors stopped
    {
        _faultPage = true;
        _saver = false;
        _jogDir = 0;  // the Stop below ends it
        push(UiAction::Kind::Stop);
        push(UiAction::Kind::BeepLong);
    }
    _faultSeen = fault;

    if (m.rebooted && !_rebootSeen)
        _screen = Screen::Rebooted;
    _rebootSeen = m.rebooted;

    // BELOW MIN ALT with 1 degree hysteresis: set below min, cleared at min + 1.
    bool low = false;
    if (m.state == MountState::Tracking && dateTimeSet())
    {
        int alt, az;
        altAz(alt, az);
        const int minAlt = _settings.get(SettingId::MinAlt);
        low = alt < minAlt || (_low && alt < minAlt + 1);
    }
    if (low && !_low)
        for (int i = 0; i < 3; ++i)
            push(UiAction::Kind::Beep);
    _low = low;

    // M3: saver only while idle or tracking, never over the fault page; leaves at once otherwise.
    if (_faultPage || (m.state != MountState::Idle && m.state != MountState::Tracking))
        _saver = false;
    else if (static_cast<int32_t>(nowMs - _lastInputMs) >= static_cast<int32_t>(kSaverMs))
    {
        _saver = true;
        _lastInputMs = nowMs - kSaverMs;  // keep the difference bounded across millis() wrap
    }

    HashCanvas h;
    render(h);
    if (_drawn && h.h == _hash)
        return false;
    _drawn = true;
    _hash = h.h;
    _canvas.clear();
    render(_canvas);
    return true;
}

// ---- rendering (v2 mockup layouts) ----

void UiController::render(Canvas &c) const
{
    if (_faultPage)
        renderFault(c);
    else if (_saver)
        renderSaver(c);
    else
        renderScreen(c);
}

void UiController::renderScreen(Canvas &c) const
{
    switch (_screen)
    {
        case Screen::Status:
            renderStatus(c);
            break;
        case Screen::Menu:
            renderMenu(c);
            break;
        case Screen::Rebooted:
            renderRebooted(c);
            break;
        case Screen::Editor:
            renderEdit(c);
            break;
        case Screen::PointConfirm:
            renderConfirm(c);
            break;
        case Screen::Refused:
            renderRefused(c);
            break;
        case Screen::ClearsSync:
            renderSmallConfirm(c, TextId::HdrSetup, TextId::ClearsSync);
            break;
        case Screen::Setup:
        case Screen::Settings:
        case Screen::AdjustOptions:
            renderList(c);
            break;
        case Screen::Adjust:
            renderAdjust(c);
            break;
        case Screen::SetHomeConfirm:
            renderSmallConfirm(c, TextId::HdrHome, TextId::SetHomeQ);
            break;
    }
}

// Header: 6x10 text + dotted rule; '!' in the last column while a warning is active; only the
// fault page inverts it.
void UiController::header(Canvas &c, const char *left, const char *right, bool inverted) const
{
    const bool warn = !inverted && warning() != TextId::Count;
    if (inverted)
        c.box(0, 0, 127, 10);
    else
        c.dots(0, 127, 11);
    c.text(Font::F6x10, 1, 1, left, inverted);
    if (warn)
        c.text(Font::F6x10, 122, 1, "!", false);
    if (right && *right)
        c.text(Font::F6x10, static_cast<int16_t>((warn ? 121 : 127) - 6 * cols(right)), 1, right, inverted);
}

void UiController::bar(Canvas &c, int16_t y0, int16_t y1, int16_t ty, const char *s) const
{
    c.box(0, y0, 127, y1);
    center(c, Font::F6x10, ty, s, true);
}

// One footer scheme: STOP bar while moving (M1), else a 2 s message, else the hint.
void UiController::footer(Canvas &c, const char *hint) const
{
    if (moving())
        bar(c, 53, 63, 54, t(TextId::FootStop));
    else if (messageActive())
        bar(c, 53, 63, 54, t(_message));
    else
    {
        c.dots(0, 127, 53);
        center(c, Font::F6x10, 54, hint, false);
    }
}

void UiController::renderStatus(Canvas &c) const
{
    char clock[16], l[64], a[24], b[24];
    const bool known = _m.timeSet;  // positions are meaningless without the time
    if (known)
        hms(clock, sizeof clock, _m.localSeconds);
    else
        strcpy(clock, "--:--:--");
    const bool transient = _transient != TextId::Count && within(_now, _transientFrom, _transientMs);
    header(c, t(transient ? _transient : stateWord()), clock);

    if (known)
    {
        int32_t ha = _m.lstSeconds - _m.raSeconds;
        while (ha >= 43200)
            ha -= 86400;
        while (ha < -43200)
            ha += 86400;
        const int32_t hv = ha < 0 ? -ha : ha;
        hms(a, sizeof a, _m.raSeconds);
        snprintf(l, sizeof l, "RA %s HA%c%02d:%02d", a, ha < 0 ? '-' : '+', (int)(hv / 3600), (int)(hv / 60 % 60));
        c.text(Font::F6x10, 0, 13, l, false);
        dms(a, sizeof a, _m.decArcSec, true);
        snprintf(l, sizeof l, "DE %s", a);
        c.text(Font::F6x10, 0, 23, l, false);
        int alt, az;
        altAz(alt, az);
        snprintf(l, sizeof l, "%s %c%02d%s  Az %03d%s", t(TextId::ShortAlt), alt < 0 ? '-' : '+', alt < 0 ? -alt : alt, kDeg, az, kDeg);
        c.text(Font::F6x10, 0, 33, l, false);
    }
    else
    {
        c.text(Font::F6x10, 0, 13, "RA --:--:-- HA --:--", false);
        snprintf(l, sizeof l, "DE --%s--'--\"", kDeg);
        c.text(Font::F6x10, 0, 23, l, false);
        snprintf(l, sizeof l, "%s --  Az ---%s", t(TextId::ShortAlt), kDeg);
        c.text(Font::F6x10, 0, 33, l, false);
    }

    const int off = _m.utcOffsetHours;
    if (_m.state == MountState::Slewing && _m.hasTarget)  // S7: target replaces LST while slewing
    {
        hms(a, sizeof a, _m.targetRaSeconds);
        dms(b, sizeof b, _m.targetDecArcSec, false);
        snprintf(l, sizeof l, "> %s %s", a, b);
    }
    else if (_m.state == MountState::Parking || _m.state == MountState::Homing)  // M7
        snprintf(l, sizeof l, "%s", t(_m.state == MountState::Parking ? TextId::ParkPos : TextId::HomePos));
    else
    {
        if (known)
            hms(a, sizeof a, _m.lstSeconds);
        else
            strcpy(a, "--:--:--");
        snprintf(l, sizeof l, "LST %s UTC%c%d", a, off < 0 ? '-' : '+', off < 0 ? -off : off);
    }
    c.text(Font::F6x10, 0, 43, l, false);

    const TextId w = warning();
    if (!moving() && w != TextId::Count)
        bar(c, 53, 63, 54, t(w));
    else
        footer(c, t(TextId::FootMenu));
}

void UiController::renderMenu(Canvas &c) const
{
    Item it[RowCount];
    for (uint8_t r = 0; r < RowCount; ++r)
    {
        const TextId lock = rowLock(r);
        const char *label = t(rowLabel(r));
        const char *tag = lock == TextId::MsgDateTimeFirst ? t(TextId::TagTime) : "";
        if (*tag && cols(label) + 1 + cols(tag) <= 19)  // S6 tag right-aligned at column 19 when it fits
            row(it[r].s, sizeof it[r].s, label, tag);
        else
            snprintf(it[r].s, sizeof it[r].s, "%s", label);
        it[r].lock = lock != TextId::Count;
    }
    list(c, TextId::HdrMenu, it, RowCount, _menuSel);
}

// List screen: header, up to five 6x10 rows (four while the bottom bar shows), selection bar,
// locked rows dotted, scrollbar when the rows do not fit.
void UiController::list(Canvas &c, TextId title, const Item *it, uint8_t n, uint8_t sel) const
{
    header(c, t(title), nullptr);
    const bool barShown = moving() || messageActive();
    const int vis = barShown ? 4 : 5;  // the bottom bar covers the fifth row
    const int top = sel < vis ? 0 : sel - vis + 1;
    for (int i = 0; i < vis && top + i < n; ++i)
    {
        const uint8_t r = static_cast<uint8_t>(top + i);
        const int16_t y = static_cast<int16_t>(13 + 10 * i);
        const bool on = r == sel;
        if (on)
            c.box(0, y - 1, 121, y + 8);
        c.text(Font::F6x10, 2, y, it[r].s, on);
        if (it[r].lock)
            c.dots(2, static_cast<int16_t>(2 + 6 * cols(it[r].s) - 1), y + 9);
    }
    if (n > vis)  // scrollbar
    {
        const int bottom = barShown ? 52 : 63, len = bottom - 11;
        const int h = (2 * len * vis + n) / (2 * n);
        const int y0 = 12 + ((len - h) * top * 2 + (n - vis)) / (2 * (n - vis));
        c.frame(124, 12, 127, static_cast<int16_t>(bottom));
        c.box(125, static_cast<int16_t>(y0), 126, static_cast<int16_t>(y0 + (h < 6 ? 6 : h) - 1));
    }
    if (barShown)
        footer(c, "");
}

void UiController::renderRebooted(Canvas &c) const
{
    header(c, t(TextId::HdrWarning), nullptr);
    c.frame(0, 17, 127, 49);
    center(c, Font::F6x10, 23, t(TextId::AlertRebooted), false);
    center(c, Font::F6x10, 35, t(TextId::DateTimeLost), false);
    footer(c, t(TextId::FootSetTime));
}

void UiController::renderFault(Canvas &c) const
{
    header(c, t(TextId::HdrFault), nullptr, true);
    c.frame(0, 17, 127, 49);
    center(c, Font::F6x10, 23, t(faultText()), false);
    center(c, Font::F6x10, 35, t(TextId::MotorsStopped), false);
    footer(c, t(TextId::FootOk));
}

void UiController::renderSaver(Canvas &c) const
{
    char l[48];
    if (_m.timeSet)
        snprintf(l, sizeof l, "%02d:%02d", (int)(_m.localSeconds / 3600), (int)(_m.localSeconds / 60 % 60));
    else
        strcpy(l, "--:--");
    center(c, Font::F10x20, 6, l, false);
    center(c, Font::F6x10, 31, t(stateWord()), false);
    const TextId w = warning();
    if (w != TextId::Count)  // the warning replaces the RA/Dec line
        bar(c, 41, 51, 42, t(w));
    else
    {
        const int32_t d = _m.decArcSec < 0 ? -_m.decArcSec : _m.decArcSec;
        snprintf(l, sizeof l, "RA %02d:%02d DE%c%02d%s%02d", (int)(_m.raSeconds / 3600), (int)(_m.raSeconds / 60 % 60),
                 _m.decArcSec < 0 ? '-' : '+', (int)(d / 3600), kDeg, (int)(d / 60 % 60));
        center(c, Font::F6x10, 42, l, false);
    }
    center(c, Font::F6x10, 54, t(TextId::TurnToWake), false);
}

// Big-digit editor: header title + step, the value in 10x20 with the selected group inverted,
// sub-label, footer. Groups are runs of digits/letters, or a leading sign.
void UiController::renderEditor(Canvas &c, TextId title, TextId sub, TextId foot) const
{
    char s[24];
    _editor.format(s, sizeof s);
    uint8_t g0[6] = {}, g1[6] = {};
    uint8_t n = 0;
    for (uint8_t i = 0; s[i] && n < 6;)
    {
        if (i == 0 && (s[i] == '+' || s[i] == '-'))
        {
            g0[n] = 0;
            g1[n++] = 1;
            i = 1;
        }
        else if (isalnum(static_cast<unsigned char>(s[i])))
        {
            g0[n] = i;
            while (isalnum(static_cast<unsigned char>(s[i])))
                ++i;
            g1[n++] = i;
        }
        else
            ++i;
    }
    const uint8_t g = _editor.group() < n ? _editor.group() : 0;
    char step[8];
    snprintf(step, sizeof step, "%d/%d", g + 1, n);
    header(c, t(title), step);

    char part[24];
    const int16_t x0 = static_cast<int16_t>((128 - 10 * cols(s) + 1) / 2);
    snprintf(part, sizeof part, "%.*s", (int)g0[g], s);
    const int16_t xs = static_cast<int16_t>(x0 + 10 * cols(part));
    if (*part)
        c.text(Font::F10x20, x0, 14, part, false);
    snprintf(part, sizeof part, "%.*s", (int)(g1[g] - g0[g]), s + g0[g]);
    const int16_t xe = static_cast<int16_t>(xs + 10 * cols(part));
    c.box(static_cast<int16_t>(xs - 1), 13, xe, 34);
    c.text(Font::F10x20, xs, 14, part, true);
    if (s[g1[g]])
        c.text(Font::F10x20, xe, 14, s + g1[g], false);
    center(c, Font::F6x10, 38, t(sub), false);
    footer(c, t(foot));
}

}  // namespace core
