
#include "app.h"
#include "devices/display.h"
#include "app_about.h"
#include "config/sprites.h"
#include "literals.h"

extern App app_menu;
extern L33T_Animation PHANTOM;

void about_onStart() {}

void about_onStop() {}

void about_onEvent(int evt) {
    if (evt == BTN_BACK) {
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
    }
}

void about_onDraw(U8G2 *u8g2) {
    u8g2->setDrawColor(1);
    u8g2->clearBuffer();
    PHANTOM.chkAnimation(true);
    u8g2->drawXBM(PHANTOM.getXpos(), PHANTOM.getYpos(), PHANTOM.getWidth(), PHANTOM.getHeight(), phantom_animation_bits[PHANTOM.getCurrentFrame()]); 
    u8g2->setFont(u8g2_font_6x10_mf);
    u8g2->drawUTF8(0, 10, "Developed with");
    u8g2->setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2->drawGlyph(36, 30, HEART_ICON);
    u8g2->setFont(u8g2_font_littlemissloudonbold_tr);
    u8g2->drawStr(5, 42, "by @iordic");
    u8g2->setFont(u8g2_font_5x8_mf);
    u8g2->drawStr(0, 60, ("FW version: " + String(FIRMWARE_VERSION)).c_str());
    if (PHANTOM.toReset()) PHANTOM.resetAni();
    u8g2->sendBuffer();
}

App app_about = {
  .name = "About",
  .onStart = about_onStart,
  .onEvent = about_onEvent,
  .onDraw = about_onDraw,
  .onStop = about_onStop
};
