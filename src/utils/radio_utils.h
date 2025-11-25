#ifndef RADIO_UTILS_H_
#define RADIO_UTILS_H_
#include <Arduino.h>

enum RadioOperation {
  CHECK = 0,
  START_JAMMER,
  RECEIVE_SIGNAL,
  RADIO_STOP
};

enum AvailableFrequencies {
  FREQ_315MHZ = 0,
  FREQ_433MHZ,
  FREQ_868MHZ,
  FREQ_915MHZ
};

enum PresetConfigs {
    PRESET_AM270 = 0,
    PRESET_AM650,
    PRESET_FM238,
    PRESET_FM476
};

float getFrequencyFromEnum(int freqEnum);
String getPresetNameFromEnum(int presetEnum);
void output(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol);
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength);
static const char* bin2tristate(const char* bin);
#endif