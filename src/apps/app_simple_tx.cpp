#include "app.h"
#include "app_simple_tx.h"
#include "app_menu.h"
#include "literals.h"

#include "config/constants.h"
#include "utils/radio_utils.h"
#include "tasks/radio_task.h"
#include "tasks/ui_task.h"
#include "utils/file_utils.h"
#include "utils/menu.h"

extern Preferences prefs;
extern Menu* currentMenu;
extern App app_menu;
Menu simpleTxMainMenu;
Menu mainListSimpleTxFiles;
Menu simpleTxFileMenu;
Menu manualTxMenu;
// Manually entered code (filled via on-screen keyboard)
String manualValueStr;
String manualBitsStr;
String manualProtocolStr;
RadioTaskParams *simpleTransmitterParams;
QueueHandle_t simpleTxQueue;
extern int row;
SimpleList<String>* savedSimpleTxFiles;
TaskHandle_t radioTransmitterTaskHandle = NULL;
bool showingFileContent = false;
SimpleTxFile currentFileContent;

void simple_tx_onStart() {
    row = 0;
    currentMenu = &simpleTxMainMenu;
    prefs.begin("configuration", true);
    simpleTransmitterParams = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    simpleTransmitterParams->operation = SEND_SIGNAL;
    simpleTransmitterParams->frequency = prefs.getUChar("frequency", FREQ_433MHZ);
    simpleTransmitterParams->preset = prefs.getUChar("preset", PRESET_AM650);
    simpleTxQueue = xQueueCreate(8, sizeof(RFMessage));
    simpleTransmitterParams->queueHandle = simpleTxQueue;
    simpleTransmitterParams->callerHandle = xTaskGetCurrentTaskHandle();
    xTaskCreatePinnedToCore(radio_task, "RadioTransmitterWorker", 2048, simpleTransmitterParams, 5, &radioTransmitterTaskHandle, 1);
    // Top menu: choose between typing a code by hand or sending a saved one
    createMenu(&simpleTxMainMenu, NULL, [](){
        addMenuNode(&simpleTxMainMenu, []() -> uint16_t { return SIMPLE_TRANSMIT_ICON; }, [](){ return String("Manual"); },
            [](){ changeMenu(&manualTxMenu); }, [](){ changeAppContext(&app_menu); });
        addMenuNode(&simpleTxMainMenu, []() -> uint16_t { return SAVE_ICON; }, [](){ return String("Saved"); },
            [](){ changeMenu(&mainListSimpleTxFiles); }, [](){ changeAppContext(&app_menu); });
    });
    // Saved signals list (files on LittleFS)
    createDynamicMenu(&mainListSimpleTxFiles, &simpleTxMainMenu, [](){return String(getFrequencyFromEnum(simpleTransmitterParams->frequency)) + "MHz " + getPresetNameFromEnum(simpleTransmitterParams->preset);}, [](){});
    fillSimpleTxFilesMenu(&mainListSimpleTxFiles, savedSimpleTxFiles);
    createMenu(&simpleTxFileMenu, &mainListSimpleTxFiles, [](){
        addMenuNode(&simpleTxFileMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ simple_tx_sendSignal(); });
        addMenuNode(&simpleTxFileMenu, &READ_FILE_ICON, MENU_ITEM_READ_FILE, [](){loadFileContent(); showingFileContent = true;});
        addMenuNode(&simpleTxFileMenu, &DELETE_ICON, MENU_ITEM_DELETE, [](){removeSelectedTxFile();});
    });
    // Manual code entry (typed with the on-screen keyboard)
    manualValueStr = "";
    manualBitsStr = "24";
    manualProtocolStr = "1";
    createMenu(&manualTxMenu, &simpleTxMainMenu, [](){
        addMenuNode(&manualTxMenu, [](){ return "Value(hex): " + manualValueStr; }, [](){ startKeyboard(&manualValueStr); });
        addMenuNode(&manualTxMenu, [](){ return "Bits: " + manualBitsStr; }, [](){ startKeyboard(&manualBitsStr); });
        addMenuNode(&manualTxMenu, [](){ return "Protocol: " + manualProtocolStr; }, [](){ startKeyboard(&manualProtocolStr); });
        addMenuNode(&manualTxMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ simple_tx_sendManual(); });
    });
    simpleTxMainMenu.build();
    mainListSimpleTxFiles.build();
    simpleTxFileMenu.build();
    manualTxMenu.build();
}

