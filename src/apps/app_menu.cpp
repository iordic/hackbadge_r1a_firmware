#include "literals.h"
#include "app.h"
#include "config/io_config.h"
#include "devices/display.h"
#include "app_menu.h"

#include "tasks/radio_task.h"
#include "utils/menu.h"
#include "utils/radio_utils.h"

extern App app_splash;
extern App app_snake;
extern App app_jammer;
extern App app_radio_receive;
extern App app_wifi_beacon_spam;

Menu mainMenu;
// Submenus
Menu subghzMenu;
Menu bleMenu;
Menu wifiMenu;
Menu gamesMenu;
Menu settingsMenu;
// Subghz submenus
Menu radioTransmitMenu;
Menu radioReceiveMenu;
// settings submenus
Menu radioSettingsMenu;
Menu neopixelSettingsMenu;

extern Menu* currentMenu;

int row;
Preferences prefs;
SettingsValue frequencySelectedConfig;
SettingsValue radioPresetConfig;
SettingsValue neopixelBrightnessConfig;
uint32_t availableRadio = 0;

void menu_onStart() {
    row = 0;
    prefs.begin("configuration", false);
    // Default config if not saved values: 433MHz - OOK - 650Khz bw
    frequencySelectedConfig.current = prefs.getUChar("frequency", FREQ_433MHZ);
    radioPresetConfig.current = prefs.getUChar("preset", PRESET_AM650);
    neopixelBrightnessConfig.current = prefs.getUChar("brightness", NEOPIXEL_BRIGHTNESS / 10);
    frequencySelectedConfig.max = FREQ_915MHZ;
    radioPresetConfig.max = PRESET_FM476;
    neopixelBrightnessConfig.max = 10;
    // Main menu
    createMenu(&mainMenu, NULL, []() {
        addMenuNode(&mainMenu, &WAVE_ICON, MENU_ITEM_SUBGHZ, &app_splash, &subghzMenu);
        addMenuNode(&mainMenu, &BLUETOOTH_ICON, MENU_ITEM_BLE, &app_splash, &bleMenu);
        addMenuNode(&mainMenu, &WIFI_ICON, MENU_ITEM_WIFI, &app_splash, &wifiMenu);
        addMenuNode(&mainMenu, &PUZZLE_ICON, MENU_ITEM_GAMES, &app_splash, &gamesMenu);
        addMenuNode(&mainMenu, &WRENCH_ICON, MENU_ITEM_SETTINGS, &app_splash, &settingsMenu);
    });
    // Games submenu
    createMenu(&gamesMenu, &mainMenu, []() {
        addMenuNode(&gamesMenu, &CURSOR_DOWN_ICON, MENU_ITEM_SNAKE, &mainMenu, &app_snake);
    });
    // Subghz submenu
    createMenu(&subghzMenu, &mainMenu, []() {
        RadioTaskParams *params = (RadioTaskParams *) malloc(sizeof(RadioTaskParams));
        params->operation = CHECK;
        params->callerHandle = xTaskGetCurrentTaskHandle();
        xTaskCreatePinnedToCore(radio_task, "RadioCheckWorker", 2048, params, 1, NULL, 1);
        xTaskNotifyWait(0, 0, &availableRadio, portMAX_DELAY);
        if (availableRadio) {
            addMenuNode(&subghzMenu, &UP_ICON, MENU_ITEM_TRANSMIT, &radioTransmitMenu);
            addMenuNode(&subghzMenu, &DOWN_ICON, MENU_ITEM_RECEIVE, &mainMenu, &app_radio_receive);
        } else {
            addMenuNode(&subghzMenu, &ERROR_ICON, MENU_ITEM_RADIO_NOT_FOUND, &mainMenu);
        }
    });
    // Radio transmit submenu
    createMenu(&radioTransmitMenu, &subghzMenu, []() {
        addMenuNode(&radioTransmitMenu, &MEGAPHONE_ICON, MENU_ITEM_JAMMER, &subghzMenu, &app_jammer);
    });
    // Settings submenu
    createMenu(&settingsMenu, &mainMenu, []() {
        addMenuNode(&settingsMenu, &RADIO_ICON, MENU_ITEM_RADIO, &radioSettingsMenu);
        addMenuNode(&settingsMenu, &RGB_ICON, MENU_ITEM_RGB, &neopixelSettingsMenu);
    });
    // Radio settings submenu
    createMenu(&radioSettingsMenu, &settingsMenu, []() {
        addMenuNodeSetting(&radioSettingsMenu, "Freq.   ", &frequencySelectedConfig, [](uint8_t value){ return String(getFrequencyFromEnum(value));}, &settingsMenu);
        addMenuNodeSetting(&radioSettingsMenu, "Preset   ", &radioPresetConfig, [](uint8_t value){ return getPresetNameFromEnum(value);}, &settingsMenu);
        addMenuNode(&radioSettingsMenu, "Apply changes", &saveRadioConfig);
    });
    // Neopixel settings submenu
    createMenu(&neopixelSettingsMenu, &settingsMenu, []() {
        addMenuNodeSetting(&neopixelSettingsMenu, "Brightness ", &neopixelBrightnessConfig, [](uint8_t value){ return String(value);}, &settingsMenu);
        addMenuNode(&neopixelSettingsMenu, "Apply changes", &saveNeopixelConfig);
    });
    // BLE submenu
    createMenu(&bleMenu, &mainMenu, []() {
        addMenuNode(&bleMenu, &FORBIDDEN_ICON, MENU_ITEM_NOT_IMPLEMENTED, &mainMenu);
    });
    // WiFi submenu
    createMenu(&wifiMenu, &mainMenu, []() {
        addMenuNode(&wifiMenu, &SPAM_ICON, MENU_ITEM_WIFI_BEACON_SPAM, &mainMenu, &app_wifi_beacon_spam);
    });

    if (!currentMenu) currentMenu = &mainMenu;
    currentMenu->selected = 0;
    mainMenu.build();
    subghzMenu.build();
    gamesMenu.build();
    settingsMenu.build();
    radioTransmitMenu.build();
    bleMenu.build();
    wifiMenu.build();
    radioSettingsMenu.build();
    neopixelSettingsMenu.build();
}

