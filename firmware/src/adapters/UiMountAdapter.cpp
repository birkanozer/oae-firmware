#ifdef ARDUINO
    #include "../../Configuration.hpp"
#endif

#if USE_LCD_UI == 1

    #include "UiMountAdapter.hpp"
    #include "inc/Globals.hpp"

static constexpr uint32_t kJogResumeMs = 500;

void UiMountAdapter::begin()
{
    // Unplanned reset: watchdog, software or low-power reset. Power-on and the reset pin are a normal start.
    // (The F1 has no brown-out flag: a brown-out reads as power-on.) Cleared so the next boot sees only its own cause.
    _rebooted = (RCC->CSR & (RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_SFTRSTF | RCC_CSR_LPWRRSTF)) != 0;
    RCC->CSR |= RCC_CSR_RMVF;
}

core::MountState UiMountAdapter::state()
{
    using core::MountState;
    if (_mount.isFindingHome())
        return MountState::Homing;
    if (_mount.isParkPending() || _mount.isParking())
        return MountState::Parking;
    // A UI "Go home" is a slew to the home position: report it as Homing until the axes stop (then the latch drops).
    if (_goingHome && !_mount.isSlewingRAorDEC())
        _goingHome = false;
    if (_goingHome)
        return MountState::Homing;
    // A UI jog (keys held, plus the deceleration tail after release) is a jog, not a GoTo: Slewing->Tracking would beep ARRIVED.
    // The mount only sets STATUS_SLEWING_MANUAL from setManualSlewMode, which startSlewing(direction) does not call.
    // After a jog from tracking, upstream restarts tracking when the axes stop: hold Jogging until it does (at most
    // kJogResumeMs), or the UI would see Idle for a poll and flip the menu row 0 label twice.
    if (_jogTail && _jogMask == 0 && !_mount.isSlewingRAorDEC())
    {
        const uint32_t now = millis();
        if (_jogStoppedAt == 0)
            _jogStoppedAt = now ? now : 1;
        if (!_jogResumeTrack || _mount.isTracking() || now - _jogStoppedAt > kJogResumeMs)
        {
            _jogTail      = false;
            _jogStoppedAt = 0;
        }
    }
    if (_jogMask != 0 || _jogTail || (_mount.mountStatus() & STATUS_SLEWING_MANUAL))
        return MountState::Jogging;
    // AZ and Focus step moves (Adjust page) run on their own steppers: count them as moving so a click is STOP.
    if (_mount.isAxisRunning(AZIMUTH_STEPS) || _mount.isAxisRunning(FOCUS_STEPS))
        return MountState::Jogging;
    if (_mount.isSlewingRAorDEC())
        return MountState::Slewing;
    if (_mount.isTracking())
        return MountState::Tracking;
    return _parked ? MountState::Parked : MountState::Idle;
}

void UiMountAdapter::fill(core::MountSnapshot &s)
{
    using core::MountState;
    // Mount has no parked state (parked and idle are both status 0): parked = a park that ended without new motion
    MountState st = state();
    if (st == MountState::Parking)
        _wasParking = true;
    else if (st == MountState::Idle && _wasParking)
        _parked = true;
    if (st != MountState::Parking)
        _wasParking = false;
    if (st != MountState::Idle && st != MountState::Parked)
        _parked = false;
    if (st == MountState::Idle && _parked)
        st = MountState::Parked;
    if (st != MountState::Slewing)
        _hasTarget = false;

    s.state           = st;
    s.raSeconds       = _mount.currentRA().getTotalSeconds();
    s.decArcSec       = core::Declination::axisToCelestialSeconds(_mount.currentDEC().getTotalSeconds(), inNorthernHemisphere);
    s.targetRaSeconds = _mount.targetRA().getTotalSeconds();
    s.targetDecArcSec = core::Declination::axisToCelestialSeconds(_mount.targetDEC().getTotalSeconds(), inNorthernHemisphere);
    s.hasTarget       = _hasTarget;
    s.lstSeconds      = _mount.calculateLst().getTotalSeconds();
    s.localSeconds    = _mount.getLocalTime().getTotalSeconds();
    const LocalDate d = _mount.getLocalDate();
    s.year            = d.year;
    s.month           = d.month;
    s.day             = d.day;
    s.utcOffsetHours  = _mount.getLocalUtcOffset();  // east-positive on both sides (the minus is LX200 wire convention only)
    s.latArcSec       = _mount.latitude().getTotalSeconds();
    s.lonArcSec       = _mount.longitude().getTotalSeconds();
    s.slewRate        = _mount.getSlewRate();
    s.dateSet         = _mount.isDateSet();
    s.timeSet         = _mount.isTimeSet();
    s.rebooted        = _rebooted;
    s.synced          = _synced;
    s.faultCode       = _mount.driverFault();
    s.supplyVoltage   = _mount.supplyVoltage();
    s.hasAz           = AZ_STEPPER_TYPE != STEPPER_TYPE_NONE;
    s.hasFocus        = FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE;
    s.azFault         = _mount.azDriverFailed();
    s.focusFault      = _mount.focusDriverFailed();
    s.axisPos[0]      = _mount.getCurrentStepperPosition(RA_STEPS);
    s.axisPos[1]      = _mount.getCurrentStepperPosition(DEC_STEPS);
    s.axisPos[2]      = _mount.getCurrentStepperPosition(AZIMUTH_STEPS);
    s.axisPos[3]      = _mount.getCurrentStepperPosition(FOCUS_STEPS);
    s.stepsPerDeg[0]  = _mount.getStepsPerDegree(RA_STEPS);
    s.stepsPerDeg[1]  = _mount.getStepsPerDegree(DEC_STEPS);
    s.stepsPerDeg[2]  = s.hasAz ? _mount.getStepsPerDegree(AZIMUTH_STEPS) : 0.0f;
}

