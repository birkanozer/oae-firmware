// UiController, part 2: GoTo, Sync, Setup, Jog, Home/Park and Settings screens (L3/L4).
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
        case Edit::GotoRa:
        case Edit::SyncRa:
            _editor.begin(FieldKind::RA, value);
            break;
        case Edit::GotoDec:
        case Edit::SyncDec:
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
        case Edit::GotoRa:
            _ra = v;
            openEditor(Edit::GotoDec, _dec);
            break;
        case Edit::GotoDec:
            _dec = v;
            checkGoto();
            break;
        case Edit::SyncRa:
            _ra = v;
            openEditor(Edit::SyncDec, _dec);
            break;
        case Edit::SyncDec:
            openSync(_ra, v);
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
        case Edit::GotoDec:
        case Edit::SyncDec:
            _dec = _editor.value();
            openEditor(_edit == Edit::GotoDec ? Edit::GotoRa : Edit::SyncRa, _ra, true);
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
        case Edit::GotoRa:
        case Edit::SyncRa:
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

void UiController::checkGoto()
{
    evalGoto();
    _screenFrom = _now;
    _screen = _check == TargetCheck::Ok ? Screen::GotoConfirm : Screen::Refused;
}

void UiController::openSync(int32_t ra, int32_t dec)
{
    _ra = ra, _dec = dec;
    _dRa = wrapHalfDay(ra - _m.raSeconds);
    _dDec = dec - _m.decArcSec;
    _bigJump = angularSeparationDeg(_m.raSeconds / 3600.0f, _m.decArcSec / 3600.0f, ra / 3600.0f, dec / 3600.0f) > kBigJumpDeg;
    _screenFrom = _now;
    _screen = Screen::SyncConfirm;
}

// ---- list screens ----

TextId UiController::homeRow(uint8_t r) const
{
    // S5: when parked, Unpark is the only motion row.
    static const TextId kIdle[] = {TextId::GoHome, TextId::SetHomeHere, TextId::Park, TextId::Back};
    static const TextId kParked[] = {TextId::Unpark, TextId::SetHomeHere, TextId::Back};
    if (_m.state == MountState::Parked)
        return r < 3 ? kParked[r] : TextId::Back;
    return r < 4 ? kIdle[r] : TextId::Back;
}

// Fault: every motion item locked. Unpark (like Track here) also needs date and time (M6).
TextId UiController::homeLock(TextId row) const
{
    if (faultActive())
        return faultText();
    if (row == TextId::Unpark && !dateTimeSet())
        return TextId::MsgDateTimeFirst;
    return TextId::Count;
}

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
        case Screen::JogMenu:
            add(t(TextId::JogRa), nullptr, false);
            add(t(TextId::JogDec), nullptr, false);
            add(t(TextId::Speed), t(speedText()), false);
            break;
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
        case Screen::HomePark:
            for (uint8_t r = 0; homeRow(r) != TextId::Back; ++r)
                add(t(homeRow(r)), nullptr, homeLock(homeRow(r)) != TextId::Count);
            break;
        case Screen::Settings:
            add(t(TextId::LangRow), t(TextId::LangName), false);
            snprintf(v, sizeof v, "%d%s", (int)_settings.get(SettingId::MinAlt), kDeg);  // S16: 0 not +0
            add(t(TextId::MinAlt), v, false);
            add(t(TextId::BeepRow), t(_settings.get(SettingId::Beep) ? TextId::On : TextId::Off), false);
            snprintf(v, sizeof v, "%d V", supplyVolts());
            add(t(TextId::SupplyRow), v, false);
            break;
        default:
            break;
    }
    add(t(TextId::Back), nullptr, false);
    return n;
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
        showMessage(_screen == Screen::HomePark ? homeLock(homeRow(_sel)) : faultText(), kMessageMs, nowMs);
        return;
    }
    clickTick();
    if (_sel == n - 1)  // Back
        return openMenu();
    switch (_screen)
    {
        case Screen::JogMenu:
            if (_sel == 2)  // the firmware's four slew rates (M4)
                push(UiAction::Kind::SetSlewRate, _m.slewRate >= 1 && _m.slewRate < 4 ? _m.slewRate + 1 : 1);
            else
            {
                _jogDec = _sel == 1;
                _screen = Screen::Jog;
            }
            break;
        case Screen::Setup:
            clickSetup(_sel);
            break;
        case Screen::HomePark:
            clickHomePark(_sel);
            break;
        case Screen::Settings:
            clickSettings(_sel);
            break;
        default:
            break;
    }
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

// M7: park and home are started here, then polled through the snapshot (PARKING / HOMING status).
void UiController::clickHomePark(uint8_t r)
{
    switch (homeRow(r))
    {
        case TextId::GoHome:
            push(UiAction::Kind::GoHome);
            _screen = Screen::Status;
            break;
        case TextId::SetHomeHere:
            _screen = Screen::SetHomeConfirm;
            break;
        case TextId::Park:
            push(UiAction::Kind::Park);
            _screen = Screen::Status;
            break;
        case TextId::Unpark:
            push(UiAction::Kind::Unpark);
            _screen = Screen::Status;
            break;
        default:
            break;
    }
}

void UiController::clickSettings(uint8_t r)
{
    const SettingId id = static_cast<SettingId>(r);
    switch (id)
    {
        case SettingId::MinAlt:
            openEditor(Edit::MinAlt, _settings.get(id));
            break;
        case SettingId::Supply:  // the row shows the firmware's value once it applied the change
        {
            const int volts = supplyVolts() == 24 ? 12 : 24;
            _settings.set(id, volts == 24);
            push(UiAction::Kind::SetSupplyVoltage, volts);
            break;
        }
        default:
            _settings.set(id, _settings.next(id));
            _lang = _settings.get(SettingId::Language) ? Lang::TR : Lang::EN;
            break;
    }
}

