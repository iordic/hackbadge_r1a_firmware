#include "app.h"
#include "apps.h"
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
// App Raw (fusiona el antiguo Raw RX + Raw TX).
// Menú de modo al entrar: Recibir / Guardados. El worker se arranca al entrar
// en un modo y se para al volver (solo uno vivo a la vez):
//   - Recibir : RECEIVE_RAW, replay/guardar la captura seleccionada.
//   - Guardados: SEND_RAW, enviar/leer/borrar ficheros.
// Las señales viven en heap (RawSignal*); las capturas son propiedad de la app,
// las de envío se ceden al task (él las libera). Ver notas de ownership abajo.
// ---------------------------------------------------------------------------

extern Preferences prefs;
extern Menu* currentMenu;
extern Menu subghzMenu;   // para volver al submenú Sub-GHz al salir
extern int row;
extern uint8_t ledsBrightness;
extern TaskHandle_t radioTransmitterTaskHandle;  // definido en app_rcswitch.cpp

// Cola UI->task de peticiones de replay RAW. La lee radio_task por extern, así
// que necesita enlace externo (no static).
QueueHandle_t rawReplayQueue = NULL;

enum RawMode { RAW_MENU, RAW_RECEIVE, RAW_SAVED };
static RawMode rawMode = RAW_MENU;

// Menús
static Menu rawModeMenu;       // raíz: Recibir / Guardados
static Menu rawRxList;         // capturas en vivo
static Menu rawRxSignalMenu;   // acciones sobre una captura: Replay / Save
static Menu rawSaveNameMenu;   // Name + Accept
static Menu rawSavedList;      // ficheros guardados
static Menu rawSavedFileMenu;  // acciones de fichero: Send / Read / Delete

// Estado RX
static QueueHandle_t rawRxQueue = NULL;   // task->UI: capturas nuevas (RawSignal*)
static TaskHandle_t  rawReceiverHandle = NULL;
static RadioTaskParams *rawReceiverParams = NULL;
static SimpleList<RawSignal*> *rawMessages = NULL;   // capturas, propiedad de la app
static String rawSaveName = "";

// Estado TX (SEND_RAW)
static QueueHandle_t rawTxQueue = NULL;
static RadioTaskParams *rawTxParams = NULL;
static SimpleList<String>* rawSavedFiles = NULL;

// Vista de contenido de fichero
static bool rawShowingFile = false;
typedef struct {
    String name;
    int count;
    String frequency, preset;
    int size;
} RawFileView;
static RawFileView rawFileContent;

// Prototipos internos
static void raw_enterReceive();
static void raw_enterSaved();
static void raw_exitSubMode();
static void raw_stopReceiver();
static void raw_stopTransmitter();
static void raw_freeAllMessages();
static void raw_replay();
static void raw_saveCaptured();
static void raw_fillSavedFiles();
static void raw_sendSelectedFile();
static void raw_loadFileContent();
static void raw_removeSelectedFile();
static void raw_buildMenus();

// --- Ciclo de vida de la app --------------------------------------------------

static void raw_onStart() {
    rawMode = RAW_MENU;
    row = 0;
    rawShowingFile = false;
    raw_buildMenus();
    currentMenu = &rawModeMenu;
}

static void raw_onStop() {
    switch (rawMode) {
    case RAW_RECEIVE: raw_stopReceiver();    break;
    case RAW_SAVED:   raw_stopTransmitter(); break;
    default: break;
    }
    rawMode = RAW_MENU;
    currentMenu = &subghzMenu;   // al salir, volver al submenú Sub-GHz
}

static void raw_onEvent(int evt) {
    if (rawMode == RAW_MENU) {
        if (evt == BTN_BACK) { changeAppContext(&app_menu); return; }
        menuHandleEvent(&rawModeMenu, evt);
        return;
    }
    if (rawShowingFile) {
        if (evt == BTN_BACK) rawShowingFile = false;
        return;
    }
    Menu* root = (rawMode == RAW_RECEIVE) ? &rawRxList : &rawSavedList;
    if (currentMenu == root) {
        if (evt == BTN_BACK) { raw_exitSubMode(); return; }
        if (root->list->isEmpty()) return;   // lista vacía: solo BACK
    }
    menuHandleEvent(currentMenu, evt);
}