void simple_tx_onStop() {
    xTaskNotify(radioTransmitterTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    vQueueDelete(simpleTxQueue);
    currentMenu = NULL;
}
void simple_tx_onEvent(int evt) {
    if (showingFileContent) {
        if (evt == BTN_BACK) {
            showingFileContent = false;
        }
        return;
    }
    // Saved list with no files: only allow going back to the top menu
    if (currentMenu == &mainListSimpleTxFiles && currentMenu->list->isEmpty()) {
        if (evt == BTN_BACK) changeMenu(&simpleTxMainMenu);
        return;
    }
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
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    if (mainListSimpleTxFiles.list->isEmpty()) {
        u8g2->setFont(u8g2_font_fub30_t_symbol);
        u8g2->drawStr(25, 40, "404");
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(10, 54, "Folder is empty.");
        u8g2->sendBuffer();
        return;
    }
    if (showingFileContent) {
        u8g2->setFont(u8g2_font_ncenB08_tr);
        u8g2->drawStr(0, 8, ("[File " + currentFileContent.name + " - " + String(currentFileContent.size) + " bytes]").c_str());
        u8g2->setFont(u8g2_font_t0_12_mr);
        u8g2->drawStr(0, 22, "Captured value:");
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(5, 35, (currentFileContent.value + " / " + String(currentFileContent.bits) + "bits").c_str());
        u8g2->setFont(u8g2_font_t0_12_mr);
        u8g2->drawStr(0, 46, "with presets:");
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(5, 60, String(currentFileContent.frequency + " MHz " + currentFileContent.preset).c_str());
    } else {
        row = drawMenu(u8g2, currentMenu, row);
    }
    u8g2->sendBuffer();
}

void fillSimpleTxFilesMenu(Menu* menu, SimpleList<String>* &files) {
    menu->list->clear();
    if (files != nullptr) {
        delete files; 
        files = nullptr;
    }
    files = FileUtils::listFiles(SIMPLE_TRANSCEIVER_PATH);
    if (files != nullptr) {
        for (int i = 0; i < files->size(); i++) {
            String fileName = files->get(i);
            addMenuNode(menu, [fileName](){ return fileName; },
                [](){ changeMenu(&simpleTxFileMenu); },
                [](){ changeMenu(&simpleTxMainMenu); });
        }
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

void simple_tx_sendManual() {
    RFMessage msg;
    msg.value = strtoul(manualValueStr.c_str(), nullptr, 16);
    msg.length = (unsigned int) manualBitsStr.toInt();
    msg.protocol = (unsigned int) manualProtocolStr.toInt();
    msg.frequency = simpleTransmitterParams->frequency;
    msg.preset = simpleTransmitterParams->preset;
    if (msg.value == 0 || msg.length == 0) {
        showPopupMenu("Invalid code");
        return;
    }
    if (msg.protocol == 0) msg.protocol = 1;
    Serial.println("Manual TX: value=0x" + manualValueStr + " bits=" + String(msg.length) + " proto=" + String(msg.protocol));
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

void loadFileContent() {
    RFMessage msg;
    String fileName = savedSimpleTxFiles->get(mainListSimpleTxFiles.selected);
    currentFileContent.name = fileName;
    loadRFMessageFromFile(fileName, &msg);
    currentFileContent.size = FileUtils::getFileSize(SIMPLE_TRANSCEIVER_PATH, fileName);
    currentFileContent.bits = msg.length;
    currentFileContent.frequency = String(getFrequencyFromEnum(msg.frequency));
    currentFileContent.preset = getPresetNameFromEnum(msg.preset);
    currentFileContent.protocol = msg.protocol;
    currentFileContent.value = String(msg.value, HEX);
}

void removeSelectedTxFile() {
    String deleteFile = savedSimpleTxFiles->get(mainListSimpleTxFiles.selected);
    if (FileUtils::remove(SIMPLE_TRANSCEIVER_PATH, deleteFile)) {
        showPopupMenu("Archivo borrado");
        savedSimpleTxFiles->remove(mainListSimpleTxFiles.selected);
        fillSimpleTxFilesMenu(&mainListSimpleTxFiles, savedSimpleTxFiles);
    } else {
        showPopupMenu("Error al borrar");
    }
    currentMenu = &mainListSimpleTxFiles;
}

App app_simple_tx = {
  .name = "Simple TX",
  .onStart = simple_tx_onStart,
  .onEvent = simple_tx_onEvent,
  .onDraw = simple_tx_onDraw,
  .onStop = simple_tx_onStop
};