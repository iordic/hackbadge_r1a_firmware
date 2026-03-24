#include "app.h"
#include "app_simple_tx.h"
#include "app_menu.h"
#include "literals.h"

#include "config/constants.h"
#include "utils/radio_utils.h"
#include "tasks/radio_task.h"
#include "utils/file_utils.h"
#include "utils/menu.h"

extern Preferences prefs;
extern Menu* currentMenu;
extern App app_menu;
Menu mainListSimpleTxFiles;
Menu simpleTxFileMenu;
RadioTaskParams *simpleTransmitterParams;
QueueHandle_t simpleTxQueue;
extern int row;
SimpleList<String>* savedSimpleTxFiles;
TaskHandle_t radioTransmitterTaskHandle = NULL;

void simple_tx_onStart() {
    row = 0;
    currentMenu = &mainListSimpleTxFiles;
    prefs.begin("configuration", true);
    simpleTransmitterParams = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    simpleTransmitterParams->operation = SEND_SIGNAL;
    simpleTransmitterParams->frequency = prefs.getUChar("frequency", FREQ_433MHZ);
    simpleTransmitterParams->preset = prefs.getUChar("preset", PRESET_AM650);
    simpleTxQueue = xQueueCreate(8, sizeof(RFMessage));
    simpleTransmitterParams->queueHandle = simpleTxQueue;
    simpleTransmitterParams->callerHandle = xTaskGetCurrentTaskHandle();
    xTaskCreatePinnedToCore(radio_task, "RadioTransmitterWorker", 2048, simpleTransmitterParams, 5, &radioTransmitterTaskHandle, 1);
    createDynamicMenu(&mainListSimpleTxFiles, NULL, [](){return String(getFrequencyFromEnum(simpleTransmitterParams->frequency)) + "MHz " + getPresetNameFromEnum(simpleTransmitterParams->preset);}, [](){});
    fillSimpleTxFilesMenu(&mainListSimpleTxFiles, savedSimpleTxFiles);
    createMenu(&simpleTxFileMenu, &mainListSimpleTxFiles, [](){
        addMenuNode(&simpleTxFileMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ simple_tx_sendSignal(); });
        addMenuNode(&simpleTxFileMenu, &READ_FILE_ICON, MENU_ITEM_READ_FILE, NULL);
        addMenuNode(&simpleTxFileMenu, &DELETE_ICON, MENU_ITEM_DELETE, NULL);
    });
    mainListSimpleTxFiles.build();
    simpleTxFileMenu.build();
}

void simple_tx_onStop() {
    xTaskNotify(radioTransmitterTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    vQueueDelete(simpleTxQueue);
    currentMenu = NULL;
}
void simple_tx_onEvent(int evt) {
  if (evt == BTN_BACK) {
      currentMenu->list->get(currentMenu->selected).hold();
  } else if (evt == BTN_OK) {
      currentMenu->list->get(currentMenu->selected).click();
  } else if (evt == BTN_UP) {
      currentMenu->selected--;
  } else if (evt == BTN_DOWN) {
      currentMenu->selected++;
  } else if (evt == BTN_LEFT) {
      currentMenu->list->get(currentMenu->selected).left();
  } else if (evt == BTN_RIGHT) {
      currentMenu->list->get(currentMenu->selected).right();
  }
}
void simple_tx_onDraw(U8G2 *u8g2) {
  row = drawMenu(u8g2, currentMenu, row);
}

void fillSimpleTxFilesMenu(Menu* menu, SimpleList<String>* &files) {
    files = FileUtils::listFiles(SIMPLE_TRANSCEIVER_PATH);
    for (int i = 0; i < files->size(); i++) {
        addMenuNode(menu, files->get(i), &app_menu, &simpleTxFileMenu);
    }
}

void simple_tx_sendSignal() {
    String fileName = savedSimpleTxFiles->get(mainListSimpleTxFiles.selected);
    Serial.println("Selected file: " + fileName);
    simple_tx_sendSignal(fileName);
}

void simple_tx_sendSignal(String fileName) {
    RFMessage msg;
    loadRFMessageFromFile(fileName, &msg);
    xTaskNotify(radioTransmitterTaskHandle, SEND_SIGNAL, eSetValueWithOverwrite);
    xQueueSend(simpleTxQueue, &msg, 0);
    showPopupMenu("Signal sent!");
}

void loadRFMessageFromFile(String fileName, RFMessage* msg) {
    if (!FileUtils::load(SIMPLE_TRANSCEIVER_PATH, fileName, (uint8_t*)msg, sizeof(RFMessage))) {
        Serial.println("Error reading file.");
        return;
    }
    Serial.println("File loaded: " + String(fileName));
}

App app_simple_tx = {
  .name = "Simple TX",
  .onStart = simple_tx_onStart,
  .onEvent = simple_tx_onEvent,
  .onDraw = simple_tx_onDraw,
  .onStop = simple_tx_onStop
};