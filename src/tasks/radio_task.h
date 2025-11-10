#ifndef RADIO_TASK_H_
#define RADIO_TASK_H_
#include <Arduino.h>
#include <ELECHOUSE_CC1101.h>

enum RadioOperation {
  CHECK = 0,
  START_JAMMER,
  STOP_JAMMER,
  TX_MODE,
  RX_MODE
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
} RadioTaskParams;

void radio_task(void *pv);
void loadConfiguration(int frequencyOption, int preset);
float getFrequencyFromEnum(int freqEnum);
void lockJamming();
#endif