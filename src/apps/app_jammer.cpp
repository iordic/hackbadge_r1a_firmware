#include "literals.h"
#include "app.h"
#include "app_jammer.h"
#include "devices/display.h"
#include "app_menu.h"

#include "tasks/ui_task.h"
#include "tasks/leds_task.h"
#include "tasks/radio_task.h"
#include "utils/menu.h"
#include "utils/radio_utils.h"
#include "config/sprites.h"
#include "config/constants.h"

extern App app_menu;
TaskHandle_t jammerTaskHandle = NULL;
float jammer_frequency;
int preset;
extern Preferences prefs;
extern uint8_t ledsBrightness;

void jammer_onStart() {
  prefs.begin("configuration", true);
  RadioTaskParams *params = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
  params->operation = START_JAMMER;
  params->callerHandle = xTaskGetCurrentTaskHandle();
  params->frequency = prefs.getUChar("frequency", FREQ_433MHZ);
  params->preset = prefs.getUChar("preset", PRESET_AM650);
  jammer_frequency = getFrequencyFromEnum(params->frequency);
  preset = params->preset;
  xTaskCreatePinnedToCore(radio_task, "RadioJammerWorker", 2048, params, 1, &jammerTaskHandle, 1);
  ledsBrightness = prefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
  sendNeopixelSolid(0x00ff0000, ledsBrightness);   // rojo: jammer activo
}

void jammer_onStop() {
  xTaskNotify(jammerTaskHandle, 1, eSetValueWithOverwrite);
  sendNeopixelIdle(ledsBrightness);
  prefs.end();
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
  u8g2->drawXBM(0, 12, bat_tx_width, bat_tx_height, bat_tx_bits);
  u8g2->setFont(u8g2_font_7x14_tr);
  u8g2->drawStr(40, 10, "Jamming at");
  char buf[8];
  dtostrf(jammer_frequency, 4, 3, buf);
  u8g2->drawStr(55, 25, buf);
  u8g2->drawStr(105, 25, "MHz");
  u8g2->drawStr(65, 40, getPresetNameFromEnum(preset).c_str());
  u8g2->setFont(u8g2_font_tiny5_tr);
  u8g2->drawButtonUTF8(80, 55, U8G2_BTN_BW2, 0,  2,  2, "Hold to exit");
  u8g2->setFontMode(0);   // drawButtonUTF8 deja el modo en transparente: lo restauramos
  u8g2->sendBuffer();
}

App app_jammer = {
  .name = "Jammer",
  .onStart = jammer_onStart,
  .onEvent = jammer_onEvent,
  .onDraw = jammer_onDraw,
  .onStop = jammer_onStop
};