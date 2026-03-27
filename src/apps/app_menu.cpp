#include "literals.h"
#include "app.h"
#include "config/io_config.h"
#include "config/constants.h"
#include "app_menu.h"

#include "tasks/ui_task.h"
#include "tasks/radio_task.h"
#include "tasks/leds_task.h"
#include "utils/menu.h"
#include "utils/radio_utils.h"

extern App app_splash;
extern App app_snake;
extern App app_jammer;
extern App app_radio_receive;
extern App app_wifi_beacon_spam;
extern App app_about;
extern App app_simple_tx;

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
Menu profileSettingsMenu;

extern Menu* currentMenu;

int row;
extern Preferences prefs;
SettingsValue frequencySelectedConfig;
SettingsValue radioPresetConfig;
SettingsValue neopixelBrightnessConfig;
uint32_t availableRadio = 0;
UserInfo userInfoSettings;

void menu_onStart() {
    row = 0;
    prefs.begin("configuration", false);
    // Default config if not saved values: 433MHz - OOK - 650Khz bw
    frequencySelectedConfig.current = prefs.getUChar("frequency", FREQ_433MHZ);
    radioPresetConfig.current = prefs.getUChar("preset", PRESET_AM650);
    neopixelBrightnessConfig.current = prefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
    frequencySelectedConfig.max = FREQ_915MHZ;
    radioPresetConfig.max = PRESET_FM476;
    neopixelBrightnessConfig.max = 10;
    userInfoSettings.name = prefs.getString("user_name", "John Doe");
    userInfoSettings.nick = prefs.getString("user_nick", "johndoe");
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
            addMenuNode(&subghzMenu, &DOWN_ICON, MENU_ITEM_RECEIVE, &radioReceiveMenu);
        } else {
            addMenuNode(&subghzMenu, &ERROR_ICON, MENU_ITEM_RADIO_NOT_FOUND, &mainMenu);
        }
    });
    // Radio transmit submenu
    createMenu(&radioTransmitMenu, &subghzMenu, []() {
        addMenuNode(&radioTransmitMenu, &MEGAPHONE_ICON, MENU_ITEM_JAMMER, &subghzMenu, &app_jammer);
        addMenuNode(&radioTransmitMenu, &SIMPLE_TRANSMIT_ICON, MENU_ITEM_SIMPLE_TX, &subghzMenu, &app_simple_tx);
    });
    // Radio receive submenu
    createMenu(&radioReceiveMenu, &subghzMenu, []() {
        addMenuNode(&radioReceiveMenu, &SIMPLE_RECEIVE_ICON, MENU_ITEM_SIMPLE_RX, &subghzMenu, &app_radio_receive);
    });
    // Settings submenu
    createMenu(&settingsMenu, &mainMenu, []() {
        addMenuNode(&settingsMenu, &RADIO_ICON, MENU_ITEM_RADIO, &radioSettingsMenu);
        addMenuNode(&settingsMenu, &RGB_ICON, MENU_ITEM_RGB, &neopixelSettingsMenu);
        addMenuNode(&settingsMenu, &USER_ICON, MENU_ITEM_PROFILE, &profileSettingsMenu);
        addMenuNode(&settingsMenu, &ABOUT_ICON, MENU_ITEM_ABOUT, &subghzMenu, &app_about);
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
    // Profile settings submenu
    createMenu(&profileSettingsMenu, &settingsMenu, []() {
        addMenuNode(&profileSettingsMenu, [](){ return "Name: " + userInfoSettings.name; }, [](){ startKeyboard(&userInfoSettings.name); /* click() */});
        addMenuNode(&profileSettingsMenu, [](){ return "Nick: " + userInfoSettings.nick; }, [](){ startKeyboard(&userInfoSettings.nick); /* click() */});
        addMenuNode(&profileSettingsMenu, "Apply changes", &saveProfileConfig);
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
    radioReceiveMenu.build();
    bleMenu.build();
    wifiMenu.build();
    radioSettingsMenu.build();
    neopixelSettingsMenu.build();
    profileSettingsMenu.build();
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
    u8g2->clearBuffer();
    row = drawMenu(u8g2, currentMenu, row);
    u8g2->sendBuffer();
}

void saveRadioConfig() {
    int ok = 0;
    ok += prefs.putUChar("frequency", frequencySelectedConfig.current);
    ok += prefs.putUChar("preset", radioPresetConfig.current);
    if (ok >= 2) showPopupMenu("Saved!");
    else showPopupMenu("Failed.");
}

void saveNeopixelConfig() {
    int ok =  prefs.putUChar("brightness", neopixelBrightnessConfig.current);
    NeopixelConfiguration neopixelConfig;
    neopixelConfig.brightness = neopixelBrightnessConfig.current;
    neopixelConfig.operation = RANDOM_ALL;
    for (int i = 0; i < NUM_LEDS; i++) neopixelConfig.colors[i] = 0;
    sendNeopixelConfig(neopixelConfig);
    if (ok) showPopupMenu("Saved!");
    else showPopupMenu("Failed.");
}
void saveProfileConfig() {
    int ok =  prefs.putString("user_name", userInfoSettings.name);
    ok += prefs.putString("user_nick", userInfoSettings.nick);
    if (ok >= 2) showPopupMenu("Saved!");
    else showPopupMenu("Failed.");
}

App app_menu = {
  .name = "Menu",
  .onStart = menu_onStart,
  .onEvent = menu_onEvent,
  .onDraw = menu_onDraw,
  .onStop = menu_onStop
};