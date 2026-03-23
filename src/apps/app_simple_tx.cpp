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
RadioTaskParams *simpleTxReceiverParams;
QueueHandle_t simpleTxQueue;
extern int row;
SimpleList<File>* savedSimpleTxFiles;

void simple_tx_onStart() {
    row = 0;
    currentMenu = &mainListSimpleTxFiles;
    prefs.begin("configuration", true);
    simpleTxReceiverParams = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    simpleTxReceiverParams->operation = RECEIVE_SIGNAL;
    simpleTxReceiverParams->frequency = prefs.getUChar("frequency", FREQ_433MHZ);
    simpleTxReceiverParams->preset = prefs.getUChar("preset", PRESET_AM650);
    simpleTxQueue = xQueueCreate(8, sizeof(RFMessage));
    simpleTxReceiverParams->queueHandle = simpleTxQueue;
    simpleTxReceiverParams->callerHandle = xTaskGetCurrentTaskHandle();
    createDynamicMenu(&mainListSimpleTxFiles, NULL, [](){return String(getFrequencyFromEnum(simpleTxReceiverParams->frequency)) + "MHz " + getPresetNameFromEnum(simpleTxReceiverParams->preset);}, [](){});
    fillSimpleTxFilesMenu(&mainListSimpleTxFiles, savedSimpleTxFiles);
    createMenu(&simpleTxFileMenu, &mainListSimpleTxFiles, [](){
        addMenuNode(&simpleTxFileMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, NULL);
        addMenuNode(&simpleTxFileMenu, &READ_FILE_ICON, MENU_ITEM_READ_FILE, NULL);
        addMenuNode(&simpleTxFileMenu, &DELETE_ICON, MENU_ITEM_DELETE, NULL);
    });
    mainListSimpleTxFiles.build();
    simpleTxFileMenu.build();
}

void simple_tx_onStop() {
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

void fillSimpleTxFilesMenu(Menu* menu, SimpleList<File>* files) {
    files = FileUtils::listFiles(SIMPLE_TRANSCEIVER_PATH);
    for (int i = 0; i < files->size(); i++) {
        addMenuNode(menu, files->get(i).name(), &app_menu, &simpleTxFileMenu);
    }
}

App app_simple_tx = {
  .name = "Simple TX",
  .onStart = simple_tx_onStart,
  .onEvent = simple_tx_onEvent,
  .onDraw = simple_tx_onDraw,
  .onStop = simple_tx_onStop
};