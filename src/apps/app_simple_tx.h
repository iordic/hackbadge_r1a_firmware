#ifndef APP_SIMPLE_TX_H_
#define APP_SIMPLE_TX_H_
#include <SimpleList.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <vector>
#include "utils/menu.h"
#include "utils/file_utils.h"
#include "tasks/radio_task.h"


void fillSimpleTxFilesMenu(Menu* menu, SimpleList<String>* &files);
void loadRFMessageFromFile(String fileName, RFMessage* msg);
void simple_tx_sendSignal();
void simple_tx_sendSignal(String fileName);
#endif