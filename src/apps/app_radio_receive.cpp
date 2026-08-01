#include "app.h"
#include "app_radio_receive.h"
#include "literals.h"
#include "app_menu.h"

#include "config/constants.h"
#include "config/sprites.h"
#include "utils/menu.h"
#include "utils/radio_utils.h"
#include "utils/file_utils.h"
#include "devices/display.h"
#include "devices/radio.h"
#include "tasks/ui_task.h"
#include "tasks/leds_task.h"
#include "tasks/radio_task.h"

extern App app_menu;
QueueHandle_t queue;
TaskHandle_t radioReceiverTaskHandle = NULL;
RadioTaskParams *receiverParams;
extern Preferences prefs;
extern uint8_t ledsBrightness;
Menu mainListReceivedSignals;
Menu receivedSignalsMenu;
Menu saveFileMenu;
SimpleList <RFMessage> *receivedMessages;
extern int row;
extern Menu* currentMenu;
String saveFileName = "";

void radio_receive_onStart() {
    currentMenu = &mainListReceivedSignals;
    receivedMessages = new SimpleList<RFMessage>;
    queue = xQueueCreate(8, sizeof(RFMessage));
    receiverParams = startRadioTask(RECEIVE_SIGNAL, queue, "RadioReceiverWorker", 2048, &radioReceiverTaskHandle);
    ledsBrightness = prefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
    sendNeopixelSolid(0x000000ff, ledsBrightness);   // azul: modo RX
    // Create received signals menu
    createDynamicMenu(&mainListReceivedSignals, NULL, [](){return String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz " + getPresetNameFromEnum(receiverParams->preset);}, [](){});
    createMenu(&receivedSignalsMenu, &mainListReceivedSignals, [](){
        addMenuNode(&receivedSignalsMenu, &REPLAY_ICON, MENU_ITEM_REPLAY, [](){ replaySignal(); });
        addMenuNode(&receivedSignalsMenu, &SAVE_ICON, MENU_ITEM_SAVE, &saveFileMenu);
    });
    createMenu(&saveFileMenu, &receivedSignalsMenu, []() {
        addMenuNode(&saveFileMenu, [](){ return "Name: " + saveFileName; }, [](){ startKeyboard(&saveFileName);});
        addMenuNode(&saveFileMenu, "Accept", &saveSignal);
    });
    receivedSignalsMenu.build();
    saveFileMenu.build();
}

void radio_receive_onStop() {
    xTaskNotify(radioReceiverTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    sendNeopixelIdle(ledsBrightness);
    vQueueDelete(queue);
    currentMenu = NULL;
}

void radio_receive_onDraw(U8G2 *u8g2) {
    RFMessage msg;
    u8g2->setDrawColor(1);
    u8g2->clearBuffer();
    if (receivedMessages->size() == 0) {
        u8g2->drawXBM(3, 0, bat_rx_width, bat_rx_height, bat_rx_bits);
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(40, 10, "Listening at");
        u8g2->drawStr(55, 25, (String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz ").c_str());
        u8g2->drawStr(80, 40, getPresetNameFromEnum(receiverParams->preset).c_str());
    } else if (receivedMessages->size() > 0) {
        row = drawMenu(u8g2, currentMenu, row);
    }
    if (xQueueReceive(queue, &msg, 0) == pdTRUE) {
        receivedMessages->add(msg);
        String signalLabel = "P" + String(msg.protocol) +" V" + String(msg.value, HEX)+ " L" + String(msg.length);
        addMenuNode(&mainListReceivedSignals, signalLabel, &app_menu, &receivedSignalsMenu);
    }
    drawRssi(u8g2);
    u8g2->sendBuffer();
}
void radio_receive_onEvent(int evt) {
    if (receivedMessages->size() == 0) {
        if (evt == BTN_BACK) {
            changeAppContext(&app_menu);
        }
        return;
    }
    menuHandleEvent(currentMenu, evt);
}

void replaySignal() {
    if (receivedMessages->size() == 0) return;
    RFMessage msg = receivedMessages->get(mainListReceivedSignals.selected);
    xTaskNotify(radioReceiverTaskHandle, REPLAY_SIGNAL, eSetValueWithOverwrite);
    xQueueSend(queue, &msg, 0);
    showPopupMenu("Signal replayed!");
}

void saveSignal() {
    if (receivedMessages->size() == 0) return;
    RFMessage msg = receivedMessages->get(mainListReceivedSignals.selected);
    msg.frequency = receiverParams->frequency;
    msg.preset = receiverParams->preset;
    bool result = FileUtils::save(SIMPLE_TRANSCEIVER_PATH, saveFileName, (uint8_t*)&msg, sizeof(RFMessage));
    if (result) {
        showPopupMenu("Saved!");
        saveFileName = "";
        currentMenu = &mainListReceivedSignals;
    } else {
        showPopupMenu("Error.");
    }
}

void drawRssi(U8G2 *u8g2) {
    int receivedRssi = radio_get()->getRssi();
    int barWidth = 40;
    u8g2->setDrawColor(1);
    u8g2->drawBox(85, 59, map(receivedRssi, -100, -11, 0, 40), 5);
    u8g2->drawFrame(85, 59, barWidth, 5);
    u8g2->setFont(u8g2_font_tiny5_tr);
    u8g2->drawStr(70, 64, "rssi");
}

App app_radio_receive = {
  .name = "Radio Receive",
  .onStart = radio_receive_onStart,
  .onEvent = radio_receive_onEvent,
  .onDraw = radio_receive_onDraw,
  .onStop = radio_receive_onStop
};