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
#include "utils/menu.h"

extern App app_menu;
QueueHandle_t queue;
TaskHandle_t radioReceiverTaskHandle = NULL;
bool emptyList, onSelectedMenu;
RadioTaskParams *receiverParams;
extern Preferences prefs;
extern uint8_t ledsBrightness;
Menu receivedSignalsMenu;
SimpleList <RFMessage> *receivedMessages;
int selectedSignalIndex = 0;
extern int row;
extern Menu* currentMenu;

void radio_receive_onStart() {
    onSelectedMenu = false;
    receivedMessages = new SimpleList<RFMessage>;
    createMenu(&receivedSignalsMenu, NULL, [](){
        addMenuNode(&receivedSignalsMenu, &REPLAY_ICON, MENU_ITEM_REPLAY, [](){ replaySignal(); });
        addMenuNode(&receivedSignalsMenu, &SAVE_ICON, MENU_ITEM_SAVE, [](){ Serial.println("Save signal");}/* TODO: Implement save functionality */);
    });
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
    receivedSignalsMenu.build();
}

void radio_receive_onStop() {
    xTaskNotify(radioReceiverTaskHandle, RADIO_STOP, eSetValueWithOverwrite);
    sendNeopixelConfig(NeopixelConfiguration{RANDOM_ALL, ledsBrightness, {0,0,0}});
    vQueueDelete(queue);
}

void radio_receive_onDraw(U8G2 *u8g2) {
    // messy and shitty code for testing, TODO: implement properly
    RFMessage msg;
    u8g2->setDrawColor(1);
    if (onSelectedMenu) {
        row = drawMenu(u8g2, &receivedSignalsMenu, row);
    } else {
        if (emptyList) {
            u8g2->clearBuffer();
            u8g2->drawXBM(3, 0, bat_rx_width, bat_rx_height, bat_rx_bits);
            u8g2->setFont(u8g2_font_7x14_tr);
            u8g2->drawStr(40, 10, "Listening at");
            u8g2->drawStr(55, 25, (String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz ").c_str());
            u8g2->drawStr(80, 40, getPresetNameFromEnum(receiverParams->preset).c_str());
            emptyList = false;
            u8g2->sendBuffer();
        } else if (receivedMessages->size() > 0) {
            drawReceivedSignalsList(u8g2);
        }
    }
    if (xQueueReceive(queue, &msg, 0) == pdTRUE) {
        receivedMessages->add(msg);
    }
}
void radio_receive_onEvent(int evt) {
    if (evt == BTN_BACK) {
        if (onSelectedMenu) {
            onSelectedMenu = false;
        } else {
            radio_receive_onStop();
            extern App *currentApp;
            currentApp = &app_menu;
            currentApp->onStart();   
        }
    } else if (evt == BTN_OK) {
        if (receivedMessages->size() > 0 && !onSelectedMenu) {
            onSelectedMenu = true;
        } else if (onSelectedMenu) {
            receivedSignalsMenu.list->get(receivedSignalsMenu.selected).click();
        }
    } else if (evt == BTN_UP) {
        if (onSelectedMenu) {
            receivedSignalsMenu.selected--;
            if (receivedSignalsMenu.selected < 0) receivedSignalsMenu.selected = 0;
        }else {
            selectedSignalIndex--;
            if (selectedSignalIndex < 0) selectedSignalIndex = 0;
        }
    } else if (evt == BTN_DOWN) {
        if (onSelectedMenu) {
            receivedSignalsMenu.selected++;
            if (receivedSignalsMenu.selected >= receivedSignalsMenu.list->size()) receivedSignalsMenu.selected = receivedSignalsMenu.list->size() - 1;
        } else {
            selectedSignalIndex++;
            if (selectedSignalIndex >= receivedMessages->size()) selectedSignalIndex = receivedMessages->size() - 1;
        } 
    } 
}

void drawReceivedSignalsList(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_t0_11_tr);
    u8g2->drawStr(0, 8, (String(getFrequencyFromEnum(receiverParams->frequency)) + "MHz " + getPresetNameFromEnum(receiverParams->preset)).c_str());
    u8g2->drawStr(95, 8, (String(selectedSignalIndex + 1) + "/" + String(receivedMessages->size())).c_str());
    u8g2->setFont(u8g2_font_7x14_tr);

    const int visibleCount = 3;  // número de líneas visibles en pantalla
    String tmp;
    int tmpLen;

    int total = receivedMessages->size();

    // --- Seguridad: evitar índices fuera de rango ---
    if (selectedSignalIndex < 0)
        selectedSignalIndex = total - 1; // wrap around to the bottom
    else if (selectedSignalIndex >= total)
        selectedSignalIndex = 0; // wrap around to the top

    // --- Ajuste automático del scroll vertical (row) ---
    // Mueve la "ventana" visible cuando el elemento seleccionado sale del rango actual
    if (selectedSignalIndex >= row + visibleCount)
        row = selectedSignalIndex - visibleCount + 1;
    else if (selectedSignalIndex < row)
        row = selectedSignalIndex;

    // --- Límite inferior (scroll hacia arriba) ---
    if (row < 0) row = 0;

    // --- Límite superior (scroll hacia abajo) ---
    // Si hay más elementos que líneas visibles, el máximo desplazamiento es total - visibleCount
    if (total > visibleCount) {
        if (row > total - visibleCount)
            row = total - visibleCount;
    } else {
        // Si caben todos, mantener siempre en 0
        row = 0;
    }
    // --- Dibujar los ítems visibles ---
    u8g2->setFont(u8g2_font_t0_12_mr);
    for (int i = row; i < total && i < row + visibleCount; i++) {
        RFMessage msg = receivedMessages->get(i);
        String msgStr = "P" + String(msg.protocol) +" V" + String(msg.value, HEX)+ " L" + String(msg.length);
        int drawColor = selectedSignalIndex == i ? 1 : 0;
        u8g2->setDrawColor(drawColor);
        u8g2->drawBox(0, (i - row + 1) * 14, 128, 16);
        u8g2->setDrawColor(!drawColor);
        u8g2->drawStr(2, (i - row + 2) * 14 - 2, msgStr.c_str());
    }
    u8g2->sendBuffer();
}

void replaySignal() {
    if (receivedMessages->size() == 0) return;
    RFMessage msg = receivedMessages->get(selectedSignalIndex);
    xTaskNotify(radioReceiverTaskHandle, RADIO_REPLAY_SIGNAL, eSetValueWithOverwrite);
    xQueueSend(queue, &msg, 0);
    showPopupMenu("Signal replayed!");
}

App app_radio_receive = {
  .name = "Radio Receive",
  .onStart = radio_receive_onStart,
  .onEvent = radio_receive_onEvent,
  .onDraw = radio_receive_onDraw,
  .onStop = radio_receive_onStop
};