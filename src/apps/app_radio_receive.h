#ifndef APP_RADIO_RECEIVE_H_
#define APP_RADIO_RECEIVE_H_
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

void replaySignal();
void saveSignal();
void drawRssi(U8G2 *u8g2);
#endif