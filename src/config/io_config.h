#ifndef CONFIG_H_
#define CONFIG_H_
#define RECEIVE_ATTR IRAM_ATTR

#define SERIAL_BAUDRATE 115200

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3c // default value should be 0x3c
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define NUM_LEDS 2

/* I/O */
#ifdef ESP32_C3_SUPERMINI_CUSTOM
    /* Strapping GPIOs:
     *   2 -> Must be HIGH during reset, if this pin goes low during reset, either flash execution or UART flashing may not work.
     *   8 -> Must be HIGH during reset, UART flashing may not work if this pin goes low during reset.
     *   0, 9 -> Works as normal GPIOs after reset (Where it must be high for regular execution and low to flash an application).
     */
    //#define BUILTIN_LED 8
    #define CC1101_SCLK  4
    #define CC1101_MISO  5
    #define CC1101_MOSI  6
    #define CC1101_CS    7 
    #define CC1101_GDO0 20
    #define CC1101_GDO2 -1 //21 // we are out of pins, so we must use gdo0 for both, receive and transmit
    #define I2C_SDA 10
    #define I2C_SCL  9
    #define NEOPIXEL 0
    // buttons
    #define BUTTON_LEFT  21
    #define BUTTON_UP     2
    #define BUTTON_RIGHT  3
    #define BUTTON_DOWN   1
    #define BUTTON_ENTER  8
#endif
#ifdef HACKBADGE_R1A
    #define CC1101_SCLK 10
    #define CC1101_MISO  3
    #define CC1101_MOSI  21 //4
    #define CC1101_CS   20 
    #define CC1101_GDO0 4 //21 // -> pin 21 cannot be used as INPUT, so we remap GDO0 to pin 4
    #define CC1101_GDO2 -1 //21 // we are out of pins, so we must use gdo0 for both, receive and transmit
    #define I2C_SDA      1
    #define I2C_SCL      0
    #define NEOPIXEL     2
    // buttons
    #define BUTTON_LEFT  9
    #define BUTTON_UP    8
    #define BUTTON_RIGHT 7
    #define BUTTON_DOWN  6
    #define BUTTON_ENTER 5
#endif
#endif