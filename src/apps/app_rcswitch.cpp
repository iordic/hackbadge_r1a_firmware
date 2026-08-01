#include "app.h"
#include "app_rcswitch.h"
#include "app_menu.h"
#include "literals.h"

#include <SimpleList.h>
#include "config/constants.h"
#include "config/sprites.h"
#include "utils/radio_utils.h"
#include "utils/menu.h"
#include "utils/file_utils.h"
#include "devices/display.h"
#include "devices/radio.h"
#include "tasks/ui_task.h"
#include "tasks/leds_task.h"
#include "tasks/radio_task.h"

// ---------------------------------------------------------------------------
// App RC-Switch (fusiona el antiguo Simple RX + Simple TX).
// Un menú de modo al entrar: Recibir / Guardados / Manual. El worker de radio
// se arranca al entrar en un modo y se para al volver (solo uno vivo a la vez):
//   - Recibir : RECEIVE_SIGNAL, replay/guardar la señal seleccionada.
//   - Guardados: SEND_SIGNAL, enviar/leer/borrar ficheros.
//   - Manual  : SEND_SIGNAL, teclear value/bits/protocol y enviar.
// ---------------------------------------------------------------------------

extern App app_menu;
extern Preferences prefs;
extern Menu* currentMenu;
extern Menu subghzMenu;   // para volver al submenú Sub-GHz al salir
extern int row;
extern uint8_t ledsBrightness;

// Handle del worker TX, compartido con radio_task (lo pone a NULL al terminar
// SEND_SIGNAL/SEND_RAW). También lo usa app_raw vía extern.
TaskHandle_t radioTransmitterTaskHandle = NULL;

enum RcMode { RC_MENU, RC_RECEIVE, RC_SAVED, RC_MANUAL };
static RcMode rcMode = RC_MENU;

// Menús
static Menu rcModeMenu;       // raíz: Recibir / Guardados / Manual
static Menu rcRxList;         // capturas en vivo
static Menu rcRxSignalMenu;   // acciones sobre una captura: Replay / Save
static Menu rcSaveNameMenu;   // Name + Accept
static Menu rcSavedList;      // ficheros guardados
static Menu rcSavedFileMenu;  // acciones de fichero: Send / Read / Delete
static Menu rcManualMenu;     // teclear código

// Estado RX
static QueueHandle_t rcRxQueue = NULL;
static TaskHandle_t  rcReceiverHandle = NULL;
static RadioTaskParams *rcReceiverParams = NULL;
static SimpleList<RFMessage> *rcReceived = NULL;
static String rcSaveName = "";

// Estado TX (SEND_SIGNAL, para Guardados y Manual)
static QueueHandle_t rcTxQueue = NULL;
static RadioTaskParams *rcTxParams = NULL;
static SimpleList<String>* rcSavedFiles = NULL;

// Entrada manual
static String rcManualValue = "";
static SettingsValue rcManualBits;
static SettingsValue rcManualProtocol;

// Vista de contenido de fichero
static bool rcShowingFile = false;
typedef struct {
    String name, value;
    int protocol, bits;
    String frequency, preset;
    int size;
} RcFileView;
static RcFileView rcFileContent;

// Prototipos internos
static void rc_enterReceive();
static void rc_enterSaved();
static void rc_enterManual();
static void rc_exitSubMode();
static void rc_stopReceiver();
static void rc_stopTransmitter();
static void rc_replay();
static void rc_saveCaptured();
static void rc_fillSavedFiles();
static void rc_sendSelectedFile();
static void rc_sendManual();
static void rc_loadFileContent();
static void rc_removeSelectedFile();
static void rc_buildMenus();

// --- Ciclo de vida de la app --------------------------------------------------

static void rcswitch_onStart() {
    rcMode = RC_MENU;
    row = 0;
    rcShowingFile = false;
    rc_buildMenus();
    currentMenu = &rcModeMenu;
}

static void rcswitch_onStop() {
    // Normalmente se sale desde el menú de modo (sin worker), pero por si acaso
    // paramos lo que hubiera activo.
    switch (rcMode) {
    case RC_RECEIVE: rc_stopReceiver();    break;
    case RC_SAVED:
    case RC_MANUAL:  rc_stopTransmitter(); break;
    default: break;
    }
    rcMode = RC_MENU;
    currentMenu = &subghzMenu;   // al salir, volver al submenú Sub-GHz
}

