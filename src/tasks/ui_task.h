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
// Atajos para los dos usos habituales: color fijo en todos los LEDs, o "reposo"
// (animación aleatoria con colores a 0). Evitan repetir el bucle de relleno.
void sendNeopixelSolid(uint32_t color, uint8_t brightness);
void sendNeopixelIdle(uint8_t brightness);
void startKeyboard(String *fieldToFill);
void keyboardInputLoop();
void changeAppContext(App* newApp);
#endif