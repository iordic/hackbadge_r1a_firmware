#include "literals.h"
#include "app.h"
#include "app_jammer.h"
#include "display.h"
#include "app_menu.h"

#include "tasks/radio_task.h"
#include "utils/menu.h"

extern App app_menu;
TaskHandle_t jammerTaskHandle = NULL;
float jammer_frequency;
int preset;

void jammer_onStart() {
  RadioTaskParams *params = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
  params->operation = START_JAMMER;
  params->callerHandle = xTaskGetCurrentTaskHandle();
  // TODO: implement load & save variables with Preferences library: https://github.com/vshymanskyy/Preferences
  params->frequency = FREQ_433MHZ;
  params->preset = PRESET_AM650;
  jammer_frequency = getFrequencyFromEnum(params->frequency);
  preset = params->preset;
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
  u8g2->drawStr(10, 10, "Jammer running...");
  u8g2->drawStr(10, 20, "Freq: ");
  char buf[8];
  dtostrf(jammer_frequency, 4, 3, buf);
  u8g2->drawStr(50, 20, buf);
  u8g2->drawStr(10, 35, String("Preset: " + getPresetString(preset)).c_str());
  u8g2->drawStr(10, 50, "Press BACK to stop");
  u8g2->sendBuffer();
}

String getPresetString(int preset) {
  switch (preset) {
    case PRESET_AM270:
      return "AM 270";
    case PRESET_AM650:
      return "AM 650";
    case PRESET_FM238:
      return "FM 238";
    case PRESET_FM476:
      return "FM 476";
    default:
      return "Custom";
  }
}


App app_jammer = {
  .name = "Jammer",
  .onStart = jammer_onStart,
  .onEvent = jammer_onEvent,
  .onDraw = jammer_onDraw,
  .onStop = jammer_onStop
};