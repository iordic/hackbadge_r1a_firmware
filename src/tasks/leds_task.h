#ifndef LEDS_TASK_H_
#define LEDS_TASK_H_
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include "config/io_config.h"

enum {
    RANDOM_ALL,
    RANDOM_SEQUENTIAL,
    FIXED_COLOR
};

typedef struct {
    uint8_t operation;
    uint8_t brightness;
    uint32_t colors[NUM_LEDS];
} NeopixelConfiguration;

void leds_task(void *pv);
#endif