static void rcswitch_onEvent(int evt) {
    if (rcMode == RC_MENU) {
        if (evt == BTN_BACK) { changeAppContext(&app_menu); return; }
        menuHandleEvent(&rcModeMenu, evt);
        return;
    }
    if (rcShowingFile) {
        if (evt == BTN_BACK) rcShowingFile = false;
        return;
    }
    Menu* root = (rcMode == RC_RECEIVE) ? &rcRxList
               : (rcMode == RC_SAVED)   ? &rcSavedList
                                        : &rcManualMenu;
    if (currentMenu == root) {
        // BACK en la raíz de un submodo: para el worker y vuelve al menú de modo.
        if (evt == BTN_BACK) { rc_exitSubMode(); return; }
        // Listas RX/guardados pueden estar vacías: solo permitimos BACK.
        if ((rcMode == RC_RECEIVE || rcMode == RC_SAVED) && root->list->isEmpty()) return;
    }
    menuHandleEvent(currentMenu, evt);
}

static void rcswitch_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    switch (rcMode) {
    case RC_MENU:
        row = drawMenu(u8g2, &rcModeMenu, row);
        break;
    case RC_RECEIVE: {
        RFMessage msg;
        if (rcReceived->size() == 0) {
            u8g2->drawXBM(3, 0, bat_rx_width, bat_rx_height, bat_rx_bits);
            u8g2->setFont(u8g2_font_7x14_tr);
            u8g2->drawStr(40, 10, "Listening at");
            u8g2->drawStr(55, 25, (String(getFrequencyFromEnum(rcReceiverParams->frequency)) + "MHz ").c_str());
            u8g2->drawStr(80, 40, getPresetNameFromEnum(rcReceiverParams->preset).c_str());
        } else {
            // currentMenu puede ser la lista de capturas o su submenú (Replay/Save).
            row = drawMenu(u8g2, currentMenu, row);
        }
        if (xQueueReceive(rcRxQueue, &msg, 0) == pdTRUE) {
            rcReceived->add(msg);
            String label = "P" + String(msg.protocol) + " V" + String(msg.value, HEX) + " L" + String(msg.length);
            addMenuNode(&rcRxList, [label](){ return label; }, &rcRxSignalMenu);
        }
        drawRssi(u8g2);
        break;
    }
    case RC_SAVED:
        if (rcSavedList.list->isEmpty()) {
            drawEmptyFolder(u8g2);
        } else if (rcShowingFile) {
            u8g2->setFont(u8g2_font_ncenB08_tr);
            u8g2->drawStr(0, 8, ("[File " + rcFileContent.name + " - " + String(rcFileContent.size) + " bytes]").c_str());
            u8g2->setFont(u8g2_font_t0_12_mr);
            u8g2->drawStr(0, 22, "Captured value:");
            u8g2->setFont(u8g2_font_7x14_mr);
            u8g2->drawStr(5, 35, (rcFileContent.value + " / " + String(rcFileContent.bits) + "bits").c_str());
            u8g2->setFont(u8g2_font_t0_12_mr);
            u8g2->drawStr(0, 46, "with presets:");
            u8g2->setFont(u8g2_font_7x14_mr);
            u8g2->drawStr(5, 60, String(rcFileContent.frequency + " MHz " + rcFileContent.preset).c_str());
        } else {
            // currentMenu puede ser la lista de ficheros o su submenú (Send/Read/Delete).
            row = drawMenu(u8g2, currentMenu, row);
        }
        break;
    case RC_MANUAL:
        row = drawMenu(u8g2, &rcManualMenu, row);
        break;
    }
    u8g2->sendBuffer();
}

// --- Transiciones de modo -----------------------------------------------------

static void rc_enterReceive() {
    rcMode = RC_RECEIVE;
    row = 0;
    rcReceived = new SimpleList<RFMessage>;
    rcRxList.list->clear();
    rcRxList.selected = 0;
    rcRxQueue = xQueueCreate(8, sizeof(RFMessage));
    rcReceiverParams = startRadioTask(RECEIVE_SIGNAL, rcRxQueue, "RcSwitchRx", 2048, &rcReceiverHandle);
    ledsBrightness = prefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
    sendNeopixelSolid(0x000000ff, ledsBrightness);   // azul: escuchando
    currentMenu = &rcRxList;
}

static void rc_stopReceiver() {
    if (rcReceiverHandle) xTaskNotify(rcReceiverHandle, RADIO_STOP, eSetValueWithOverwrite);
    rcReceiverHandle = NULL;
    sendNeopixelIdle(ledsBrightness);
    if (rcRxQueue) { vQueueDelete(rcRxQueue); rcRxQueue = NULL; }
    if (rcReceived) { delete rcReceived; rcReceived = NULL; }
    rcRxList.list->clear();
}

