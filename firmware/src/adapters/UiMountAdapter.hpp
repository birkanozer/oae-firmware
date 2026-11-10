#pragma once

#if USE_LCD_UI == 1

    #include "inc/Globals.hpp"
    #include "Mount.hpp"
    #include "core/ui/UiController.hpp"

// Translates between Mount and the pure UI: fills the snapshot, applies the UI's actions with the
// same Mount calls the LX200 handlers use. No decisions here.
class UiMountAdapter
{
  public:
    explicit UiMountAdapter(Mount &mount) : _mount(mount)
    {
    }
    void begin();  // reads and clears the reset-cause flags
    void fill(core::MountSnapshot &s);
    void apply(const core::UiAction &a);  // mount actions only; beeps are the Knob's

  private:
    core::MountState state();

    Mount &_mount;
    bool _rebooted        = false;
    bool _hasTarget       = false;
    bool _synced          = false;
    bool _wasParking      = false;
    bool _parked          = false;
    bool _goingHome       = false;  // UI "Go home" running: reported as Homing until the axes stop
    uint8_t _jogMask      = 0;      // directions of UI jogs held
    bool _jogTail         = false;  // released, axes still decelerating
    bool _jogResumeTrack  = false;  // was tracking before the jog
    uint32_t _jogStoppedAt = 0;     // millis() when the axes first stopped in the tail, 0 = not yet
};

#endif
