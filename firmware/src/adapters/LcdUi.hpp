#pragma once

#if USE_LCD_UI == 1

    #include "core/ui/UiController.hpp"

void lcdUiSetup();
void lcdUiReloadSettings();   // after the store was cleared (:XFR): back to defaults, nothing written back
void lcdUiUpdate();           // from the main loop after mount.loop(): knob, tick, actions, throttled frame
core::NetStatus &lcdUiNet();  // what the bridge reports over :XN, shown on Settings > Network
uint8_t lcdUiOwnWifi();       // the Own WiFi setting for :XNG: 0 Auto, 1 On, 2 Off

#endif
