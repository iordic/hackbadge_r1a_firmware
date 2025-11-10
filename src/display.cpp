#include "display.h"
#include "config/io_config.h"
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void display_init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  //u8g2.setI2CAddress(SCREEN_ADDRESS); // If needed: https://github.com/olikraus/u8g2/wiki/u8g2reference#seti2caddress
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr);
}

U8G2 *display_get() {
    return &u8g2;
}
