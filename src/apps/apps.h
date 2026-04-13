#ifndef APPS_H_
#define APPS_H_
#include "app.h"

// Registro de las pantallas: cada app define su `App app_<name>` global en su
// propio .cpp; aquí se declaran para que el menú y las transiciones entre apps
// (changeAppContext) puedan referenciarlas. Sustituye a los headers por-app.
extern App app_splash;
extern App app_menu;
extern App app_about;
extern App app_snake;
extern App app_jammer;
extern App app_rcswitch;
extern App app_raw;
extern App app_wifi_beacon_spam;
extern App app_i2c_tools;
#endif
