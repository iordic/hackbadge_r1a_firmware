#include "app.h"
#include "app_radio_receive.h"
#include "literals.h"
#include "app.h"
#include "app_menu.h"

#include "tasks/radio_task.h"
#include "display.h"

extern App app_menu;
QueueHandle_t queue;
TaskHandle_t radioReceiverTaskHandle = NULL;
bool firstMessage;

void radio_receive_onStart() {
    firstMessage = true;
    RadioTaskParams *params = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    params->operation = RECEIVE_SIGNAL;
    params->frequency = FREQ_433MHZ;
    params->preset = PRESET_AM650;
    queue = xQueueCreate(8, sizeof(RFMessage));
    params->queueHandle = queue;
    params->callerHandle = xTaskGetCurrentTaskHandle();
    xTaskCreatePinnedToCore(radio_task, "RadioReceiverWorker", 2048, params, 5, &radioReceiverTaskHandle, 1);
}
void radio_receive_onStop() {
    xTaskNotify(radioReceiverTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
}

void radio_receive_onDraw(U8G2 *u8g2) {
    // messy and shitty code for testing, TODO: implement properly
    RFMessage msg;
    if (firstMessage) {
        u8g2->clearBuffer();
        u8g2->setDrawColor(1);
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(10, 10, "Rx - 433MHz - AM 650");
        u8g2->drawStr(10, 20, "Listening for signals...");
        u8g2->drawStr(10, 60, "Press BACK to stop");
        u8g2->sendBuffer();
        firstMessage = false;
    }
    if (xQueueReceive(queue, &msg, 0) == pdTRUE) {
        u8g2->clearBuffer();
        u8g2->setDrawColor(1);
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(10, 10, "Rx - 433MHz - AM 650");
        u8g2->drawStr(10, 20, "Signal received:");
        u8g2->drawStr(10, 30, String("Value: " + String(msg.value, HEX)).c_str());
        u8g2->drawStr(10, 40, String("Length: " + String(msg.length)).c_str());
        u8g2->drawStr(10, 50, String("Protocol: " + String(msg.protocol)).c_str());
        u8g2->drawStr(10, 60, "Press BACK to stop");
        u8g2->sendBuffer();
    } 
}
void radio_receive_onEvent(int evt) {
    if (evt == BTN_BACK) {
        radio_receive_onStop();
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
    }
}

App app_radio_receive = {
  .name = "Radio Receive",
  .onStart = radio_receive_onStart,
  .onEvent = radio_receive_onEvent,
  .onDraw = radio_receive_onDraw,
  .onStop = radio_receive_onStop
};