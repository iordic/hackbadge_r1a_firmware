#include "config/io_config.h"
#include "literals.h"
#include "app.h"
#include "app_i2c_tools.h"

extern App app_menu;

int i2cCursor = 0;

void i2c_tools_onStart() {

}

void i2c_tools_onStop() {

}   

void i2c_tools_onEvent(int evt) {
    if (evt == BTN_LEFT) {
        if ((i2cCursor & 0x0f) == 0) i2cCursor = i2cCursor | 0x0f;
        else i2cCursor = i2cCursor - 1;
    } else if (evt == BTN_RIGHT) {
        if ((i2cCursor & 0x0f) == 0x0f) i2cCursor = i2cCursor & 0xf0;
        else i2cCursor = i2cCursor + 1;
    } else if (evt == BTN_UP) {
        if ((i2cCursor & 0xf0) == 0) i2cCursor = i2cCursor | 0x70;
        else i2cCursor = i2cCursor - 0x10;
    } else if (evt == BTN_DOWN) {
        if ((i2cCursor & 0x70) == 0x70) i2cCursor = i2cCursor & 0x0f;
        else i2cCursor = i2cCursor + 0x10;
    }

    if (evt == BTN_BACK) {
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
    }
}

void i2c_tools_onDraw(U8G2 *u8g2) {
boolean detected = false;
u8g2->clearBuffer();
u8g2->setDrawColor(1);
u8g2->setFontDirection(3);
u8g2->setFont(u8g2_font_Wizzard_tr);
u8g2->drawStr(10, 63, ("cursor: " + String(i2cCursor, HEX)).c_str());
u8g2->setFont(u8g2_font_5x7_tr);
u8g2->setFontDirection(0);
// draw cols header
for (int i = 0; i < 16; i++) u8g2->drawStr(30 + i*6, 10, String(i, HEX).c_str());
// draw rows header
for (int i = 0; i < 8; i++) u8g2->drawStr(20, 20 + i*6, String(i, HEX).c_str());

u8g2->drawLine(15, 1, 26, 12);
u8g2->drawLine(16, 13, 126, 13);
u8g2->drawLine(27, 1, 27, 62);
u8g2->drawFrame(15, 1, 113, 63);
u8g2->drawStr(17, 11, "x");
u8g2->drawStr(23, 7, "y");
char c;
for (int i = 0; i < 8; i++) {
  for (int j = 0; j < 16; j++) {
    int address = (i << 4) | j;
    if (address == i2cCursor) {
        u8g2->drawBox(30 + j*6, 20 + (i-1)*6, 5, 7);
        u8g2->setDrawColor(0);
    } else {
        u8g2->setDrawColor(1);
    }
    detected = check_i2c_address(address);
    if (detected && address == SCREEN_ADDRESS) c = 'S';
    else c = detected ? 'x' : '-';
    u8g2->drawStr(30 + j*6, 20 + i*6, String(c).c_str());
  }
}
u8g2->sendBuffer();
}

boolean check_i2c_address(int address) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

App app_i2c_tools = {
  .name = "I2C Tools",
  .onStart = i2c_tools_onStart,
  .onEvent = i2c_tools_onEvent,
  .onDraw = i2c_tools_onDraw,
  .onStop = i2c_tools_onStop
};