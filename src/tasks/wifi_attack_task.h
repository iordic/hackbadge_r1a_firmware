#ifndef WIFI_ATTACK_TASK_H_
#define WIFI_ATTACK_TASK_H_
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

enum OperationCommands {
    START_BEACON_SPAM = 1,
    STOP_ATTACK
};

void wifi_attack_task(void *pv);
void beaconSpamList(const char list[]);
void beaconAttack();
void generateRandomWiFiMac(uint8_t *mac);
void wifiDisconnect();
void nextChannel();
#endif