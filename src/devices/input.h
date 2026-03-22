#ifndef INPUT_H_
#define INPUT_H_
#include "config/io_config.h"
#include "app.h"

#ifdef USE_INPUT_PULLUP
#define INPUT_TYPE_BUTTON INPUT_PULLUP
#else
#define INPUT_TYPE_BUTTON INPUT
#endif

#define REPEAT_INTERVAL 200
#define LONG_PRESS_INTERVAL 800

int input_read();
void input_init();
void ticks();
#endif