// ---- jog (M4) ----

void UiController::turnJog(int8_t detents, uint32_t nowMs)
{
    if (!within(_now, nowMs, kJogStopMs))  // spun while the UI was blocked: dropped
        return;
    const uint8_t dir = _jogDec ? (detents > 0 ? 1 : 4) : (detents > 0 ? 2 : 8);  // N1 E2 S4 W8, right = E / N
    if (dir != _jogDir)
    {
        jogStop();
        push(UiAction::Kind::JogStart, dir);
        _jogDir = dir;
    }
    _jogMs = nowMs;
}

void UiController::jogStop()
{
    if (_jogDir)
        push(UiAction::Kind::JogStop, _jogDir);  // the adapter stops the given directions
    _jogDir = 0;
}

TextId UiController::speedText() const
{
    static const TextId kRates[] = {TextId::SpeedSlow, TextId::SpeedCenter, TextId::SpeedFind, TextId::SpeedMax};
    return kRates[_m.slewRate >= 1 && _m.slewRate <= 4 ? _m.slewRate - 1 : 0];
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
        case Edit::GotoRa:
        case Edit::SyncRa:
            title = _edit == Edit::GotoRa ? TextId::HdrGotoRa : TextId::HdrSyncRa;
            last = TextId::FootNextBack;
            break;
        case Edit::GotoDec:
        case Edit::SyncDec:
            title = _edit == Edit::GotoDec ? TextId::HdrGotoDec : TextId::HdrSyncDec;
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
    const TextId title = _screen == Screen::JogMenu ? TextId::HdrJog
                         : _screen == Screen::Setup ? TextId::HdrSetup
                         : _screen == Screen::HomePark ? TextId::HdrHomePark
                                                       : TextId::HdrSettings;
    list(c, title, it, n, _sel < n ? _sel : n - 1);
}

void UiController::renderConfirm(Canvas &c) const
{
    char l[48], a[24], b[24];
    const bool go = _screen == Screen::GotoConfirm;
    header(c, t(go ? TextId::HdrGoto : TextId::HdrSync), t(TextId::HdrConfirm));
    hms(a, sizeof a, _ra);
    dms(b, sizeof b, _dec, true);
    if (go)
    {
        snprintf(l, sizeof l, "RA %s", a);
        c.text(Font::F6x10, 0, 13, l, false);
        snprintf(l, sizeof l, "DE %s", b);
        c.text(Font::F6x10, 0, 23, l, false);
        // S4: direction sanity check.
        snprintf(l, sizeof l, "%s%c%02d%s Az%03d%s %s%d", t(TextId::ShortAlt), _alt < 0 ? '-' : '+', _alt < 0 ? -_alt : _alt, kDeg, _az, kDeg,
                 t(TextId::ShortSun), _sun);
        c.text(Font::F6x10, 0, 33, l, false);
        if (!within(_now, _screenFrom, kArmMs))  // shown only once armed
        {
            c.box(0, 42, 127, 52);
            center(c, Font::F6x10, 43, t(TextId::ClickGo), true);
        }
        footer(c, t(TextId::FootCancel));
        return;
    }
    // M5: the offset; above 5 degrees a "BIG JUMP" frame.
    const int32_t ra = _dRa < 0 ? -_dRa : _dRa, de = _dDec < 0 ? -_dDec : _dDec;
    const bool armed = !within(_now, _screenFrom, kArmMs);  // like GO: shown only once armed
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
        if (armed)
        {
            c.box(0, 43, 127, 52);
            center(c, Font::F6x10, 43, t(TextId::ClickSync), true);
        }
    }
    else
    {
        char line[32];
        snprintf(line, sizeof line, "RA %s", a);
        c.text(Font::F6x10, 0, 13, line, false);
        snprintf(line, sizeof line, "DE %s", b);
        c.text(Font::F6x10, 0, 23, line, false);
        c.text(Font::F6x10, 0, 33, l, false);
        if (armed)
        {
            c.box(0, 42, 127, 52);
            center(c, Font::F6x10, 43, t(TextId::ClickSync), true);
        }
    }
    footer(c, t(TextId::FootCancel));
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

void UiController::renderSmallConfirm(Canvas &c, TextId hdr, TextId msg) const
{
    header(c, t(hdr), nullptr);
    center(c, Font::F7x13B, 17, t(msg), false);
    c.box(0, 36, 127, 46);
    center(c, Font::F6x10, 37, t(TextId::ClickYes), true);
    footer(c, t(TextId::FootCancel));
}

void UiController::renderJog(Canvas &c) const
{
    char l[32], a[24];
    header(c, t(_jogDec ? TextId::HdrJogDec : TextId::HdrJogRa), t(speedText()));
    if (_jogDec)
    {
        c.frame(0, 13, 61, 28);
        c.text(Font::F7x13B, 21, 14, "RA", false);
        c.box(66, 13, 127, 28);
        c.text(Font::F7x13B, 86, 14, "DEC", true);
    }
    else
    {
        c.box(0, 13, 61, 28);
        c.text(Font::F7x13B, 21, 14, "RA", true);
        c.frame(66, 13, 127, 28);
        c.text(Font::F7x13B, 86, 14, "DEC", false);
    }
    hms(a, sizeof a, _m.raSeconds);
    snprintf(l, sizeof l, "RA %s", a);
    c.text(Font::F6x10, 0, 32, l, false);
    dms(a, sizeof a, _m.decArcSec, true);
    snprintf(l, sizeof l, "DE %s", a);
    c.text(Font::F6x10, 0, 42, l, false);
    footer(c, t(_jogDec ? TextId::FootJogDec : TextId::FootJogRa));
}

}  // namespace core