void menu_onStop() {
    prefs.end();
}

void menu_onEvent(int evt) {
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

void menu_onDraw(U8G2 *u8g2) {
    uint8_t xStartWritting = 0;
    u8g2->clearBuffer();
    const int visibleCount = 4;  // número de líneas visibles en pantalla
    String tmp;
    int tmpLen;

    int total = currentMenu->list->size();

    // --- Seguridad: evitar índices fuera de rango ---
    if (currentMenu->selected < 0)
        currentMenu->selected = total - 1; // wrap around to the bottom
    else if (currentMenu->selected >= total)
        currentMenu->selected = 0; // wrap around to the top

    // --- Ajuste automático del scroll vertical (row) ---
    // Mueve la "ventana" visible cuando el elemento seleccionado sale del rango actual
    if (currentMenu->selected >= row + visibleCount)
        row = currentMenu->selected - visibleCount + 1;
    else if (currentMenu->selected < row)
        row = currentMenu->selected;

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
    for (int i = row; i < total && i < row + visibleCount; i++) {
        xStartWritting = 2;
        int drawColor = currentMenu->selected == i ? 1 : 0;
        u8g2->setDrawColor(drawColor);
        u8g2->drawBox(0, (i - row) * 16, 128, 16);
        u8g2->setDrawColor(!drawColor);
        if (currentMenu->list->get(i).getIcon()) {
            u8g2->setFont(u8g2_font_open_iconic_all_2x_t);
            u8g2->drawGlyph(xStartWritting, (i - row + 1) * 16, currentMenu->list->get(i).getIcon());
            xStartWritting = 21;
        }
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(xStartWritting, (i - row + 1) * 16 - 2, currentMenu->list->get(i).getStr().c_str());
    }
    u8g2->sendBuffer();
}

void showPopupMenu(const char* message) {
    U8G2 *u8g2 = display_get();
    u8g2->setDrawColor(0);
    u8g2->drawRBox(10, 10, 110, 40, 2);
    u8g2->setDrawColor(1);
    u8g2->drawRFrame(12, 12, 106, 36, 2);
    u8g2->drawRFrame(10, 10, 110, 40, 5);
    u8g2->setFont(u8g2_font_7x14_mr);
    int16_t strWidth = u8g2->getStrWidth(message);
    //u8g2->drawBox((128 - strWidth - 12) / 2, 12, strWidth + 12, 30);
    u8g2->drawStr((128 - strWidth) / 2, 36, message);
    u8g2->sendBuffer();
}

void saveRadioConfig() {
    int ok = 0;
    ok += prefs.putUChar("frequency", frequencySelectedConfig.current);
    ok += prefs.putUChar("preset", radioPresetConfig.current);
    if (ok >= 2) showPopupMenu("Saved!");
    else showPopupMenu("Failed.");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void saveNeopixelConfig() {
    int ok =  prefs.putUChar("brightness", neopixelBrightnessConfig.current);
    if (ok) showPopupMenu("Saved!");
    else showPopupMenu("Failed.");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

App app_menu = {
  .name = "Menu",
  .onStart = menu_onStart,
  .onEvent = menu_onEvent,
  .onDraw = menu_onDraw,
  .onStop = menu_onStop
};