static void rc_enterSaved() {
    rcMode = RC_SAVED;
    row = 0;
    rcShowingFile = false;
    rcTxQueue = xQueueCreate(8, sizeof(RFMessage));
    rcTxParams = startRadioTask(SEND_SIGNAL, rcTxQueue, "RcSwitchTx", 2048, &radioTransmitterTaskHandle);
    rc_fillSavedFiles();
    rcSavedList.selected = 0;
    currentMenu = &rcSavedList;
}

static void rc_enterManual() {
    rcMode = RC_MANUAL;
    row = 0;
    rcTxQueue = xQueueCreate(8, sizeof(RFMessage));
    rcTxParams = startRadioTask(SEND_SIGNAL, rcTxQueue, "RcSwitchTx", 2048, &radioTransmitterTaskHandle);
    rcManualMenu.selected = 0;
    currentMenu = &rcManualMenu;
}

static void rc_stopTransmitter() {
    if (radioTransmitterTaskHandle) xTaskNotify(radioTransmitterTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    radioTransmitterTaskHandle = NULL;
    if (rcTxQueue) { vQueueDelete(rcTxQueue); rcTxQueue = NULL; }
    if (rcSavedFiles) { delete rcSavedFiles; rcSavedFiles = NULL; }
}

static void rc_exitSubMode() {
    switch (rcMode) {
    case RC_RECEIVE: rc_stopReceiver();    break;
    case RC_SAVED:
    case RC_MANUAL:  rc_stopTransmitter(); break;
    default: break;
    }
    rcMode = RC_MENU;
    row = 0;
    rcShowingFile = false;
    currentMenu = &rcModeMenu;
}

// --- Acciones -----------------------------------------------------------------

static void rc_replay() {
    if (!rcReceived || rcReceived->size() == 0) return;
    RFMessage msg = rcReceived->get(rcRxList.selected);
    xTaskNotify(rcReceiverHandle, REPLAY_SIGNAL, eSetValueWithOverwrite);
    xQueueSend(rcRxQueue, &msg, 0);
    showPopupMenu("Signal replayed!");
}

static void rc_saveCaptured() {
    if (!rcReceived || rcReceived->size() == 0) return;
    RFMessage msg = rcReceived->get(rcRxList.selected);
    msg.frequency = rcReceiverParams->frequency;
    msg.preset = rcReceiverParams->preset;
    if (FileUtils::save(SIMPLE_TRANSCEIVER_PATH, rcSaveName, (uint8_t*)&msg, sizeof(RFMessage))) {
        showPopupMenu("Saved!");
        rcSaveName = "";
        currentMenu = &rcRxList;
    } else {
        showPopupMenu("Error.");
    }
}

static void rc_fillSavedFiles() {
    rcSavedList.list->clear();
    if (rcSavedFiles) { delete rcSavedFiles; rcSavedFiles = NULL; }
    rcSavedFiles = FileUtils::listFiles(SIMPLE_TRANSCEIVER_PATH);
    if (rcSavedFiles) {
        for (int i = 0; i < rcSavedFiles->size(); i++) {
            String fileName = rcSavedFiles->get(i);
            addMenuNode(&rcSavedList, [fileName](){ return fileName; }, &rcSavedFileMenu);
        }
    }
}

static void rc_sendSelectedFile() {
    String fileName = rcSavedFiles->get(rcSavedList.selected);
    RFMessage msg;
    if (!FileUtils::load(SIMPLE_TRANSCEIVER_PATH, fileName, (uint8_t*)&msg, sizeof(RFMessage))) {
        showPopupMenu("Read error.");
        return;
    }
    xTaskNotify(radioTransmitterTaskHandle, SEND_SIGNAL, eSetValueWithOverwrite);
    xQueueSend(rcTxQueue, &msg, 0);
    showPopupMenu("Signal sent!");
}

static void rc_sendManual() {
    RFMessage msg;
    msg.value = strtoul(rcManualValue.c_str(), nullptr, 16);
    msg.length = rcManualBits.current;
    msg.protocol = rcManualProtocol.current;
    msg.frequency = rcTxParams->frequency;
    msg.preset = rcTxParams->preset;
    if (msg.value == 0 || msg.length == 0) { showPopupMenu("Invalid code"); return; }
    if (msg.protocol == 0) msg.protocol = 1;
    xTaskNotify(radioTransmitterTaskHandle, SEND_SIGNAL, eSetValueWithOverwrite);
    xQueueSend(rcTxQueue, &msg, 0);
    showPopupMenu("Signal sent!");
}

static void rc_loadFileContent() {
    RFMessage msg;
    String fileName = rcSavedFiles->get(rcSavedList.selected);
    rcFileContent.name = fileName;
    FileUtils::load(SIMPLE_TRANSCEIVER_PATH, fileName, (uint8_t*)&msg, sizeof(RFMessage));
    rcFileContent.size = FileUtils::getFileSize(SIMPLE_TRANSCEIVER_PATH, fileName);
    rcFileContent.bits = msg.length;
    rcFileContent.frequency = String(getFrequencyFromEnum(msg.frequency));
    rcFileContent.preset = getPresetNameFromEnum(msg.preset);
    rcFileContent.protocol = msg.protocol;
    rcFileContent.value = String(msg.value, HEX);
}

static void rc_removeSelectedFile() {
    String deleteFile = rcSavedFiles->get(rcSavedList.selected);
    if (FileUtils::remove(SIMPLE_TRANSCEIVER_PATH, deleteFile)) {
        showPopupMenu("File deleted");
        rc_fillSavedFiles();
        rcSavedList.selected = 0;
    } else {
        showPopupMenu("Delete error");
    }
    currentMenu = &rcSavedList;
}

// --- Construcción de menús ----------------------------------------------------

static void rc_buildMenus() {
    createMenu(&rcModeMenu, NULL, [](){
        addMenuNode(&rcModeMenu, &SIMPLE_RECEIVE_ICON, MENU_ITEM_RECEIVE, [](){ rc_enterReceive(); });
        addMenuNode(&rcModeMenu, &SAVE_ICON, MENU_ITEM_SAVED, [](){ rc_enterSaved(); });
        addMenuNode(&rcModeMenu, &SIMPLE_TRANSMIT_ICON, MENU_ITEM_MANUAL, [](){ rc_enterManual(); });
    });
    createDynamicMenu(&rcRxList, &rcModeMenu, [](){
        return String(getFrequencyFromEnum(rcReceiverParams->frequency)) + "MHz " + getPresetNameFromEnum(rcReceiverParams->preset);
    }, [](){});
    createMenu(&rcRxSignalMenu, &rcRxList, [](){
        addMenuNode(&rcRxSignalMenu, &REPLAY_ICON, MENU_ITEM_REPLAY, [](){ rc_replay(); });
        addMenuNode(&rcRxSignalMenu, &SAVE_ICON, MENU_ITEM_SAVE, &rcSaveNameMenu);
    });
    createMenu(&rcSaveNameMenu, &rcRxSignalMenu, [](){
        addMenuNode(&rcSaveNameMenu, [](){ return "Name: " + rcSaveName; }, [](){ startKeyboard(&rcSaveName); });
        addMenuNode(&rcSaveNameMenu, "Accept", [](){ rc_saveCaptured(); });
    });
    createDynamicMenu(&rcSavedList, &rcModeMenu, [](){
        return String(getFrequencyFromEnum(rcTxParams->frequency)) + "MHz " + getPresetNameFromEnum(rcTxParams->preset);
    }, [](){});
    createMenu(&rcSavedFileMenu, &rcSavedList, [](){
        addMenuNode(&rcSavedFileMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ rc_sendSelectedFile(); });
        addMenuNode(&rcSavedFileMenu, &READ_FILE_ICON, MENU_ITEM_READ_FILE, [](){ rc_loadFileContent(); rcShowingFile = true; });
        addMenuNode(&rcSavedFileMenu, &DELETE_ICON, MENU_ITEM_DELETE, [](){ rc_removeSelectedFile(); });
    });
    rcManualValue = "";
    rcManualBits.current = 24; rcManualBits.max = 64;
    rcManualProtocol.current = 1; rcManualProtocol.max = 12;
    createMenu(&rcManualMenu, &rcModeMenu, [](){
        addMenuNode(&rcManualMenu, [](){ return String(MENU_ITEM_MANUAL_VALUE) + rcManualValue; }, [](){ startKeyboard(&rcManualValue); });
        addMenuNodeSetting(&rcManualMenu, MENU_ITEM_MANUAL_BITS, &rcManualBits, [](uint8_t v){ return String(v); }, &rcModeMenu);
        addMenuNodeSetting(&rcManualMenu, MENU_ITEM_MANUAL_PROTOCOL, &rcManualProtocol, [](uint8_t v){ return String(v); }, &rcModeMenu);
        addMenuNode(&rcManualMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ rc_sendManual(); });
    });
    rcModeMenu.build();
    rcRxSignalMenu.build();
    rcSaveNameMenu.build();
    rcSavedFileMenu.build();
    rcManualMenu.build();
    // rcRxList y rcSavedList se rellenan al entrar en su modo.
}

App app_rcswitch = {
  .name = "RC-Switch",
  .onStart = rcswitch_onStart,
  .onEvent = rcswitch_onEvent,
  .onDraw = rcswitch_onDraw,
  .onStop = rcswitch_onStop
};
