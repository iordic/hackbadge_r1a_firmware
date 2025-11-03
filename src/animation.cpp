#include "animation.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
// width, height, frame_delay (ms), xInc, yInc, xStart, xEnd, yStart, yEnd, nFrames
L33T_Animation PHANTOM(30, 50, 250, 0, 0, 0, 0, 10, 10, frames);
void beginU8g2() {
    u8g2.begin();
    //u8g2.setBitmapMode(1); // mode transparent
}
void drawSplashTexts() {
    u8g2.firstPage();
    do {
        u8g2.drawFrame(38, 16, 88, 32);
        u8g2.setFont(u8g2_font_5x8_mf);
        u8g2.drawStr(44, 18, "badge:-$ whoami");
        u8g2.setFont(u8g2_font_littlemissloudonbold_tr);
        u8g2.drawStr(42, 30, "@iordic");
        u8g2.setFont(u8g2_font_6x10_mf);
        u8g2.drawUTF8(40, 42, "Jordi Castelló");
    } while (u8g2.nextPage());
}

void updateAnimation() {
    PHANTOM.chkAnimation(true);
    u8g2.sendBuffer();
    u8g2.drawXBM(PHANTOM.getXpos(), PHANTOM.getYpos(), PHANTOM.getWidth(), PHANTOM.getHeight(), phantom_animation_bits[PHANTOM.getCurrentFrame()]); 
    if (PHANTOM.toReset())
        PHANTOM.resetAni();
}
