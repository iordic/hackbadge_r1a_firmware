#include <Arduino.h>
#include <Wire.h>
#include <OneButton.h>

//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <ELECHOUSE_CC1101.h>

#include "config.h"
#include "animation.h"


Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL, NEO_GRB + NEO_KHZ800);
OneButton leftBtn, upBtn, rightBtn, downBtn, enterBtn;
ELECHOUSE_CC1101 *cc1101;

static void handleLeftClick() {
  Serial.println("Clicked left!");
}
static void handleUpClick() {
  Serial.println("Clicked up!");
}
static void handleRightClick() {
  Serial.println("Clicked right!");
}
static void handleDownClick() {
  Serial.println("Clicked down!");
}
static void handleEnterClick() {
  Serial.println("Clicked enter!");
}


void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.begin(SERIAL_BAUDRATE);
  cc1101 = new ELECHOUSE_CC1101(CC1101_GDO0, CC1101_GDO2, CC1101_SCLK, CC1101_MISO, CC1101_MOSI, CC1101_CS, FSPI);
  cc1101->init();
  // Buttons setup
  leftBtn.setup(BUTTON_LEFT, INPUT_PULLUP, true);
  upBtn.setup(BUTTON_UP, INPUT_PULLUP, true);
  rightBtn.setup(BUTTON_RIGHT, INPUT_PULLUP, true);
  downBtn.setup(BUTTON_DOWN, INPUT_PULLUP, true);
  enterBtn.setup(BUTTON_ENTER, INPUT_PULLUP, true);

  leftBtn.attachClick(handleLeftClick);
  rightBtn.attachClick(handleRightClick);
  upBtn.attachClick(handleUpClick);
  downBtn.attachClick(handleDownClick);
  enterBtn.attachClick(handleEnterClick);

  for(int i=0;i<NUM_LEDS;i++) {
    strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
  }
  strip.show();
  if (cc1101->getCC1101()) {
    Serial.println("CC1101 conected");
  } else {
    Serial.println("CC1101 not conected");
  }
  beginU8g2();
  drawSplashTexts();
}

void loop() {
  leftBtn.tick();
  upBtn.tick();
  rightBtn.tick();
  downBtn.tick();
  enterBtn.tick();
  updateAnimation();
}
