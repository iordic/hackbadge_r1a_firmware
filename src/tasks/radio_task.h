#ifndef RADIO_TASK_H_
#define RADIO_TASK_H_
#include <Arduino.h>
#include <ELECHOUSE_CC1101.h>
#include <RCSwitch.h>
#include "utils/rcswitch_decoder.h"

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

typedef struct {
  int operation; 
  int frequency;
  int preset;
  TaskHandle_t callerHandle;  // para avisar a quien devolver el resultado
  QueueHandle_t queueHandle;
} RadioTaskParams;

typedef struct {
  unsigned long value;
  unsigned int length;
  unsigned int protocol;
} RFMessage;

void radio_task(void *pv);
void loadConfiguration(int frequencyOption, int preset);
float getFrequencyFromEnum(int freqEnum);
void lockJamming();
void radioReceiveSignal();
#endif