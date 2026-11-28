#pragma once

#if USE_LCD_UI == 1

    #include "inc/Globals.hpp"
    #include "Mount.hpp"
    #include "core/ui/UiController.hpp"
    #if SUPPLY_SENSE == 1
        #include "core/SupplySense.hpp"
    #endif

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
    #if SUPPLY_SENSE == 1
    core::SupplyFilter _supply;
    uint32_t _supplyMs = 0;  // last sample (millis)
    void sampleSupply(uint32_t nowMs);
    #endif
};

#endif
