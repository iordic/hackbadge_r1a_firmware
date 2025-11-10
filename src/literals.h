#include <stdint.h>
#include <Arduino.h>

// Menu item icons
const uint16_t WAVE_ICON PROGMEM = 0x0055;
const uint16_t BLUETOOTH_ICON PROGMEM = 0x005e;
const uint16_t WIFI_ICON PROGMEM = 0x00f7;
const uint16_t PUZZLE_ICON PROGMEM = 0x00ef;
const uint16_t WRENCH_ICON PROGMEM = 0x011a;
const uint16_t GEAR_ICON PROGMEM = 0x0081;
const uint16_t UP_ICON PROGMEM = 0x0052;
const uint16_t DOWN_ICON PROGMEM = 0x004f;
const uint16_t CURSOR_DOWN_ICON PROGMEM = 0x0042;
const uint16_t ERROR_ICON PROGMEM = 0x0079;

// Menu item labels
const char MENU_ITEM_SUBGHZ[] PROGMEM = "Subghz";
const char MENU_ITEM_BLE[] PROGMEM = "BLE";
const char MENU_ITEM_WIFI[] PROGMEM = "WiFi";
const char MENU_ITEM_GAMES[] PROGMEM = "Games";
const char MENU_ITEM_SETTINGS[] PROGMEM = "Settings";
const char MENU_ITEM_RADIO_NOT_FOUND[] PROGMEM = "cc1101 not found";
const char MENU_ITEM_CONFIG[] PROGMEM = "Config";
const char MENU_ITEM_TRANSMIT[] PROGMEM = "Transmit";
const char MENU_ITEM_RECEIVE[] PROGMEM = "Receive";
const char MENU_ITEM_SNAKE[] PROGMEM = "Snake";
