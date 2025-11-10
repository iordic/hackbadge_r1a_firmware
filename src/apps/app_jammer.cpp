#include "literals.h"
#include "app.h"
#include "app_jammer.h"
#include "display.h"
#include "app_menu.h"

#include "tasks/radio_task.h"
#include "utils/menu.h"

extern App app_menu;
TaskHandle_t jammerTaskHandle = NULL;

void jammer_onStart() {
  RadioTaskParams *params = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
  params->operation = START_JAMMER;
  params->callerHandle = xTaskGetCurrentTaskHandle();
  // TODO: implement load & save variables with Preferences library: https://github.com/vshymanskyy/Preferences
  params->frequency = FREQ_433MHZ;
  params->preset = PRESET_AM650;
  xTaskCreatePinnedToCore(radio_task, "RadioJammerWorker", 2048, params, 1, &jammerTaskHandle, 1);
}

void jammer_onStop() {
  xTaskNotify(jammerTaskHandle, 1, eSetValueWithOverwrite);
}

void jammer_onEvent(int evt) {
  if (evt == BTN_BACK) {
        jammer_onStop();
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
  }
}

void jammer_onDraw(U8G2 *u8g2) {
  u8g2->clearBuffer();
  u8g2->setDrawColor(1);
  u8g2->setFont(u8g2_font_7x14_tr);
  u8g2->drawStr(10, 30, "Jammer running...");
  u8g2->sendBuffer();
}

App app_jammer = {
  .name = "Jammer",
  .onStart = jammer_onStart,
  .onEvent = jammer_onEvent,
  .onDraw = jammer_onDraw,
  .onStop = jammer_onStop
};