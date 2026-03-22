#include "input.h"

static int lastButton = BTN_NONE;
unsigned long lastMillisButtonPress = 0;
bool longPressTriggered = false; 

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
  int currentPhysButton = BTN_NONE;
  unsigned long currentMillis = millis();

  // 1. Lectura física
  if (digitalRead(BUTTON_LEFT) == LOW)        currentPhysButton = BTN_LEFT;
  else if (digitalRead(BUTTON_UP) == LOW)     currentPhysButton = BTN_UP;
  else if (digitalRead(BUTTON_RIGHT) == LOW)  currentPhysButton = BTN_RIGHT;
  else if (digitalRead(BUTTON_DOWN) == LOW)   currentPhysButton = BTN_DOWN;
  else if (digitalRead(BUTTON_ENTER) == LOW)  currentPhysButton = BTN_OK;

  // --- CASO A: NO HAY NINGÚN BOTÓN PULSADO ---
  if (currentPhysButton == BTN_NONE) {
    int releasedButton = lastButton; // Guardamos qué botón se acaba de soltar
    lastButton = BTN_NONE;
    // Si soltamos el OK y NO se había activado el BACK, devolvemos OK ahora
    if (releasedButton == BTN_OK && !longPressTriggered) {
      longPressTriggered = false;
      return BTN_OK; 
    }
    longPressTriggered = false;
    return BTN_NONE;
  }
  // --- CASO B: ES UNA PULSACIÓN NUEVA (distinta a la anterior) ---
  if (currentPhysButton != lastButton) {
    lastButton = currentPhysButton;
    lastMillisButtonPress = currentMillis;
    longPressTriggered = false;
    // Si es D-PAD (flechas), disparamos la primera pulsación de inmediato
    if (currentPhysButton != BTN_OK) {
      return currentPhysButton;
    }
    return BTN_NONE; // Si es OK, esperamos a ver si lo mantiene o lo suelta
  }
  // --- CASO C: MANTENIENDO EL MISMO BOTÓN ---
  // Si es el botón OK y pasa el tiempo largo -> Devolvemos BACK
  if (currentPhysButton == BTN_OK && !longPressTriggered) {
    if (currentMillis - lastMillisButtonPress >= LONG_PRESS_INTERVAL) {
      longPressTriggered = true; // Marcamos que ya hicimos el BACK
      return BTN_BACK;
    }
  }
  // Si son las FLECHAS y pasa el tiempo de auto-repetición
  if (currentPhysButton != BTN_OK && (currentMillis - lastMillisButtonPress >= REPEAT_INTERVAL)) {
    lastMillisButtonPress = currentMillis; // Reset para la siguiente repetición
    return currentPhysButton;
  }
  return BTN_NONE;
}

void input_init() {
  pinMode(BUTTON_LEFT, INPUT_TYPE_BUTTON);
  pinMode(BUTTON_UP, INPUT_TYPE_BUTTON);
  pinMode(BUTTON_RIGHT, INPUT_TYPE_BUTTON);
  pinMode(BUTTON_DOWN, INPUT_TYPE_BUTTON);
  pinMode(BUTTON_ENTER, INPUT_TYPE_BUTTON);
}
