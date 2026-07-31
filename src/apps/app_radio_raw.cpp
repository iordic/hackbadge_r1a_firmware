#include "app.h"
#include "app_radio_raw.h"
#include "app_radio_receive.h"   // reutilizamos drawRssi()
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
extern Preferences prefs;
extern uint8_t ledsBrightness;
extern int row;
extern Menu* currentMenu;

QueueHandle_t rawQueue;        // task->UI: capturas nuevas (punteros RawSignal*)
QueueHandle_t rawReplayQueue;  // UI->task: peticiones de replay (punteros RawSignal*)
TaskHandle_t rawReceiverTaskHandle = NULL;
RadioTaskParams *rawReceiverParams;
Menu mainListRawSignals;
Menu rawSignalMenu;
Menu rawSaveFileMenu;
// Guardamos punteros: la señal vive en heap (la crea radio_task) y la libera esta app.
SimpleList<RawSignal*> *rawMessages;
String rawSaveFileName = "";

static void raw_freeAllMessages() {
    if (!rawMessages) return;
    for (int i = 0; i < rawMessages->size(); i++) {
        freeRawSignal(rawMessages->get(i));
    }
    delete rawMessages;
    rawMessages = NULL;
}

void radio_raw_onStart() {
    currentMenu = &mainListRawSignals;
    rawMessages = new SimpleList<RawSignal*>;
    rawReceiverParams = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
    rawReceiverParams->operation = RECEIVE_RAW;
    rawReceiverParams->frequency = prefs.getUChar("frequency", FREQ_433MHZ);
    rawReceiverParams->preset = prefs.getUChar("preset", PRESET_AM650);
    // Las colas transportan PUNTEROS a RawSignal (no la señal por valor).
    rawQueue = xQueueCreate(8, sizeof(RawSignal*));
    rawReplayQueue = xQueueCreate(4, sizeof(RawSignal*));
    rawReceiverParams->queueHandle = rawQueue;
    rawReceiverParams->callerHandle = xTaskGetCurrentTaskHandle();

    ledsBrightness = prefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
    NeopixelConfiguration config;
    config.operation = FIXED_COLOR;
    config.brightness = ledsBrightness;
    for (int i = 0; i < NUM_LEDS; i++) config.colors[i] = 0x0000ff00; // verde: modo RAW
    sendNeopixelConfig(config);

    xTaskCreatePinnedToCore(radio_task, "RawReceiverWorker", 4096, rawReceiverParams, 5, &rawReceiverTaskHandle, 1);

    createDynamicMenu(&mainListRawSignals, NULL, [](){
        return "RAW " + String(getFrequencyFromEnum(rawReceiverParams->frequency)) + " " + getPresetNameFromEnum(rawReceiverParams->preset);
    }, [](){});
    createMenu(&rawSignalMenu, &mainListRawSignals, [](){
        addMenuNode(&rawSignalMenu, &REPLAY_ICON, MENU_ITEM_REPLAY, [](){ raw_replay(); });
        addMenuNode(&rawSignalMenu, &SAVE_ICON, MENU_ITEM_SAVE, &rawSaveFileMenu);
    });
    createMenu(&rawSaveFileMenu, &rawSignalMenu, []() {
        addMenuNode(&rawSaveFileMenu, [](){ return "Name: " + rawSaveFileName; }, [](){ startKeyboard(&rawSaveFileName); });
        addMenuNode(&rawSaveFileMenu, "Accept", &raw_saveSignal);
    });
    rawSignalMenu.build();
    rawSaveFileMenu.build();
}

void radio_raw_onStop() {
    xTaskNotify(rawReceiverTaskHandle, RADIO_STOP, eSetValueWithOverwrite);

    NeopixelConfiguration neopixelConfig;
    neopixelConfig.brightness = ledsBrightness;
    neopixelConfig.operation = RANDOM_ALL;
    for (int i = 0; i < NUM_LEDS; i++) neopixelConfig.colors[i] = 0;
    sendNeopixelConfig(neopixelConfig);

    // Drenamos punteros pendientes en la cola para no filtrarlos.
    RawSignal *pending = NULL;
    while (xQueueReceive(rawQueue, &pending, 0) == pdTRUE) {
        freeRawSignal(pending);
    }
    vQueueDelete(rawQueue);
    // rawReplayQueue solo lleva punteros a señales que siguen en rawMessages
    // (no son copias), así que se liberan en raw_freeAllMessages(); aquí solo
    // vaciamos y destruimos la cola.
    while (xQueueReceive(rawReplayQueue, &pending, 0) == pdTRUE) { /* no-op */ }
    vQueueDelete(rawReplayQueue);
    raw_freeAllMessages();
    currentMenu = NULL;
}

void radio_raw_onDraw(U8G2 *u8g2) {
    RawSignal *sig = NULL;
    u8g2->setDrawColor(1);
    u8g2->clearBuffer();
    if (rawMessages->size() == 0) {
        u8g2->drawXBM(3, 0, bat_rx_width, bat_rx_height, bat_rx_bits);
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(40, 10, "RAW listen");
        u8g2->drawStr(55, 25, (String(getFrequencyFromEnum(rawReceiverParams->frequency)) + "MHz ").c_str());
        u8g2->drawStr(80, 40, getPresetNameFromEnum(rawReceiverParams->preset).c_str());
    } else {
        row = drawMenu(u8g2, currentMenu, row);
    }
    if (xQueueReceive(rawQueue, &sig, 0) == pdTRUE && sig) {
        rawMessages->add(sig);
        String label = "RAW " + String(sig->count) + "ch";
        addMenuNode(&mainListRawSignals, label, &app_menu, &rawSignalMenu);
    }
    drawRssi(u8g2);
    u8g2->sendBuffer();
}

void radio_raw_onEvent(int evt) {
    if (rawMessages->size() == 0) {
        if (evt == BTN_BACK) {
            changeAppContext(&app_menu);
        }
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

void raw_replay() {
    if (rawMessages->size() == 0) return;
    RawSignal *sig = rawMessages->get(mainListRawSignals.selected);
    xTaskNotify(rawReceiverTaskHandle, REPLAY_RAW, eSetValueWithOverwrite);
    xQueueSend(rawReplayQueue, &sig, 0);   // enviamos el PUNTERO; la señal sigue siendo nuestra
    showPopupMenu("RAW replayed!");
}

void raw_saveSignal() {
    if (rawMessages->size() == 0) return;
    if (rawSaveFileName.length() == 0) {
        showPopupMenu("Name is empty.");
        return;
    }
    RawSignal *sig = rawMessages->get(mainListRawSignals.selected);
    if (FileUtils::saveRaw(RAW_TRANSCEIVER_PATH, rawSaveFileName, sig)) {
        showPopupMenu("Saved!");
        rawSaveFileName = "";
        currentMenu = &mainListRawSignals;
    } else {
        showPopupMenu("Error.");
    }
}

App app_radio_raw = {
  .name = "Radio Raw",
  .onStart = radio_raw_onStart,
  .onEvent = radio_raw_onEvent,
  .onDraw = radio_raw_onDraw,
  .onStop = radio_raw_onStop
};
