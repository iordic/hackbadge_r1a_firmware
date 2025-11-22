#include "ui_task.h"

extern App app_splash;
App *currentApp = &app_splash;

Preferences prefs;
uint8_t frequencySelectedConfig;
uint8_t radioPresetConfig;

void ui_task(void *pv) {
  prefs.begin("configuration");
  // Default config if not saved values: 433MHz - OOK - 650Khz bw
  frequencySelectedConfig = prefs.getInt("frequency", FREQ_433MHZ);
  radioPresetConfig = prefs.getInt("preset", PRESET_AM650);
  U8G2 *u8g2 = display_get();
  currentApp->onStart();
  
  while (true) {
    int evt = input_read();
    if (evt != BTN_NONE) {
      currentApp->onEvent(evt); 
    }
    currentApp->onDraw(u8g2);
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}