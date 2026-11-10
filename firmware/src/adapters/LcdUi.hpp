#pragma once

#if USE_LCD_UI == 1

void lcdUiSetup();
void lcdUiUpdate();  // from the main loop after mount.loop(): knob, tick, actions, throttled frame

#endif
