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
RadioTaskParams *receiverParams;

void radio_receive_onStart() {
    firstMessage = true;
    receiverParams = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    receiverParams->operation = RECEIVE_SIGNAL;
    receiverParams->frequency = FREQ_433MHZ;
    receiverParams->preset = PRESET_AM650;
    queue = xQueueCreate(8, sizeof(RFMessage));
    receiverParams->queueHandle = queue;
    receiverParams->callerHandle = xTaskGetCurrentTaskHandle();
    xTaskCreatePinnedToCore(radio_task, "RadioReceiverWorker", 2048, receiverParams, 5, &radioReceiverTaskHandle, 1);
}
void radio_receive_onStop() {
    xTaskNotify(radioReceiverTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
}

void radio_receive_onDraw(U8G2 *u8g2) {
    // messy and shitty code for testing, TODO: implement properly
    RFMessage msg;
    String header = "Rx on " + String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz " + getPresetNameFromEnum(receiverParams->preset);
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    u8g2->setFont(u8g2_font_t0_11_tr);
    u8g2->drawStr(0, 10, header.c_str());
    u8g2->drawHLine(0, 10, 128);
    if (firstMessage) {
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(0, 25, "Listening for signals...");
        u8g2->sendBuffer();
        firstMessage = false;
    }
    if (xQueueReceive(queue, &msg, 0) == pdTRUE) {
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(10, 20, "Signal received:");
        u8g2->drawStr(10, 30, String("Value: " + String(msg.value, HEX)).c_str());
        u8g2->drawStr(10, 40, String("Length: " + String(msg.length)).c_str());
        u8g2->drawStr(10, 50, String("Protocol: " + String(msg.protocol)).c_str());
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