#ifndef RADIO_H_
#define RADIO_H_
#include <Arduino.h>
#include <ELECHOUSE_CC1101.h>

void radio_init();
ELECHOUSE_CC1101 *radio_get();
#endif