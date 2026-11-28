// UiController, part 2: Point (GO / SYNC), Adjust, Setup and Settings screens.
// Safety (STOP while moving, fault page, saver, 500 ms click ignore) runs in UiController.cpp
// before any of these handlers.

#include <cmath>
#include <cstdio>
#include <cstring>
#include "core/CalendarMath.hpp"
#include "core/ui/UiController.hpp"
#include "core/ui/UiDraw.hpp"

namespace core {

using namespace draw;

namespace {

constexpr float kBigJumpDeg = 5.0f;  // M5
// Adjust step sizes in steps, per axis id (0 RA, 1 DEC, 2 AZ, 3 Focus, 4 ALT); at most 127 detents * 300000 fits int32_t.
constexpr int32_t kStepSizes[5][5] = {{10, 100, 1000, 10000, 50000},
                                      {10, 100, 1000, 10000, 50000},
                                      {100, 1000, 10000, 60000, 300000},
                                      {1, 10, 100, 1000, 10000},
                                      {100, 1000, 10000, 60000, 300000}};

int32_t wrapHalfDay(int32_t s)  // seconds -> (-43200, 43200]
{
    s %= 86400;
    if (s > 43200)
        s -= 86400;
    if (s <= -43200)
        s += 86400;
    return s;
}

}  // namespace

// ---- editors ----

void UiController::openEditor(Edit e, int32_t value, bool lastGroup)
{
    _edit = e;
    _fastPath = false;  // fast-path callers set it again after opening
    switch (e)
    {
        case Edit::PointRa:
            _editor.begin(FieldKind::RA, value);
            break;
        case Edit::PointDec:
            _editor.begin(FieldKind::Dec, value);
            break;
        case Edit::Date:
            _editor.begin(FieldKind::Date, value);
            break;
        case Edit::Time:  // S1c: hh:mm, seconds 00 at the click
            _editor.begin(FieldKind::TimeHM, value);
            break;
        case Edit::Utc:
            _editor.begin(FieldKind::Int, value, -12, 14);
            break;
        case Edit::Lat:
        case Edit::Lon:  // S10: hemisphere letter as the last group
            _editor.begin(e == Edit::Lat ? FieldKind::Latitude : FieldKind::Longitude, value);
            _editor.setLetters(letter(true, e == Edit::Lat), letter(false, e == Edit::Lat));
            break;
        case Edit::MinAlt:
        {
            const SettingRow &r = settingRow(SettingId::MinAlt);
            _editor.begin(FieldKind::Int, value, r.min, r.max);
            break;
        }
    }
    if (lastGroup)
        while (!_editor.click())
        {
        }
    _screen = Screen::Editor;
}

void UiController::editorDone()
{
    _pendingN = 0;  // a "Sure?" left by STOP or the fault page must never ride along with a new edit
    const int32_t v = _editor.value();
    switch (_edit)
    {
        case Edit::PointRa:
            _ra = v;
            openEditor(Edit::PointDec, _dec);
            break;
        case Edit::PointDec:
            _dec = v;
            openPointConfirm();
            break;
        case Edit::Date:
            if (_fastPath)  // S1a: then the time; both applied together
            {
                _pendingDate = v;
                openEditor(Edit::Time, _m.localSeconds);
                _fastPath = true;
            }
            else
            {
                pend(UiAction::Kind::SetDate, v);
                commit(Screen::Setup);
            }
            break;
        case Edit::Time:
            if (_fastPath)
            {
                pend(UiAction::Kind::SetDate, _pendingDate);
                pend(UiAction::Kind::SetTime, v);
                commit(Screen::Status);
            }
            else
            {
                pend(UiAction::Kind::SetTime, v);
                commit(Screen::Setup);
            }
            break;
        case Edit::Utc:
            pend(UiAction::Kind::SetUtcOffset, v);
            commit(Screen::Setup);
            break;
        case Edit::Lat:
            pend(UiAction::Kind::SetLatitude, v);
            commit(Screen::Setup);
            break;
        case Edit::Lon:
            pend(UiAction::Kind::SetLongitude, v);
            commit(Screen::Setup);
            break;
        case Edit::MinAlt:
            _settings.set(SettingId::MinAlt, v);
            _screen = Screen::Settings;
            break;
    }
}

// S3: hold = previous group; on the first group, back to the previous editor or out.
void UiController::editorBack()
{
    if (!_editor.back())
        return;
    switch (_edit)
    {
        case Edit::PointDec:
            _dec = _editor.value();
            openEditor(Edit::PointRa, _ra, true);
            break;
        case Edit::Time:
            if (_fastPath)
            {
                openEditor(Edit::Date, _pendingDate, true);
                _fastPath = true;
            }
            else
                _screen = Screen::Setup;
            break;
        case Edit::Date:
            if (_fastPath)
                openMenu();
            else
                _screen = Screen::Setup;
            break;
        case Edit::Utc:
        case Edit::Lat:
        case Edit::Lon:
            _screen = Screen::Setup;
            break;
        case Edit::MinAlt:
            _screen = Screen::Settings;
            break;
        case Edit::PointRa:
            openMenu();
            break;
    }
}

// The one place setup values reach the mount. After a Sync, a date, time, UTC offset or longitude
// change asks "Clears Sync. Sure?" first and is applied only on yes (S5).
void UiController::commit(Screen ret)
{
    bool ask = false;
    for (uint8_t i = 0; i < _pendingN; ++i)
        ask |= _pending[i].kind != UiAction::Kind::SetLatitude;
    if (ask && _m.synced)
    {
        _ret = ret;
        _screen = Screen::ClearsSync;
        return;
    }
    for (uint8_t i = 0; i < _pendingN; ++i)
        push(_pending[i].kind, _pending[i].a);
    _pendingN = 0;
    _screen = ret;
}

// S4/S9: GoTo safety gates (date and time are set: the row is locked otherwise). Also re-run at the GO click.
void UiController::evalGoto()
{
    const CalendarDate utc = localToUtcDate({_m.year, _m.month, _m.day}, _m.localSeconds, _m.utcOffsetHours);
    const int32_t utcSec = ((_m.localSeconds - _m.utcOffsetHours * 3600) % 86400 + 86400) % 86400;
    const float ra = _ra / 3600.0f, dec = _dec / 3600.0f, lst = _m.lstSeconds / 3600.0f, lat = _m.latArcSec / 3600.0f;
    _check = checkTarget(ra, dec, lst, lat, _settings.get(SettingId::MinAlt), utc.year, utc.month, utc.day, utcSec / 3600.0f);
    const AltAz h = equatorialToHorizontal(lst - ra, dec, lat);
    _alt = static_cast<int>(lroundf(h.altDeg));
    _az = static_cast<int>(lroundf(h.azDeg)) % 360;
    if (_az < 0)
        _az += 360;
    float sra = 0, sdec = 0;
    sunRaDec(utc.year, utc.month, utc.day, utcSec / 3600.0f, sra, sdec);
    const float sun = angularSeparationDeg(ra, dec, sra, sdec);
    _sun = std::isfinite(sun) ? static_cast<int>(lroundf(sun)) : 0;
}

// The Point confirm screen: GO selected, armed 600 ms later. The SYNC offset is worked out here, the
// GO checks run again at the click (evalGoto), so a refusal only shows when GO is clicked.
void UiController::openPointConfirm()
{
    evalGoto();
    _dRa = wrapHalfDay(_ra - _m.raSeconds);
    _dDec = _dec - _m.decArcSec;
    _bigJump = angularSeparationDeg(_m.raSeconds / 3600.0f, _m.decArcSec / 3600.0f, _ra / 3600.0f, _dec / 3600.0f) > kBigJumpDeg;
    _pointSync = false;
    _screenFrom = _now;
    _screen = Screen::PointConfirm;
}

// ---- list screens ----

uint8_t UiController::items(Item *out) const
{
    uint8_t n = 0;
    auto add = [&](const char *label, const char *value, bool lock) {
        if (value)
            row(out[n].s, sizeof out[n].s, label, value);
        else
            snprintf(out[n].s, sizeof out[n].s, "%s", label);
        out[n++].lock = lock;
    };
    char v[24];
    switch (_screen)
    {
        case Screen::Setup:
            if (_m.dateSet)
                formatEdit(v, sizeof v, FieldKind::Date, _m.year * 10000 + _m.month * 100 + _m.day);
            else
                strcpy(v, "----------");
            add(t(TextId::Date), v, false);
            if (_m.timeSet)
                formatEdit(v, sizeof v, FieldKind::TimeHM, _m.localSeconds);
            else
                strcpy(v, "--:--");
            add(t(TextId::Time), v, false);
            formatEdit(v, sizeof v, FieldKind::Int, _m.utcOffsetHours);
            add(t(TextId::UtcOffset), v, false);
            formatEdit(v, sizeof v, FieldKind::Latitude, _m.latArcSec);
            add(t(TextId::Latitude), v, false);
            formatEdit(v, sizeof v, FieldKind::Longitude, _m.lonArcSec);
            add(t(TextId::Longitude), v, false);
            break;
        case Screen::Settings:
            add(t(TextId::Setup), nullptr, false);  // Date, time, UTC offset, position live under Settings
            add(t(TextId::LangRow), t(TextId::LangName), false);
            snprintf(v, sizeof v, "%d%s", (int)_settings.get(SettingId::MinAlt), kDeg);  // S16: 0 not +0
            add(t(TextId::MinAlt), v, false);
            add(t(TextId::BeepRow), t(_settings.get(SettingId::Beep) ? TextId::On : TextId::Off), false);
            supplyText(v, sizeof v);
            add(t(TextId::SupplyRow), v, false);
            add(t(TextId::NetworkRow), nullptr, false);  // kNetworkRow
            break;
        case Screen::AdjustOptions:
            stepText(v, sizeof v);
            add(t(TextId::StepRow), v, false);
            add(t(TextId::SyncOnTarget), nullptr, optionLock(1) != TextId::Count);
            add(t(TextId::SetHomeHere), nullptr, optionLock(2) != TextId::Count);
            add(t(TextId::SetHomeAll), nullptr, optionLock(3) != TextId::Count);
            add(t(TextId::GoHome), nullptr, optionLock(4) != TextId::Count);
            add(t(TextId::MainMenu), nullptr, false);
            break;
        default:
            break;
    }
    add(t(TextId::Back), nullptr, false);
    return n;
}

void UiController::supplyText(char *out, size_t n) const
{
    const int tenths = (_m.supplyMv + 50) / 100;  // measured, one decimal (integer maths: no float printf)
    const int applied = _m.supplyVoltage;
    if (_settings.get(SettingId::Supply) == 0)  // Auto: the applied choice and the measurement (row width: 12 EN, 11 TR)
    {
        if (tenths && applied)
            snprintf(out, n, "%s %d %d.%d", t(TextId::SupplyAuto), applied, tenths / 10, tenths % 10);
        else if (tenths)
            snprintf(out, n, "%s %d.%dV", t(TextId::SupplyAuto), tenths / 10, tenths % 10);
        else if (applied)
            snprintf(out, n, "%s %d --", t(TextId::SupplyAuto), applied);
        else
            snprintf(out, n, "%s --", t(TextId::SupplyAuto));
    }
    else if (tenths)
        snprintf(out, n, "%d V (%d.%d)", supplyVolts(), tenths / 10, tenths % 10);
    else
        snprintf(out, n, "%d V", supplyVolts());
}

void UiController::clickList(uint32_t nowMs)
{
    Item it[kItems];
    const uint8_t n = items(it);
    if (_sel >= n)
        _sel = static_cast<uint8_t>(n - 1);
    if (it[_sel].lock)
    {
        push(UiAction::Kind::BeepError);
        showMessage(_screen == Screen::AdjustOptions ? optionLock(_sel) : faultText(), kMessageMs, nowMs);
        return;
    }
    clickTick();
    if (_sel == n - 1)  // Back: one level up, like a long press
        return holdScreen();
    switch (_screen)
    {
        case Screen::Setup:
            clickSetup(_sel);
            break;
        case Screen::Settings:
            clickSettings(_sel);
            break;
        case Screen::AdjustOptions:
            clickAdjustOptions(_sel);
            break;
        default:
            break;
    }
}

// Adjust options: 0 Step size, 1 Sync on target, 2 Set home here, 3 Set home all, 4 Go home, 5 Main menu (never locked).
// The locks match the rows' dots.
TextId UiController::optionLock(uint8_t row) const
{
    switch (row)
    {
        case 1:  // the last Point target, and the clock for the sky reference
            return !dateTimeSet() ? TextId::MsgDateTimeFirst : (_haveTarget ? TextId::Count : TextId::MsgNoTarget);
        case 2:  // the markers: only while nothing moves or tracks
        case 3:
            return _m.state == MountState::Idle ? TextId::Count : TextId::MsgStopFirst;
        case 4:  // from idle or tracking, no clock needed
            return faultActive() ? faultText() : TextId::Count;
        default:
            return TextId::Count;
    }
}

void UiController::clickAdjustOptions(uint8_t r)
{
    switch (r)
    {
        case 0:  // next size of the current axis
            _stepSel[_axis] = static_cast<uint8_t>((_stepSel[_axis] + 1) % 5);
            break;
        case 1:
            push(UiAction::Kind::Sync, _tgtRa, _tgtDec);
            showTransient(TextId::HdrSynced, kMessageMs, _now);
            _screen = Screen::Status;
            break;
        case 2:
        case 3:
            _homeAll = r == 3;
            _screen = Screen::SetHomeConfirm;
            break;
        case 4:
            push(UiAction::Kind::GoHome);
            _screen = Screen::Status;
            break;
        default:
            openMenu();
            break;
    }
}

// Turn on Adjust: one step size per detent (detents of one tick merge into one move).
// Never moves an axis whose driver failed.
void UiController::turnAdjust(int8_t detents, uint32_t nowMs)
{
    if (axisFaulted())
    {
        push(UiAction::Kind::BeepError);
        showMessage(_axis == 4 ? TextId::FaultAlt : TextId::FaultDriver, kMessageMs, nowMs);
        return;
    }
    push(UiAction::Kind::MoveSteps, detents * stepSize(), _axis);
    _stepLatch = true;  // a click in the same tick must STOP this move, not cycle the axis
    _stepMs = nowMs;
}

void UiController::clickSetup(uint8_t r)
{
    switch (r)
    {
        case 0:
            openDateEditor();  // S1b: prefilled from the mount or the last saved date
            break;
        case 1:
            openEditor(Edit::Time, _m.localSeconds);
            break;
        case 2:
            openEditor(Edit::Utc, _m.utcOffsetHours);
            break;
        case 3:
            openEditor(Edit::Lat, _m.latArcSec);
            break;
        default:
            openEditor(Edit::Lon, _m.lonArcSec);
            break;
    }
}

void UiController::clickSettings(uint8_t r)
{
    if (r == 0)  // Setup
    {
        _sel = 0;
        _screen = Screen::Setup;
        return;
    }
    if (r == kNetworkRow)
    {
        _screen = Screen::Network;
        return;
    }
    const SettingId id = static_cast<SettingId>(r - 1);
    switch (id)
    {
        case SettingId::MinAlt:
            openEditor(Edit::MinAlt, _settings.get(id));
            break;
        case SettingId::Supply:  // Auto, then the applied value and on round the four, back to Auto at the start
        {
            int i = 0;  // a value that is not a choice counts as the last one (24 V)
            while (i < 3 && kSupplyVolts[i] != supplyVolts())
                ++i;
            int idx;  // next manual choice, 0..3
            bool toAuto = false;
            if (_settings.get(id) == 0)
            {
                idx = _m.supplySense ? i : 0;  // leaving Auto changes nothing (no sense input: Auto is only a stand-in, go to 12)
                _supplyOrigin = static_cast<uint8_t>(idx);
            }
            else
            {
                idx = (i + 1) % 4;
                toAuto = _m.supplySense && idx == _supplyOrigin;  // no Auto without a sense input
            }
            _settings.set(id, toAuto ? 0 : idx + 1);
            if (!toAuto && kSupplyVolts[idx] != _m.supplyVoltage)
                push(UiAction::Kind::SetSupplyVoltage, kSupplyVolts[idx]);
            break;
        }
        default:
            _settings.set(id, _settings.next(id));
            _lang = _settings.get(SettingId::Language) ? Lang::TR : Lang::EN;
            break;
    }
}

int32_t UiController::stepSize() const
{
    return kStepSizes[_axis][_stepSel[_axis]];
}

char UiController::letter(bool positive, bool lat) const
{
    if (_lang == Lang::TR)
        return lat ? (positive ? 'K' : 'G') : (positive ? 'D' : 'B');
    return lat ? (positive ? 'N' : 'S') : (positive ? 'E' : 'W');
}

// The editor's own text for a value, so the Setup list and the editor always match.
void UiController::formatEdit(char *out, size_t n, FieldKind k, int32_t v) const
{
    FieldEditor e;
    e.begin(k, v, -12, 14);
    if (k == FieldKind::Latitude || k == FieldKind::Longitude)
        e.setLetters(letter(true, k == FieldKind::Latitude), letter(false, k == FieldKind::Latitude));
    e.format(out, n);
}

// ---- rendering ----

void UiController::renderEdit(Canvas &c) const
{
    static const TextId kHms[] = {TextId::SubHours, TextId::SubMinutes, TextId::SubSeconds};
    static const TextId kDms[] = {TextId::SubSign, TextId::SubDegrees, TextId::SubMinutes, TextId::SubSeconds};
    static const TextId kDate[] = {TextId::SubYear, TextId::SubMonth, TextId::SubDay};
    static const TextId kUtc[] = {TextId::SubSign, TextId::SubHours};
    static const TextId kLat[] = {TextId::SubDegrees, TextId::SubMinutes, TextId::SubNorthSouth};
    static const TextId kLon[] = {TextId::SubDegrees, TextId::SubMinutes, TextId::SubEastWest};
    static const TextId kDeg1[] = {TextId::SubSign, TextId::SubDegrees};
    const TextId *sub = kHms;
    uint8_t n = 3;
    TextId title = TextId::HdrSetupDate, last = TextId::FootSave;
    switch (_edit)
    {
        case Edit::PointRa:
            title = TextId::HdrPointRa;
            last = TextId::FootNextBack;
            break;
        case Edit::PointDec:
            title = TextId::HdrPointDec;
            sub = kDms, n = 4, last = TextId::FootCheck;
            break;
        case Edit::Date:
            sub = kDate;
            if (_fastPath)
                last = TextId::FootNextBack;
            break;
        case Edit::Time:
            title = TextId::HdrSetupTime, n = 2, last = TextId::FootAt00;
            break;
        case Edit::Utc:
            title = TextId::HdrSetupUtc, sub = kUtc, n = 2;
            break;
        case Edit::Lat:
            title = TextId::HdrSetupLat, sub = kLat;
            break;
        case Edit::Lon:
            title = TextId::HdrSetupLon, sub = kLon;
            break;
        case Edit::MinAlt:
            title = TextId::HdrMinAlt, sub = kDeg1, n = 2;
            break;
    }
    const uint8_t g = _editor.group() < n ? _editor.group() : n - 1;
    renderEditor(c, title, sub[g], g == n - 1 ? last : TextId::FootNextBack);
}

void UiController::renderList(Canvas &c) const
{
    Item it[kItems];
    const uint8_t n = items(it);
    const TextId title = _screen == Screen::Setup ? TextId::HdrSetup : (_screen == Screen::AdjustOptions ? TextId::HdrAdjust : TextId::HdrSettings);
    list(c, title, it, n, _sel < n ? _sel : n - 1);
}

// Point confirm: the target, what GO or SYNC would do, and the GO | SYNC selector the knob turns.
// The click bar appears once armed (600 ms).
void UiController::renderConfirm(Canvas &c) const
{
    char l[48], a[24], b[24];
    header(c, t(TextId::HdrPoint), t(TextId::HdrConfirm));
    hms(a, sizeof a, _ra);
    dms(b, sizeof b, _dec, true);
    const bool armed = !within(_now, _screenFrom, kArmMs);
    int16_t top = 42;  // selector row
    if (!_pointSync)
    {
        snprintf(l, sizeof l, "RA %s", a);
        c.text(Font::F6x10, 0, 13, l, false);
        snprintf(l, sizeof l, "DE %s", b);
        c.text(Font::F6x10, 0, 23, l, false);
        // S4: direction sanity check.
        snprintf(l, sizeof l, "%s%c%02d%s Az%03d%s %s%d", t(TextId::ShortAlt), _alt < 0 ? '-' : '+', _alt < 0 ? -_alt : _alt, kDeg, _az, kDeg,
                 t(TextId::ShortSun), _sun);
        c.text(Font::F6x10, 0, 33, l, false);
    }
    else
    {
        // M5: the offset; above 5 degrees a "BIG JUMP" frame.
        const int32_t ra = _dRa < 0 ? -_dRa : _dRa, de = _dDec < 0 ? -_dDec : _dDec;
        char off[32];
        snprintf(off, sizeof off, "dRA%c%d:%02d dDE%c%d%s", _dRa < 0 ? '-' : '+', (int)(ra / 3600), (int)(ra / 60 % 60), _dDec < 0 ? '-' : '+',
                 (int)((de + 1800) / 3600), kDeg);
        if (cols(off) + 1 + cols(t(TextId::ShortMove)) <= 21)
            snprintf(l, sizeof l, "%s %s", t(TextId::ShortMove), off);
        else
            snprintf(l, sizeof l, "%s", off);
        if (_bigJump)
        {
            char both[48];
            snprintf(both, sizeof both, "%s %s", a, b);
            c.text(Font::F6x10, 0, 13, both, false);
            c.text(Font::F6x10, 0, 22, l, false);
            c.frame(0, 30, 127, 41);
            center(c, Font::F6x10, 31, t(TextId::BigJump), false);
            top = 43;
        }
        else
        {
            char line[32];
            snprintf(line, sizeof line, "RA %s", a);
            c.text(Font::F6x10, 0, 13, line, false);
            snprintf(line, sizeof line, "DE %s", b);
            c.text(Font::F6x10, 0, 23, line, false);
            c.text(Font::F6x10, 0, 33, l, false);
        }
    }
    // Selector: two cells, the chosen one inverted.
    for (int i = 0; i < 2; ++i)
    {
        const bool on = (i == 1) == _pointSync;
        const int16_t x0 = i ? 65 : 0, x1 = i ? 127 : 62;
        const char *label = t(i ? TextId::HdrSync : TextId::ChoiceGo);
        if (on)
            c.box(x0, top, x1, 52);
        else
            c.frame(x0, top, x1, 52);
        c.text(Font::F6x10, static_cast<int16_t>(x0 + (x1 - x0 + 1 - 6 * cols(label)) / 2), 43, label, on);
    }
    if (armed && !moving() && !messageActive())
        bar(c, 53, 63, 54, t(_pointSync ? TextId::ClickSync : TextId::ClickGo));
    else
        footer(c, t(TextId::FootCancel));
}

void UiController::stepText(char *out, size_t n) const
{
    snprintf(out, n, "%d", (int)stepSize());
}

// Adjust: axis (header), the axis strip, position, step size with its degrees (RA, DEC, AZ, ALT; Focus has steps only).
void UiController::renderAdjust(Canvas &c) const
{
    char l[48], v[24];
    const TextId names[kAxes] = {TextId::LabelRa, TextId::LabelDec, TextId::LabelAz, TextId::AxisFocus, TextId::LabelAlt};
    const bool stopped = _transient == TextId::HdrStopped && within(_now, _transientFrom, _transientMs);  // STOPPED stays on this page
    header(c, t(stopped ? TextId::HdrStopped : TextId::HdrAdjust), t(names[_axis]));
    snprintf(l, sizeof l, "%s %ld", t(TextId::PosRow), (long)_m.axisPos[_axis]);
    c.text(Font::F6x10, 0, 13, l, false);
    stepText(v, sizeof v);
    if (_m.stepsPerDeg[_axis] > 0.0f)
    {
        const float deg = stepSize() / _m.stepsPerDeg[_axis];
        const long d100 = lroundf(deg * 100.0f);
        if (d100 >= 1 && d100 < 100000)
            snprintf(l, sizeof l, "%s %s = %ld.%02ld%s", t(TextId::StepRow), v, d100 / 100, d100 % 100, kDeg);
        else if (d100 < 1)  // below 0.01 degrees: arc-seconds
            snprintf(l, sizeof l, "%s %s = %ld.%ld\"", t(TextId::StepRow), v, lroundf(deg * 36000.0f) / 10, lroundf(deg * 36000.0f) % 10);
        else
            snprintf(l, sizeof l, "%s %s", t(TextId::StepRow), v);
    }
    else
        snprintf(l, sizeof l, "%s %s", t(TextId::StepRow), v);
    c.text(Font::F6x10, 0, 23, l, false);
    int16_t x = 2;  // the axes in use in display order, the selected one inverted; 8 px gaps fit all five
    for (uint8_t i = 0; i < kAxes; ++i)
    {
        const uint8_t a = kAxisOrder[i];
        if (!axisFitted(a))
            continue;
        const char *name = t(names[a]);
        const int16_t w = static_cast<int16_t>(6 * cols(name));
        if (a == _axis)
            c.box(static_cast<int16_t>(x - 1), 32, static_cast<int16_t>(x + w), 41);
        c.text(Font::F6x10, x, 33, name, a == _axis);
        x = static_cast<int16_t>(x + w + 8);
    }
    footer(c, t(TextId::FootAdjust));
}

void UiController::renderRefused(Canvas &c) const
{
    char l[48], a[24];
    header(c, t(TextId::HdrGoto), t(TextId::HdrRefused));
    hms(a, sizeof a, _ra);
    snprintf(l, sizeof l, "RA %s", a);
    c.text(Font::F6x10, 0, 13, l, false);
    dms(a, sizeof a, _dec, true);
    snprintf(l, sizeof l, "DE %s", a);
    c.text(Font::F6x10, 0, 23, l, false);
    TextId why = TextId::RefInvalid;
    if (_check == TargetCheck::BelowHorizon)
    {
        why = TextId::RefBelow;
        snprintf(l, sizeof l, "%s %+d%s (min %d%s)", t(TextId::ShortAlt), _alt, kDeg, (int)_settings.get(SettingId::MinAlt), kDeg);
        c.text(Font::F6x10, 0, 33, l, false);
    }
    else if (_check == TargetCheck::NearSun)
    {
        why = TextId::RefSun;
        snprintf(l, sizeof l, "%s %d%s %s (min 15)", t(TextId::ShortSun), _sun, kDeg, t(TextId::Away));
        c.text(Font::F6x10, 0, 33, l, false);
    }
    c.frame(0, 41, 127, 53);
    c.frame(1, 42, 126, 52);
    center(c, Font::F6x10, 43, t(why), false);
    footer(c, t(TextId::FootEdit));
}

// Network: mode, name (up to 20 characters, a line of its own), address, clients (+ signal in Hotspot mode), and the
// Own WiFi choice as the click bar. Name, address and clients only while the bridge reports.
void UiController::renderNetwork(Canvas &c) const
{
    static const TextId kModes[] = {TextId::Off, TextId::NetHotspot, TextId::NetOwnWifi, TextId::NetConnecting};
    static const TextId kOwn[] = {TextId::Auto, TextId::On, TextId::Off};
    const NetStatus &n = _m.net;
    const bool up = n.seen && within(_now, n.atMs, kNoBridgeMs);
    char l[48];
    header(c, t(TextId::HdrNetwork), nullptr);
    snprintf(l, sizeof l, "%s: %s", t(TextId::ModeRow), t(!up ? TextId::NetNoBridge : kModes[n.mode < 4 ? n.mode : 0]));
    c.text(Font::F6x10, 0, 13, l, false);
    if (up)
    {
        snprintf(l, sizeof l, "%.20s", n.ssid);
        c.text(Font::F6x10, 0, 23, l, false);
        if (n.ip[0] | n.ip[1] | n.ip[2] | n.ip[3])
            snprintf(l, sizeof l, "IP: %u.%u.%u.%u", n.ip[0], n.ip[1], n.ip[2], n.ip[3]);
        else
            snprintf(l, sizeof l, "IP: -");
        c.text(Font::F6x10, 0, 33, l, false);
        if (n.mode == 1 && n.rssi < 0)
            snprintf(l, sizeof l, "%s: %u  %d dBm", t(TextId::NetClients), n.clients % 10u, (int)n.rssi);
        else
            snprintf(l, sizeof l, "%s: %u", t(TextId::NetClients), n.clients % 10u);
        c.text(Font::F6x10, 0, 43, l, false);
    }
    snprintf(l, sizeof l, "%s: %s", t(TextId::NetOwnWifi), t(kOwn[_settings.get(SettingId::OwnWifi)]));
    if (moving() || messageActive())
        footer(c, "");
    else
        bar(c, 53, 63, 54, l);
}

void UiController::renderSmallConfirm(Canvas &c, TextId hdr, TextId msg) const
{
    header(c, t(hdr), nullptr);
    center(c, Font::F7x13B, 17, t(msg), false);
    c.box(0, 36, 127, 46);
    center(c, Font::F6x10, 37, t(TextId::ClickYes), true);
    footer(c, t(TextId::FootCancel));
}

}  // namespace core