void UiMountAdapter::apply(const core::UiAction &a)
{
    using Kind = core::UiAction::Kind;
    switch (a.kind)
    {
        case Kind::Stop:
            _goingHome = false;
            _jogMask = 0;
            _jogTail = false;
            _mount.stopAll();  // stays stopped: the arrival block does not restart tracking
            _mount.stopSlewing(AZIMUTH_STEPS);  // stopAll() only covers RA, DEC and tracking
            _mount.stopSlewing(FOCUS_STEPS);
            break;
        case Kind::TrackOn:
            _goingHome = false;
            _jogMask = 0;
            _jogTail = false;
            _mount.startSlewing(TRACKING);
            break;
        case Kind::TrackOff:
            _mount.stopSlewing(TRACKING);
            break;
        case Kind::JogStart:  // direction bits N1 E2 S4 W8 equal the mount's
            _goingHome = false;
            if (_jogMask == 0 && !_jogTail)
                _jogResumeTrack = _mount.isTracking();  // startSlewing(direction) stops tracking: ask first
            _mount.startSlewing(a.a);
            if (!_mount.isParking())
            {
                _jogMask |= a.a & ALL_DIRECTIONS;
                if (!_jogResumeTrack)
                    _mount.suppressTrackingOnArrival();  // not tracking before the jog: stay idle after it
            }
            break;
        case Kind::JogStop:
            _mount.stopSlewing(a.a);
            _jogMask &= ~a.a;
            _jogStoppedAt = 0;
            if (_jogMask == 0)
                _jogTail = true;  // still Jogging until the axes stop
            break;
        case Kind::SetSlewRate:
            _mount.setSlewRate(a.a);
            break;
        case Kind::Goto:
            _goingHome = false;
            _jogMask = 0;
            _jogTail = false;
            _mount.targetRA().set(a.a / 3600, a.a / 60 % 60, a.a % 60);
            _mount.targetDEC() = Declination::fromCelestialSeconds(a.b);
            _mount.startSlewingToTarget();
            _hasTarget = true;
            break;
        case Kind::Sync:
            _mount.syncPosition(DayTime(a.a / 3600, a.a / 60 % 60, a.a % 60), Declination::fromCelestialSeconds(a.b));
            _synced = true;
            break;
        case Kind::SetDate:
            _mount.setLocalStartDate(a.a / 10000, a.a / 100 % 100, a.a % 100);
            _mount.applyPendingHomeReference();
            _synced = false;  // the UI has already asked "Clears Sync. Sure?"
            break;
        case Kind::SetTime:
            _mount.setLocalStartTime(DayTime(a.a / 3600, a.a / 60 % 60, a.a % 60));
            _mount.applyPendingHomeReference();
            _synced = false;
            break;
        case Kind::SetUtcOffset:
            _mount.setLocalUtcOffset(a.a);
            _mount.applyPendingHomeReference();
            _synced = false;
            break;
        case Kind::SetLatitude:
        {
            Latitude v;
            v.addSeconds(a.a);
            _mount.setLatitude(v);
            break;
        }
        case Kind::SetLongitude:
        {
            Longitude v;
            v.addSeconds(a.a);
            _mount.setLongitude(v);
            _mount.applyPendingHomeReference();
            _synced = false;
            break;
        }
        case Kind::GoHome:
            _jogMask = 0;
            _jogTail = false;
            _mount.stopSlewing(TRACKING);  // also when RA is already at home and no slew follows
            _mount.startSlewingToHome();
            _mount.suppressTrackingOnArrival();  // after a UI home the mount is idle, not tracking (startSlewingToHome clears the flag)
            _goingHome = true;
            break;
        case Kind::SetHome:  // the UI offers it only when idle: setHome() waits for the axes
            _mount.setHomeHere();
            _synced = false;
            break;
        case Kind::MoveSteps:  // same call as the LX200 :MX command
        {
            static const StepperAxis axes[4] = {RA_STEPS, DEC_STEPS, AZIMUTH_STEPS, FOCUS_STEPS};
            if (a.b >= 0 && a.b < 4)
            {
                _goingHome = false;
                _mount.moveStepperBy(axes[a.b], a.a);
            }
            break;
        }
        case Kind::Park:
            _goingHome = false;
            _jogMask = 0;
            _jogTail = false;
            _mount.startPark();
            break;
        case Kind::Unpark:
            _goingHome = false;
            _jogMask = 0;
            _jogTail = false;
            _mount.startSlewing(TRACKING);
            break;
        case Kind::SetSupplyVoltage:
            _mount.applySupplyVoltage(static_cast<uint8_t>(a.a));
            break;
        default:  // Beep, BeepError, BeepLong
            break;
    }
}

#endif
