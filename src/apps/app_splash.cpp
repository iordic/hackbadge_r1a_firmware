
#include "app.h"
#include "display.h"
#include "app_splash.h"

extern App app_menu;

void splash_onStart() {}

void splash_onStop() {}

void splash_onEvent(int evt) {
    if (evt == BTN_BACK) {
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
    }
}

void splash_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    u8g2->drawFrame(38, 16, 88, 32);
    u8g2->setFont(u8g2_font_5x8_mf);
    u8g2->drawStr(44, 18, "badge:-$ whoami");
    u8g2->setFont(u8g2_font_littlemissloudonbold_tr);
    u8g2->drawStr(42, 30, "@iordic");
    u8g2->setFont(u8g2_font_6x10_mf);
    u8g2->drawUTF8(40, 42, "Jordi Castelló");

    PHANTOM.chkAnimation(true);
    u8g2->drawXBM(PHANTOM.getXpos(), PHANTOM.getYpos(), PHANTOM.getWidth(), PHANTOM.getHeight(), phantom_animation_bits[PHANTOM.getCurrentFrame()]); 
    if (PHANTOM.toReset())
        PHANTOM.resetAni();
    u8g2->sendBuffer();
}

App app_splash = {
  .name = "Splash",
  .onStart = splash_onStart,
  .onEvent = splash_onEvent,
  .onDraw = splash_onDraw,
  .onStop = splash_onStop
};
