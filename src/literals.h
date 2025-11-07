#include <stdint.h>
#include <Arduino.h>

// Menu item icons
const uint16_t WAVE_ICON PROGMEM = 0x0055;
const uint16_t BLUETOOTH_ICON PROGMEM = 0x005e;
const uint16_t WIFI_ICON PROGMEM = 0x00f7;
const uint16_t PUZZLE_ICON PROGMEM = 0x00ef;
const uint16_t WRENCH_ICON PROGMEM = 0x011a;

// Menu item labels
const char MENU_ITEM_SUBGHZ[] PROGMEM = "Subghz";
const char MENU_ITEM_BLE[] PROGMEM = "BLE";
const char MENU_ITEM_WIFI[] PROGMEM = "WiFi";
const char MENU_ITEM_GAMES[] PROGMEM = "Games";
const char MENU_ITEM_SETTINGS[] PROGMEM = "Settings";
