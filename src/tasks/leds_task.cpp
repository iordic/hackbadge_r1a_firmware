#include "leds_task.h"

Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL, NEO_GRB + NEO_KHZ800);

extern Preferences prefs;
extern NeopixelConfiguration neopixelConfiguration;
extern SemaphoreHandle_t neopixelMutex;
BaseType_t xNeopixelResult;

void leds_task(void *pv) {
    uint32_t ledsNotificationValue;
    NeopixelConfiguration* config;
    uint32_t color;
    uint32_t colors[NUM_LEDS];
    int mode = RANDOM_ALL, selected = 0;
    prefs.begin("configuration");
    uint8_t brightness = prefs.getUChar("brightness", 5);
    while (true) {
        strip.setBrightness(255 * brightness / 10);
        switch (mode) {
        case RANDOM_ALL:
            for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
            break;
        case RANDOM_SEQUENTIAL:
            color = selected > 0 ? strip.getPixelColor(selected - 1) : strip.Color(random(256), random(256), random(256));
            strip.setPixelColor(selected, color);
            selected = (++selected) % NUM_LEDS;
        case FIXED_COLOR:
            for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, colors[i]);
            break;
        default:
            break;
        }
        strip.show();
        vTaskDelay(500);
        xNeopixelResult = xTaskNotifyWait(0, 0, &ledsNotificationValue, 0);
        // load values
        if (xNeopixelResult == pdTRUE) {
            xSemaphoreTake(neopixelMutex, portMAX_DELAY);
            mode = neopixelConfiguration.operation;
            brightness = neopixelConfiguration.brightness;
            for (int i = 0; i < NUM_LEDS; i++) colors[i] = neopixelConfiguration.colors[i];
            xSemaphoreGive(neopixelMutex);
        }
        vTaskDelay(500);
    }
}