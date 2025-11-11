#ifndef RCSWITCH_DECODER_UTILS_H_
#define RCSWITCH_DECODER_UTILS_H_
#include <Arduino.h>
void output(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol);
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength);
static const char* bin2tristate(const char* bin);
#endif