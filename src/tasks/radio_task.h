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
  int frequency;
  int preset;
} RFMessage;

// --- RAW (timings crudos OOK, enfoque Evil Crow RF) ---
#define RAW_MAX_CHANGES   2000   // tope de muestras por captura (samplesize)
#define RAW_MIN_CHANGES     20   // mínimo para dar la trama por válida (MIN_SAMPLE)
#define RAW_MIN_PULSE_US   100   // pulsos por debajo = glitch: se ignoran
#define RAW_END_GAP_US  100000   // 100 ms: silencio que reinicia y cierra la captura
#define RAW_REPEAT           2   // repeticiones del burst en el replay
#define RAW_REPEAT_GAP_US 20000  // silencio entre repeticiones

typedef struct {
  int frequency;
  int preset;
  uint16_t count;      // nº de duraciones en durations
  int32_t *durations;  // µs con signo: >0 = pulso HIGH, <0 = pulso LOW; en heap
} RawSignal;

void radio_task(void *pv);
void loadConfiguration(int frequencyOption, int preset);
void lockJamming();
void radioReceiveSignal();
void replaySignal(RCSwitch *mySwitch, RFMessage msg);
void sendSignal();
void sendSignal(RCSwitch *mySwitch, RFMessage msg);
// RAW
void radioReceiveRaw();
void radioSendRaw();
void sendRaw(const RawSignal *sig);
void replayRaw(const RawSignal *sig);
void freeRawSignal(RawSignal *sig);
#endif