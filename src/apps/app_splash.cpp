
#include "app.h"
#include "devices/display.h"
#include "app_splash.h"
#include "config/sprites.h"

extern App app_menu;
extern L33T_Animation BAT_SPLASH;

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
    u8g2->setDrawColor(1);
    u8g2->clearBuffer();
    BAT_SPLASH.chkAnimation(true);
    u8g2->drawXBM(BAT_SPLASH.getXpos(), BAT_SPLASH.getYpos(), BAT_SPLASH.getWidth(), BAT_SPLASH.getHeight(), bat_splash_bits[BAT_SPLASH.getCurrentFrame()]); 
    u8g2->drawFrame(34, 30, 94, 34);
    u8g2->setFont(u8g2_font_5x8_mf);
    u8g2->drawStr(38, 32, "hackbat:-$ whoami");
    u8g2->setFont(u8g2_font_littlemissloudonbold_tr);
    u8g2->drawStr(38, 46, "@iordic");
    u8g2->setFont(u8g2_font_6x10_mf);
    u8g2->drawUTF8(38, 58, "Jordi Castelló");
    if (BAT_SPLASH.toReset()) BAT_SPLASH.resetAni();
    u8g2->sendBuffer();
}

App app_splash = {
  .name = "Splash",
  .onStart = splash_onStart,
  .onEvent = splash_onEvent,
  .onDraw = splash_onDraw,
  .onStop = splash_onStop
};
