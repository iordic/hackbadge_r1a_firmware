#ifndef APP_RAW_TX_H_
#define APP_RAW_TX_H_
#include <SimpleList.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "utils/menu.h"
#include "utils/file_utils.h"
#include "tasks/radio_task.h"

typedef struct {
    String name;
    int count;
    String frequency;
    String preset;
    int size;
} RawTxFile;

void fillRawTxFilesMenu(Menu* menu, SimpleList<String>* &files);
void raw_tx_sendSignal();
void raw_tx_loadFileContent();
void removeSelectedRawTxFile();
#endif
