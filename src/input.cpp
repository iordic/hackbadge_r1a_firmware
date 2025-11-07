#include "input.h"

static int lastButton = BTN_NONE;
OneButton leftBtn, upBtn, rightBtn, downBtn, enterBtn;

static void handleLeft() {
  lastButton = BTN_LEFT;
}
static void handleUp() {
    lastButton = BTN_UP;
}
static void handleRight() {
    lastButton = BTN_RIGHT;
}
static void handleDown() {
  lastButton = BTN_DOWN;
}
static void handleOk() {
  lastButton = BTN_OK;
}
static void handleBack() {
  lastButton = BTN_BACK;
}

int input_read() {
    ticks();
    int pressedButton = lastButton;
    lastButton = BTN_NONE;
    return pressedButton;
}

void input_init() {
    // Buttons setup
    leftBtn.setup(BUTTON_LEFT, INPUT_PULLUP, true);
    upBtn.setup(BUTTON_UP, INPUT_PULLUP, true);
    rightBtn.setup(BUTTON_RIGHT, INPUT_PULLUP, true);
    downBtn.setup(BUTTON_DOWN, INPUT_PULLUP, true);
    enterBtn.setup(BUTTON_ENTER, INPUT_PULLUP, true);
    // attach callbacks
    leftBtn.attachClick(handleLeft);
    rightBtn.attachClick(handleRight);
    upBtn.attachClick(handleUp);
    downBtn.attachClick(handleDown);
    enterBtn.attachClick(handleOk);
    enterBtn.attachLongPressStart(handleBack);
}

void ticks() {
    leftBtn.tick();
    upBtn.tick();
    rightBtn.tick();
    downBtn.tick();
    enterBtn.tick();
}