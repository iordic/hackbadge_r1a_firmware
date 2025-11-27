#include "ui_task.h"
#include "tasks/leds_task.h"

extern App app_splash;
App *currentApp = &app_splash;

uint8_t ledsBrightness;

NeopixelConfiguration neopixelConfiguration;
SemaphoreHandle_t neopixelMutex;
TaskHandle_t neopixelWorkerHandle = NULL;

void ui_task(void *pv) {
  neopixelMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(leds_task, "leds_task", 4096, NULL, 1, &neopixelWorkerHandle, 0);
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

void sendNeopixelConfig(NeopixelConfiguration params) {
  xSemaphoreTake(neopixelMutex, portMAX_DELAY);
  neopixelConfiguration.brightness = params.brightness;
  for (int i = 0; i < NUM_LEDS; i++) neopixelConfiguration.colors[i] = params.colors[i];
  neopixelConfiguration.operation = params.operation;
  xSemaphoreGive(neopixelMutex);
  xTaskNotify(neopixelWorkerHandle, 1, eSetValueWithOverwrite);
}