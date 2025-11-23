#ifndef RADIO_TASK_H_
#define RADIO_TASK_H_
#include <Arduino.h>
#include <ELECHOUSE_CC1101.h>
#include <RCSwitch.h>
#include <Preferences.h>

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
void lockJamming();
void radioReceiveSignal();
#endif