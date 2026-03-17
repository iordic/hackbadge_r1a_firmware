#include "app.h"
#include "app_radio_receive.h"
#include "literals.h"
#include "app.h"
#include "app_menu.h"

#include "tasks/ui_task.h"
#include "tasks/leds_task.h"
#include "tasks/radio_task.h"
#include "devices/display.h"
#include "utils/radio_utils.h"
#include "config/sprites.h"

extern App app_menu;
QueueHandle_t queue;
TaskHandle_t radioReceiverTaskHandle = NULL;
bool emptyList;
RadioTaskParams *receiverParams;
extern Preferences prefs;
extern uint8_t ledsBrightness;

void radio_receive_onStart() {
    emptyList = true;
    receiverParams = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    receiverParams->operation = RECEIVE_SIGNAL;
    receiverParams->frequency = prefs.getUChar("frequency", FREQ_433MHZ);
    receiverParams->preset = prefs.getUChar("preset", PRESET_AM650);
    queue = xQueueCreate(8, sizeof(RFMessage));
    receiverParams->queueHandle = queue;
    receiverParams->callerHandle = xTaskGetCurrentTaskHandle();
    ledsBrightness = prefs.getUChar("brightness");
    sendNeopixelConfig(NeopixelConfiguration{FIXED_COLOR, ledsBrightness, {0x000000ff,0x000000ff,0x000000ff,0x000000ff}});
    xTaskCreatePinnedToCore(radio_task, "RadioReceiverWorker", 2048, receiverParams, 5, &radioReceiverTaskHandle, 1);
}
void radio_receive_onStop() {
    xTaskNotify(radioReceiverTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    sendNeopixelConfig(NeopixelConfiguration{RANDOM_ALL, ledsBrightness, {0,0,0}});
}

void radio_receive_onDraw(U8G2 *u8g2) {
    // messy and shitty code for testing, TODO: implement properly
    RFMessage msg;
    u8g2->setDrawColor(1);
    if (emptyList) {
        u8g2->clearBuffer();
        u8g2->drawXBM(3, 0, bat_rx_width, bat_rx_height, bat_rx_bits);
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(40, 10, "Listening at");
        u8g2->drawStr(55, 25, (String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz ").c_str());
        u8g2->drawStr(80, 40, getPresetNameFromEnum(receiverParams->preset).c_str());
        emptyList = false;
        u8g2->sendBuffer();
    }
    if (xQueueReceive(queue, &msg, 0) == pdTRUE) {
        u8g2->clearBuffer();
        String header = String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz " + getPresetNameFromEnum(receiverParams->preset);
        u8g2->setFont(u8g2_font_t0_11_tr);
        u8g2->drawStr(0, 8, header.c_str());
        /*TODO: (index/N elements) u8g2->drawStr(95, 8, "11/50");*/
        emptyList = false;
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