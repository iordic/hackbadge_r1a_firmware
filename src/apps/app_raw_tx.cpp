#include "app.h"
#include "app_raw_tx.h"
#include "app_menu.h"
#include "literals.h"

#include "config/constants.h"
#include "utils/radio_utils.h"
#include "tasks/ui_task.h"
#include "tasks/radio_task.h"
#include "utils/file_utils.h"
#include "utils/menu.h"

extern Preferences prefs;
extern Menu* currentMenu;
extern App app_menu;
extern int row;
extern TaskHandle_t radioTransmitterTaskHandle;

Menu mainListRawTxFiles;
Menu rawTxFileMenu;
RadioTaskParams *rawTransmitterParams;
QueueHandle_t rawTxQueue;
SimpleList<String>* savedRawTxFiles = nullptr;
bool showingRawFileContent = false;
RawTxFile currentRawFileContent;

void raw_tx_onStart() {
    row = 0;
    currentMenu = &mainListRawTxFiles;
    showingRawFileContent = false;
    prefs.begin("configuration", true);
    rawTxQueue = xQueueCreate(4, sizeof(RawSignal*));
    rawTransmitterParams = startRadioTask(SEND_RAW, rawTxQueue, "RawTransmitterWorker", 4096, &radioTransmitterTaskHandle);

    createDynamicMenu(&mainListRawTxFiles, NULL, [](){
        return "RAW " + String(getFrequencyFromEnum(rawTransmitterParams->frequency)) + " " + getPresetNameFromEnum(rawTransmitterParams->preset);
    }, [](){});
    fillRawTxFilesMenu(&mainListRawTxFiles, savedRawTxFiles);
    createMenu(&rawTxFileMenu, &mainListRawTxFiles, [](){
        addMenuNode(&rawTxFileMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ raw_tx_sendSignal(); });
        addMenuNode(&rawTxFileMenu, &READ_FILE_ICON, MENU_ITEM_READ_FILE, [](){ raw_tx_loadFileContent(); showingRawFileContent = true; });
        addMenuNode(&rawTxFileMenu, &DELETE_ICON, MENU_ITEM_DELETE, [](){ removeSelectedRawTxFile(); });
    });
    mainListRawTxFiles.build();
    rawTxFileMenu.build();
}

void raw_tx_onStop() {
    xTaskNotify(radioTransmitterTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    // Drenamos la cola para no filtrar señales que el task no llegó a enviar.
    RawSignal *pending = NULL;
    while (xQueueReceive(rawTxQueue, &pending, 0) == pdTRUE) freeRawSignal(pending);
    vQueueDelete(rawTxQueue);
    if (savedRawTxFiles != nullptr) {
        delete savedRawTxFiles;
        savedRawTxFiles = nullptr;
    }
    currentMenu = NULL;
}

void raw_tx_onEvent(int evt) {
    if (mainListRawTxFiles.list->isEmpty()) {
        if (evt == BTN_BACK) {
            changeAppContext(&app_menu);
        }
        return;
    }
    if (showingRawFileContent) {
        if (evt == BTN_BACK) showingRawFileContent = false;
        return;
    }
    menuHandleEvent(currentMenu, evt);
}

void raw_tx_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    if (mainListRawTxFiles.list->isEmpty()) {
        drawEmptyFolder(u8g2);
        u8g2->sendBuffer();
        return;
    }
    if (showingRawFileContent) {
        u8g2->setFont(u8g2_font_ncenB08_tr);
        u8g2->drawStr(0, 8, ("[" + currentRawFileContent.name + " - " + String(currentRawFileContent.size) + "B]").c_str());
        u8g2->setFont(u8g2_font_t0_12_mr);
        u8g2->drawStr(0, 24, "Pulses:");
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(5, 37, String(currentRawFileContent.count).c_str());
        u8g2->setFont(u8g2_font_t0_12_mr);
        u8g2->drawStr(0, 48, "with presets:");
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(5, 62, (currentRawFileContent.frequency + " MHz " + currentRawFileContent.preset).c_str());
    } else {
        row = drawMenu(u8g2, currentMenu, row);
    }
    u8g2->sendBuffer();
}

void fillRawTxFilesMenu(Menu* menu, SimpleList<String>* &files) {
    menu->list->clear();
    if (files != nullptr) {
        delete files;
        files = nullptr;
    }
    files = FileUtils::listFiles(RAW_TRANSCEIVER_PATH);
    if (files != nullptr) {
        for (int i = 0; i < files->size(); i++) {
            addMenuNode(menu, files->get(i), &app_menu, &rawTxFileMenu);
        }
    }
}

void raw_tx_sendSignal() {
    String fileName = savedRawTxFiles->get(mainListRawTxFiles.selected);
    RawSignal *sig = FileUtils::loadRaw(RAW_TRANSCEIVER_PATH, fileName);
    if (!sig) {
        showPopupMenu("Read error.");
        return;
    }
    // Cedemos la propiedad al task: él la libera cuando termina de emitirla.
    xTaskNotify(radioTransmitterTaskHandle, SEND_RAW, eSetValueWithOverwrite);
    if (xQueueSend(rawTxQueue, &sig, 0) != pdTRUE) {
        freeRawSignal(sig);   // no entró en la cola: nadie más la va a liberar
        showPopupMenu("Busy.");
        return;
    }
    showPopupMenu("Signal sent!");
}

void raw_tx_loadFileContent() {
    String fileName = savedRawTxFiles->get(mainListRawTxFiles.selected);
    RawSignal *sig = FileUtils::loadRaw(RAW_TRANSCEIVER_PATH, fileName);
    currentRawFileContent.name = fileName;
    currentRawFileContent.size = FileUtils::getFileSize(RAW_TRANSCEIVER_PATH, fileName);
    if (!sig) {
        currentRawFileContent.count = 0;
        currentRawFileContent.frequency = "?";
        currentRawFileContent.preset = "?";
        return;
    }
    currentRawFileContent.count = sig->count;
    currentRawFileContent.frequency = String(getFrequencyFromEnum(sig->frequency));
    currentRawFileContent.preset = getPresetNameFromEnum(sig->preset);
    freeRawSignal(sig);
}

void removeSelectedRawTxFile() {
    String deleteFile = savedRawTxFiles->get(mainListRawTxFiles.selected);
    if (FileUtils::remove(RAW_TRANSCEIVER_PATH, deleteFile)) {
        showPopupMenu("File deleted");
        fillRawTxFilesMenu(&mainListRawTxFiles, savedRawTxFiles);
    } else {
        showPopupMenu("Delete error");
    }
    currentMenu = &mainListRawTxFiles;
}

App app_raw_tx = {
  .name = "Raw TX",
  .onStart = raw_tx_onStart,
  .onEvent = raw_tx_onEvent,
  .onDraw = raw_tx_onDraw,
  .onStop = raw_tx_onStop
};
