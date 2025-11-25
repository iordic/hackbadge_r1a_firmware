#ifndef INPUT_H_
#define INPUT_H_
#include <OneButton.h>
#include "config/io_config.h"
#include "app.h"

int input_read();
void input_init();
void ticks();
#endif