static void raw_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    switch (rawMode) {
    case RAW_MENU:
        row = drawMenu(u8g2, &rawModeMenu, row);
        break;
    case RAW_RECEIVE: {
        RawSignal *sig = NULL;
        if (rawMessages->size() == 0) {
            u8g2->drawXBM(3, 0, bat_rx_width, bat_rx_height, bat_rx_bits);
            u8g2->setFont(u8g2_font_7x14_tr);
            u8g2->drawStr(40, 10, "RAW listen");
            u8g2->drawStr(55, 25, (String(getFrequencyFromEnum(rawReceiverParams->frequency)) + "MHz ").c_str());
            u8g2->drawStr(80, 40, getPresetNameFromEnum(rawReceiverParams->preset).c_str());
        } else {
            // currentMenu puede ser la lista de capturas o su submenú (Replay/Save).
            row = drawMenu(u8g2, currentMenu, row);
        }
        if (xQueueReceive(rawRxQueue, &sig, 0) == pdTRUE && sig) {
            rawMessages->add(sig);
            String label = "RAW " + String(sig->count) + "ch";
            addMenuNode(&rawRxList, [label](){ return label; }, &rawRxSignalMenu);
        }
        drawRssi(u8g2);
        break;
    }
    case RAW_SAVED:
        if (rawSavedList.list->isEmpty()) {
            drawEmptyFolder(u8g2);
        } else if (rawShowingFile) {
            u8g2->setFont(u8g2_font_ncenB08_tr);
            u8g2->drawStr(0, 8, ("[" + rawFileContent.name + " - " + String(rawFileContent.size) + "B]").c_str());
            u8g2->setFont(u8g2_font_t0_12_mr);
            u8g2->drawStr(0, 24, "Pulses:");
            u8g2->setFont(u8g2_font_7x14_mr);
            u8g2->drawStr(5, 37, String(rawFileContent.count).c_str());
            u8g2->setFont(u8g2_font_t0_12_mr);
            u8g2->drawStr(0, 48, "with presets:");
            u8g2->setFont(u8g2_font_7x14_mr);
            u8g2->drawStr(5, 62, (rawFileContent.frequency + " MHz " + rawFileContent.preset).c_str());
        } else {
            // currentMenu puede ser la lista de ficheros o su submenú (Send/Read/Delete).
            row = drawMenu(u8g2, currentMenu, row);
        }
        break;
    }
    u8g2->sendBuffer();
}

// --- Transiciones de modo -----------------------------------------------------

static void raw_enterReceive() {
    rawMode = RAW_RECEIVE;
    row = 0;
    rawMessages = new SimpleList<RawSignal*>;
    rawRxList.list->clear();
    rawRxList.selected = 0;
    // Las colas transportan PUNTEROS a RawSignal (no la señal por valor).
    rawRxQueue = xQueueCreate(8, sizeof(RawSignal*));
    rawReplayQueue = xQueueCreate(4, sizeof(RawSignal*));
    rawReceiverParams = startRadioTask(RECEIVE_RAW, rawRxQueue, "RawRx", 4096, &rawReceiverHandle);
    ledsBrightness = prefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
    sendNeopixelSolid(0x0000ff00, ledsBrightness);   // verde: escuchando RAW
    currentMenu = &rawRxList;
}

static void raw_stopReceiver() {
    if (rawReceiverHandle) xTaskNotify(rawReceiverHandle, RADIO_STOP, eSetValueWithOverwrite);
    rawReceiverHandle = NULL;
    sendNeopixelIdle(ledsBrightness);
    // Drenamos capturas pendientes en la cola task->UI para no filtrarlas.
    RawSignal *pending = NULL;
    if (rawRxQueue) {
        while (xQueueReceive(rawRxQueue, &pending, 0) == pdTRUE) freeRawSignal(pending);
        vQueueDelete(rawRxQueue);
        rawRxQueue = NULL;
    }
    // rawReplayQueue solo lleva punteros a señales que siguen en rawMessages
    // (no son copias): se liberan en raw_freeAllMessages(); aquí solo vaciamos.
    if (rawReplayQueue) {
        while (xQueueReceive(rawReplayQueue, &pending, 0) == pdTRUE) { /* no-op */ }
        vQueueDelete(rawReplayQueue);
        rawReplayQueue = NULL;
    }
    raw_freeAllMessages();
    rawRxList.list->clear();
}

