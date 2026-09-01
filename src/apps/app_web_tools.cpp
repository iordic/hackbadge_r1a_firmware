#include "app.h"
#include "apps.h"
#include "literals.h"
#include <Preferences.h>
#include "config/constants.h"
#include "devices/display.h"
#include "tasks/ui_task.h"          // startKeyboard, currentApp
#include "tasks/web_tools_task.h"   // WtState, WEB_MODE_*, startWebToolsTask, ...
#include "utils/menu.h"             // Menu, drawMenu, menuHandleEvent, showPopupMenu

// La app tiene dos estados: CONFIG (elegir modo AP/STA + credenciales y arrancar)
// y RUNNING (portal levantado, se muestra estado/IP/token). El worker
// web_tools_task hace la red; aquí solo se dibuja y se gestionan botones. Calca
// el patrón de app_evil_portal.
enum WtState { WT_CONFIG, WT_RUNNING };
static WtState wt_state = WT_CONFIG;

extern Preferences prefs; // abierto por app_menu, compartido (como radio/leds)

static uint8_t wt_mode = WEB_MODE_AP;
static String  wt_apSsid, wt_apPass, wt_staSsid, wt_staPass;

static Menu wt_menu;
static int  wt_row = 0;

// Enmascara una contraseña para no mostrarla en claro en el OLED (se edita entera
// con el teclado; aquí solo pintamos puntos).
static String mask(const String &s) {
    if (s.isEmpty()) return String("(unset)");
    String out;
    for (size_t i = 0; i < s.length(); i++) out += '*';
    return out;
}

static String truncate(const String &s, uint8_t maxChars) {
    if (s.length() <= maxChars) return s;
    return s.substring(0, maxChars);
}

static void loadNetConfig() {
    wt_mode = prefs.getUChar("web_mode", WEB_MODE_AP);
    wt_apSsid = prefs.getString("ap_ssid", WEBTOOLS_AP_SSID);
    wt_apPass = prefs.getString("ap_pass", WEBTOOLS_AP_PASS);
    wt_staSsid = prefs.getString("wifi_ssid", "");
    wt_staPass = prefs.getString("wifi_pass", "");
}

static void persistNetConfig() {
    prefs.putUChar("web_mode", wt_mode);
    prefs.putString("ap_ssid", wt_apSsid);
    prefs.putString("ap_pass", wt_apPass);
    prefs.putString("wifi_ssid", wt_staSsid);
    prefs.putString("wifi_pass", wt_staPass);
}

static void exitToMenu() {
    stopWebToolsTask();
    extern App *currentApp;
    currentApp = &app_menu;
    currentApp->onStart();
}

static void startWebTools() {
    if (wt_mode == WEB_MODE_AP && wt_apSsid.isEmpty()) wt_apSsid = WEBTOOLS_AP_SSID;
    if (wt_mode == WEB_MODE_STA && wt_staSsid.isEmpty()) {
        showPopupMenu("Set WiFi SSID");
        return;
    }
    persistNetConfig();
    if (startWebToolsTask(wt_mode, wt_staSsid, wt_staPass, wt_apSsid, wt_apPass)) wt_state = WT_RUNNING;
    else showPopupMenu("Start failed");
}

