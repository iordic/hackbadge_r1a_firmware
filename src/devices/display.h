#ifndef DISPLAY_H_
#define DISPLAY_H_
#include <U8g2lib.h>
#include <OLEDKeyboard.h>

void display_init();
U8G2 *display_get();
OLEDKeyboard *keyboard_get();
#endif