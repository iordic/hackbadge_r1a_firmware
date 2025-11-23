#include "ui_task.h"

extern App app_splash;
App *currentApp = &app_splash;

void ui_task(void *pv) {
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