#include <Arduino.h>
#include <Wire.h>
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>
#include <ELECHOUSE_CC1101.h>

#include "config.h"
#include "tasks/ui_task.h"


Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL, NEO_GRB + NEO_KHZ800);
ELECHOUSE_CC1101 *cc1101;

void setup() {
  display_init();
  input_init();
  xTaskCreatePinnedToCore(ui_task, "ui_task", 4096, NULL, 1, NULL, 0);
  Serial.begin(SERIAL_BAUDRATE);
  cc1101 = new ELECHOUSE_CC1101(CC1101_GDO0, CC1101_GDO2, CC1101_SCLK, CC1101_MISO, CC1101_MOSI, CC1101_CS, FSPI);
  cc1101->init();

  for(int i=0;i<NUM_LEDS;i++) {
    strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
  }
  strip.show();
  if (cc1101->getCC1101()) {
    Serial.println("CC1101 conected");
  } else {
    Serial.println("CC1101 not conected");
  }
}

void loop() {

}
