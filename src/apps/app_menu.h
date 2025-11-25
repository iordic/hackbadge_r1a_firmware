#ifndef APP_MENU_H_
#define APP_MENU_H_

#include <Arduino.h>
#include <functional>
#include <SimpleList.h>

void saveRadioConfig();
void saveNeopixelConfig();
void showPopupMenu(const char* message);
#endif