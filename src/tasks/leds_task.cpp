#include "leds_task.h"

Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL, NEO_GRB + NEO_KHZ800);

void leds_task(void *pv) {
    int selected = 0;
    uint32_t color;
    strip.setBrightness(50);
    for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
    while (true) {
        color = selected > 0 ? strip.getPixelColor(selected - 1) : strip.Color(random(256), random(256), random(256));
        strip.setPixelColor(selected, color);
        selected = (++selected) % 2;
        strip.show();
        vTaskDelay(1000);
    }
}