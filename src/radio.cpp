#include "radio.h"
#include "config/io_config.h"

ELECHOUSE_CC1101 *_cc1101;


void radio_init() {
    pinMode(CC1101_GDO2, OUTPUT);
    _cc1101 = new ELECHOUSE_CC1101(CC1101_GDO0, CC1101_GDO2, CC1101_SCLK, CC1101_MISO, CC1101_MOSI, CC1101_CS, FSPI);
}

ELECHOUSE_CC1101* radio_get() {
    return _cc1101;
}