// Reconstruye el menú CONFIG según el modo elegido: solo se muestran los campos
// del modo activo (AP -> AP SSID/Pass; STA -> WiFi SSID/Pass). Alternar el modo
// llama aquí para rehacer la lista. get() del menú devuelve el nodo por valor, así
// que recrear la lista desde el propio callback left/right es seguro.
static void buildWtMenu() {
    createMenu(&wt_menu, NULL, []() {
        // Modo AP/STA: scroll lateral (< >) alterna y reconstruye. BACK sale.
        wt_menu.list->add(MenuNode{
            []() -> uint16_t { return 0; },
            []() { return String("Mode <") + (wt_mode == WEB_MODE_AP ? "AP" : "STA") + ">"; },
            []() {},
            []() { exitToMenu(); },
            []() { wt_mode = (wt_mode == WEB_MODE_AP) ? WEB_MODE_STA : WEB_MODE_AP; buildWtMenu(); },
            []() { wt_mode = (wt_mode == WEB_MODE_AP) ? WEB_MODE_STA : WEB_MODE_AP; buildWtMenu(); }
        });
        if (wt_mode == WEB_MODE_AP) {
            addMenuNode(&wt_menu, []() { return String("AP SSID: ") + wt_apSsid; },
                        []() { startKeyboard(&wt_apSsid); }, []() { exitToMenu(); });
            addMenuNode(&wt_menu, []() { return String("AP Pass: ") + mask(wt_apPass); },
                        []() { startKeyboard(&wt_apPass); }, []() { exitToMenu(); });
        } else {
            addMenuNode(&wt_menu, []() { return String("WiFi SSID: ") + wt_staSsid; },
                        []() { startKeyboard(&wt_staSsid); }, []() { exitToMenu(); });
            addMenuNode(&wt_menu, []() { return String("WiFi Pass: ") + mask(wt_staPass); },
                        []() { startKeyboard(&wt_staPass); }, []() { exitToMenu(); });
        }
        addMenuNode(&wt_menu, []() { return String("Start portal"); },
                    []() { startWebTools(); }, []() { exitToMenu(); });
    });
    wt_menu.build();
    wt_menu.selected = 0;
}

void web_tools_onStart() {
    wt_state = WT_CONFIG;
    wt_row = 0;
    loadNetConfig();
    buildWtMenu();
}

void web_tools_onStop() { stopWebToolsTask(); }

void web_tools_onEvent(int evt) {
    if (wt_state == WT_CONFIG) {
        menuHandleEvent(&wt_menu, evt);
    } else { // WT_RUNNING
        if (evt == BTN_BACK) {
            stopWebToolsTask();
            wt_state = WT_CONFIG;
            wt_menu.selected = 0;
        }
    }
}

static void drawRunning(U8G2 *u8g2) {
    u8g2->setFont(u8g2_font_6x13_tr);
    u8g2->drawStr(0, 11, "WEB TOOLS");
    u8g2->drawHLine(0, 15, 128);
    u8g2->setFont(u8g2_font_5x8_tr);

    uint8_t st = webGetStatus();
    if (st == WEB_STARTING) {
        if (wt_mode == WEB_MODE_STA) {
            u8g2->drawStr(0, 30, "Connecting to WiFi...");
            u8g2->drawStr(0, 42, truncate(String("SSID: ") + wt_staSsid, 25).c_str());
        } else {
            u8g2->drawStr(0, 30, "Starting AP...");
        }
        return;
    }
    if (st == WEB_FAILED) {
        u8g2->drawStr(0, 30, "Connection failed.");
        u8g2->drawStr(0, 42, "BACK to change config");
        return;
    }

    char ip[24], token[7];
    webGetIp(ip, sizeof(ip));
    webGetToken(token, sizeof(token));

    bool sta = (st == WEB_RUNNING_STA);
    u8g2->drawStr(0, 26, sta ? "Mode: STA" : "Mode: AP");
    if (sta) {
        u8g2->drawStr(0, 35, "http://" WEBTOOLS_MDNS_HOST ".local");
        u8g2->drawStr(0, 44, (String("IP: ") + ip).c_str());
    } else {
        u8g2->drawStr(0, 35, "Browse to:");
        u8g2->drawStr(0, 44, (String("http://") + ip).c_str());
    }
    u8g2->drawStr(0, 53, (String("Token: ") + token).c_str());
    u8g2->drawStr(0, 62, (String("Clients: ") + webGetClientCount()).c_str());
}

void web_tools_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    if (wt_state == WT_CONFIG) wt_row = drawMenu(u8g2, &wt_menu, wt_row);
    else drawRunning(u8g2);
    u8g2->sendBuffer();
}

App app_web_tools = {
    .name = "Web Tools",
    .onStart = web_tools_onStart,
    .onEvent = web_tools_onEvent,
    .onDraw = web_tools_onDraw,
    .onStop = web_tools_onStop,
};