static void raw_enterSaved() {
    rawMode = RAW_SAVED;
    row = 0;
    rawShowingFile = false;
    rawTxQueue = xQueueCreate(4, sizeof(RawSignal*));
    rawTxParams = startRadioTask(SEND_RAW, rawTxQueue, "RawTx", 4096, &radioTransmitterTaskHandle);
    raw_fillSavedFiles();
    rawSavedList.selected = 0;
    currentMenu = &rawSavedList;
}

static void raw_stopTransmitter() {
    if (radioTransmitterTaskHandle) xTaskNotify(radioTransmitterTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    radioTransmitterTaskHandle = NULL;
    // Drenamos la cola para no filtrar señales que el task no llegó a enviar.
    RawSignal *pending = NULL;
    if (rawTxQueue) {
        while (xQueueReceive(rawTxQueue, &pending, 0) == pdTRUE) freeRawSignal(pending);
        vQueueDelete(rawTxQueue);
        rawTxQueue = NULL;
    }
    if (rawSavedFiles) { delete rawSavedFiles; rawSavedFiles = NULL; }
}

static void raw_freeAllMessages() {
    if (!rawMessages) return;
    for (int i = 0; i < rawMessages->size(); i++) freeRawSignal(rawMessages->get(i));
    delete rawMessages;
    rawMessages = NULL;
}

static void raw_exitSubMode() {
    switch (rawMode) {
    case RAW_RECEIVE: raw_stopReceiver();    break;
    case RAW_SAVED:   raw_stopTransmitter(); break;
    default: break;
    }
    rawMode = RAW_MENU;
    row = 0;
    rawShowingFile = false;
    currentMenu = &rawModeMenu;
}

// --- Acciones -----------------------------------------------------------------

static void raw_replay() {
    if (!rawMessages || rawMessages->size() == 0) return;
    RawSignal *sig = rawMessages->get(rawRxList.selected);
    xTaskNotify(rawReceiverHandle, REPLAY_RAW, eSetValueWithOverwrite);
    xQueueSend(rawReplayQueue, &sig, 0);   // enviamos el PUNTERO; la señal sigue siendo nuestra
    showPopupMenu("RAW replayed!");
}

static void raw_saveCaptured() {
    if (!rawMessages || rawMessages->size() == 0) return;
    if (rawSaveName.length() == 0) { showPopupMenu("Name is empty."); return; }
    RawSignal *sig = rawMessages->get(rawRxList.selected);
    if (FileUtils::saveRaw(RAW_TRANSCEIVER_PATH, rawSaveName, sig)) {
        showPopupMenu("Saved!");
        rawSaveName = "";
        currentMenu = &rawRxList;
    } else {
        showPopupMenu("Error.");
    }
}

static void raw_fillSavedFiles() {
    rawSavedList.list->clear();
    if (rawSavedFiles) { delete rawSavedFiles; rawSavedFiles = NULL; }
    rawSavedFiles = FileUtils::listFiles(RAW_TRANSCEIVER_PATH);
    if (rawSavedFiles) {
        for (int i = 0; i < rawSavedFiles->size(); i++) {
            String fileName = rawSavedFiles->get(i);
            addMenuNode(&rawSavedList, [fileName](){ return fileName; }, &rawSavedFileMenu);
        }
    }
}

static void raw_sendSelectedFile() {
    String fileName = rawSavedFiles->get(rawSavedList.selected);
    RawSignal *sig = FileUtils::loadRaw(RAW_TRANSCEIVER_PATH, fileName);
    if (!sig) { showPopupMenu("Read error."); return; }
    // Cedemos la propiedad al task: él la libera cuando termina de emitirla.
    xTaskNotify(radioTransmitterTaskHandle, SEND_RAW, eSetValueWithOverwrite);
    if (xQueueSend(rawTxQueue, &sig, 0) != pdTRUE) {
        freeRawSignal(sig);   // no entró en la cola: nadie más la va a liberar
        showPopupMenu("Busy.");
        return;
    }
    showPopupMenu("Signal sent!");
}

static void raw_loadFileContent() {
    String fileName = rawSavedFiles->get(rawSavedList.selected);
    RawSignal *sig = FileUtils::loadRaw(RAW_TRANSCEIVER_PATH, fileName);
    rawFileContent.name = fileName;
    rawFileContent.size = FileUtils::getFileSize(RAW_TRANSCEIVER_PATH, fileName);
    if (!sig) {
        rawFileContent.count = 0;
        rawFileContent.frequency = "?";
        rawFileContent.preset = "?";
        return;
    }
    rawFileContent.count = sig->count;
    rawFileContent.frequency = String(getFrequencyFromEnum(sig->frequency));
    rawFileContent.preset = getPresetNameFromEnum(sig->preset);
    freeRawSignal(sig);
}

static void raw_removeSelectedFile() {
    String deleteFile = rawSavedFiles->get(rawSavedList.selected);
    if (FileUtils::remove(RAW_TRANSCEIVER_PATH, deleteFile)) {
        showPopupMenu("File deleted");
        raw_fillSavedFiles();
        rawSavedList.selected = 0;
    } else {
        showPopupMenu("Delete error");
    }
    currentMenu = &rawSavedList;
}

// --- Construcción de menús ----------------------------------------------------

static void raw_buildMenus() {
    createMenu(&rawModeMenu, NULL, [](){
        addMenuNode(&rawModeMenu, &SIMPLE_RECEIVE_ICON, MENU_ITEM_RECEIVE, [](){ raw_enterReceive(); });
        addMenuNode(&rawModeMenu, &SAVE_ICON, MENU_ITEM_SAVED, [](){ raw_enterSaved(); });
    });
    createDynamicMenu(&rawRxList, &rawModeMenu, [](){
        return "RAW " + String(getFrequencyFromEnum(rawReceiverParams->frequency)) + " " + getPresetNameFromEnum(rawReceiverParams->preset);
    }, [](){});
    createMenu(&rawRxSignalMenu, &rawRxList, [](){
        addMenuNode(&rawRxSignalMenu, &REPLAY_ICON, MENU_ITEM_REPLAY, [](){ raw_replay(); });
        addMenuNode(&rawRxSignalMenu, &SAVE_ICON, MENU_ITEM_SAVE, &rawSaveNameMenu);
    });
    createMenu(&rawSaveNameMenu, &rawRxSignalMenu, [](){
        addMenuNode(&rawSaveNameMenu, [](){ return "Name: " + rawSaveName; }, [](){ startKeyboard(&rawSaveName); });
        addMenuNode(&rawSaveNameMenu, "Accept", [](){ raw_saveCaptured(); });
    });
    createDynamicMenu(&rawSavedList, &rawModeMenu, [](){
        return "RAW " + String(getFrequencyFromEnum(rawTxParams->frequency)) + " " + getPresetNameFromEnum(rawTxParams->preset);
    }, [](){});
    createMenu(&rawSavedFileMenu, &rawSavedList, [](){
        addMenuNode(&rawSavedFileMenu, &PLAY_ICON, MENU_ITEM_SEND_SIGNAL, [](){ raw_sendSelectedFile(); });
        addMenuNode(&rawSavedFileMenu, &READ_FILE_ICON, MENU_ITEM_READ_FILE, [](){ raw_loadFileContent(); rawShowingFile = true; });
        addMenuNode(&rawSavedFileMenu, &DELETE_ICON, MENU_ITEM_DELETE, [](){ raw_removeSelectedFile(); });
    });
    rawModeMenu.build();
    rawRxSignalMenu.build();
    rawSaveNameMenu.build();
    rawSavedFileMenu.build();
    // rawRxList y rawSavedList se rellenan al entrar en su modo.
}

App app_raw = {
  .name = "Raw",
  .onStart = raw_onStart,
  .onEvent = raw_onEvent,
  .onDraw = raw_onDraw,
  .onStop = raw_onStop
};
