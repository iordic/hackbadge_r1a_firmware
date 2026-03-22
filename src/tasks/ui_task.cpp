#include "ui_task.h"
#include "tasks/leds_task.h"

extern App app_splash;
App *currentApp = &app_splash;

uint8_t ledsBrightness;
boolean keyBoardOnScreen = false;
String *keyboardInputText;
Preferences prefs;

NeopixelConfiguration neopixelConfiguration;
SemaphoreHandle_t neopixelMutex;
TaskHandle_t neopixelWorkerHandle = NULL;

void ui_task(void *pv) {
  neopixelMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(leds_task, "leds_task", 4096, NULL, 1, &neopixelWorkerHandle, 0);
  U8G2 *u8g2 = display_get();
  currentApp->onStart();
  while (true) {
    if (keyBoardOnScreen) {
      keyboardInputLoop();
      continue;
    }
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

void startKeyboard(String *fieldToFill) {
  keyBoardOnScreen = true;
  keyboardInputText = fieldToFill;
  keyboard_get()->begin();
}

void keyboardInputLoop() {
  OLEDKeyboard *keyboard = keyboard_get();
  while (keyBoardOnScreen) {
    int evt = input_read();
    if (evt == BTN_BACK) {
      keyBoardOnScreen = false;
      keyboard->reset();
    } else {
      keyboard->handleInput((KeyBoardInput) evt);
      if (keyboard->update()) {
        keyBoardOnScreen = false;
        *keyboardInputText = keyboard->getInputText();
        Serial.print("Input: ");
        Serial.println(*keyboardInputText);
        keyboard->reset();
      }
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void changeAppContext(App* newApp) {
    currentApp->onStop();
    currentApp = newApp;
    currentApp->onStart();
}