#include <stdint.h>
#include <Arduino.h>

// Menu item icons
const uint16_t WAVE_ICON PROGMEM = 0x0055;
const uint16_t BLUETOOTH_ICON PROGMEM = 0x005e;
const uint16_t WIFI_ICON PROGMEM = 0x00f7;
const uint16_t PUZZLE_ICON PROGMEM = 0x00ef;
const uint16_t WRENCH_ICON PROGMEM = 0x011a;
const uint16_t UP_ICON PROGMEM = 0x0052;
const uint16_t DOWN_ICON PROGMEM = 0x004f;
const uint16_t CURSOR_DOWN_ICON PROGMEM = 0x0042;
const uint16_t ERROR_ICON PROGMEM = 0x0079;
const uint16_t MEGAPHONE_ICON PROGMEM = 0x0069;
const uint16_t RGB_ICON PROGMEM = 0x0047;
const uint16_t RADIO_ICON PROGMEM = 0x0054;
const uint16_t SPAM_ICON PROGMEM = 0x009d;
const uint16_t FORBIDDEN_ICON PROGMEM = 0x0057;

// Menu item labels
const char MENU_ITEM_SUBGHZ[] PROGMEM = "Subghz";
const char MENU_ITEM_BLE[] PROGMEM = "BLE";
const char MENU_ITEM_WIFI[] PROGMEM = "WiFi";
const char MENU_ITEM_GAMES[] PROGMEM = "Games";
const char MENU_ITEM_SETTINGS[] PROGMEM = "Settings";
const char MENU_ITEM_RADIO_NOT_FOUND[] PROGMEM = "cc1101 not found";
const char MENU_ITEM_TRANSMIT[] PROGMEM = "Transmit";
const char MENU_ITEM_RECEIVE[] PROGMEM = "Receive";
const char MENU_ITEM_SNAKE[] PROGMEM = "Snake";
const char MENU_ITEM_JAMMER[] PROGMEM = "Jammer";
const char MENU_ITEM_RADIO[] PROGMEM = "Radio";
const char MENU_ITEM_RGB[] PROGMEM = "RGB Lights";
const char MENU_ITEM_WIFI_BEACON_SPAM[] PROGMEM = "Beacon Spam";
const char MENU_ITEM_NOT_IMPLEMENTED[] PROGMEM = "WIP. Coming soon!";

// Rickroll SPAM Backon SSIDs
const char rickrollssids[] PROGMEM = {"01 Never gonna give you up\n"
                                      "02 Never gonna let you down\n"
                                      "03 Never gonna run around\n"
                                      "04 and desert you\n"
                                      "05 Never gonna make you cry\n"
                                      "06 Never gonna say goodbye\n"
                                      "07 Never gonna tell a lie\n"
                                      "08 and hurt you\n"};