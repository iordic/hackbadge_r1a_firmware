#ifndef UI_TASK_H_
#define UI_TASK_H_
#include "app.h"
#include "devices/input.h"
#include "devices/display.h"
#include "tasks/radio_task.h"
#include "tasks/leds_task.h"
#include <Arduino.h>

void ui_task(void *pv);
void sendNeopixelConfig(NeopixelConfiguration params);
#endif