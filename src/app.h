#pragma once
#include <U8g2lib.h>

typedef struct App {
  const char *name;
  void (*onStart)();
  void (*onEvent)(int evt);
  void (*onDraw)(U8G2 *u8g2);
  void (*onStop)();
} App;

// Eventos genéricos
enum {
  BTN_NONE = 0,
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_OK,
  BTN_BACK
};
