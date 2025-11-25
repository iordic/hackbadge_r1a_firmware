#include "literals.h"
#include "app.h"
#include "devices/display.h"
#include "app_menu.h"
#include "app_wifi_beacon_spam.h"
#include "tasks/wifi_attack_task.h"

extern App app_menu;

TaskHandle_t beaconSpamTaskHandle = NULL;

void wifi_beacon_spam_onStart() {
  xTaskCreatePinnedToCore(wifi_attack_task, "beacon_spam_task", 4096, NULL, 1, &beaconSpamTaskHandle, 0);
}

void wifi_beacon_spam_onStop() {
  xTaskNotify(beaconSpamTaskHandle, STOP_ATTACK, eSetValueWithOverwrite);
}

void wifi_beacon_spam_onEvent(int evt) {
  if (evt == BTN_BACK) {
        wifi_beacon_spam_onStop();
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
  }
}

void wifi_beacon_spam_onDraw(U8G2 *u8g2) {
  u8g2->clearBuffer();
  u8g2->setDrawColor(1);
  u8g2->setFont(u8g2_font_7x14_tr);
  u8g2->drawStr(10, 10, "WiFi Beacon Spam");
  u8g2->drawStr(10, 50, "Press BACK to return");
  u8g2->sendBuffer();
}

App app_wifi_beacon_spam = {
  .name = "WiFi Beacon Spam",
  .onStart = wifi_beacon_spam_onStart,
  .onEvent = wifi_beacon_spam_onEvent,
  .onDraw = wifi_beacon_spam_onDraw,
  .onStop = wifi_beacon_spam_onStop
};