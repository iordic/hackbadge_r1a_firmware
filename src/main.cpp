#include <Arduino.h>
#include <Wire.h>
#include <OneButton.h>

#include "config/io_config.h"
#include "tasks/ui_task.h"
#include "devices/radio.h"


void setup() {
  display_init();
  radio_init();
  input_init();
  xTaskCreatePinnedToCore(ui_task, "ui_task", 4096, NULL, 1, NULL, 0);
  Serial.begin(SERIAL_BAUDRATE);
}

void loop() {

}
