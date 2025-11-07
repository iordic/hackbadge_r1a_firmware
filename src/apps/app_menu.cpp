#include "literals.h"
#include "app.h"
#include "display.h"
#include "app_menu.h"

extern App app_splash;

Menu* currentMenu;

Menu mainMenu;
Menu subghzMenu;

int row;

void menu_onStart() {
    row = 0;
    createMenu(&mainMenu, NULL, []() {
        addMenuNode(&mainMenu, &WAVE_ICON, MENU_ITEM_SUBGHZ, &subghzMenu);
        addMenuNode(&mainMenu, &BLUETOOTH_ICON, MENU_ITEM_BLE, &subghzMenu);
        addMenuNode(&mainMenu, &WIFI_ICON, MENU_ITEM_WIFI, &subghzMenu);
        addMenuNode(&mainMenu, &PUZZLE_ICON, MENU_ITEM_GAMES, &subghzMenu);
        addMenuNode(&mainMenu, &WRENCH_ICON, MENU_ITEM_SETTINGS, &subghzMenu);
    });
}

void menu_onStop() {}

void menu_onEvent(int evt) {
  if (evt == BTN_BACK) {
    extern App *currentApp;
    currentApp = &app_splash;
    currentApp->onStart();
  } else if (evt == BTN_UP) {
    currentMenu->selected--;
  } else if (evt == BTN_DOWN) {
    currentMenu->selected++;
  }
}

void menu_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    /*
    u8g2->drawBox(0, 32, 128, 16);
    u8g2->setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2->drawGlyph(2, 16, 0x0055);
    u8g2->drawGlyph(2, 32, 0x005e);
    u8g2->setDrawColor(0);
    u8g2->drawGlyph(2, 48, 0x00ef);
    u8g2->setDrawColor(1);
    u8g2->drawGlyph(2, 64, 0x011a);
    u8g2->setFont(u8g2_font_7x14_tr);
    u8g2->drawStr(21, 14, "SubGhz");
    u8g2->drawStr(21, 30,"BLE");
    u8g2->setDrawColor(0);
    u8g2->drawStr(21, 46,"Games");
    u8g2->setDrawColor(1);
    u8g2->drawStr(21, 62, "Settings");
    u8g2->sendBuffer();
    */
   const int visibleCount = 4;  // número de líneas visibles en pantalla
   uint16_t tempIcon; 
   String tmpString;
    //int tmpLen;

    int total = currentMenu->list->size();

    // --- Seguridad: evitar índices fuera de rango ---
    if (currentMenu->selected < 0)
        currentMenu->selected = 0;
    else if (currentMenu->selected >= total)
        currentMenu->selected = total - 1;

    // --- Ajuste automático del scroll vertical (row) ---
    // Mueve la "ventana" visible cuando el elemento seleccionado sale del rango actual
    if (currentMenu->selected >= row + visibleCount)
        row = currentMenu->selected - visibleCount + 1;
    else if (currentMenu->selected < row)
        row = currentMenu->selected;

    // --- Límite inferior (scroll hacia arriba) ---
    if (row < 0) row = 0;

    // --- Límite superior (scroll hacia abajo) ---
    // Si hay más elementos que líneas visibles, el máximo desplazamiento es total - visibleCount
    if (total > visibleCount) {
        if (row > total - visibleCount)
            row = total - visibleCount;
    } else {
        // Si caben todos, mantener siempre en 0
        row = 0;
    }

    // --- Dibujar los ítems visibles ---
    for (int i = row; i < total && i < row + visibleCount; i++) {
        tempIcon = currentMenu->list->get(i).getIcon();
        tmpString = currentMenu->list->get(i).getStr();
        //tmpLen = tmp.length();

        /* TODO
        // --- Scroll horizontal si el texto es más largo que el máximo ---
        if ((currentMenu->selected == i) && (tmpLen >= maxLen)) {
            tmp = tmp + tmp;  // duplicar para efecto de desplazamiento circular
            tmp = tmp.substring(scrollCounter, scrollCounter + maxLen - 1);

            if (((scrollCounter > 0) && (scrollTime < currentTime - scrollSpeed)) ||
                ((scrollCounter == 0) && (scrollTime < currentTime - scrollSpeed * 4))) {
                scrollTime = currentTime;
                scrollCounter++;
            }

            if (scrollCounter > tmpLen)
                scrollCounter = 0;
        }
        */

        // --- Agregar cursor al ítem seleccionado ---
        //tmp = (currentMenu->selected == i ? CURSOR : SPACE) + tmp;

        // --- Dibujar el texto ---
        // (i - row) es el índice relativo dentro de las 5 filas visibles
        //drawString(0, (i - row) * 12, tmp);
    }
}

// Adapted snippet taken from esp8266_deauther
void changeMenu(Menu* menu) {
    currentMenu = menu;
   
}

void createMenu(Menu* menu, Menu* parent, std::function<void()>build) {
    menu->list       = new SimpleList<MenuNode>;
    menu->parentMenu = parent;
    menu->selected   = 0;
    menu->build      = build;
}

void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click,
                            std::function<void()>hold) {
    menu->list->add(MenuNode{ NULL, getStr, click, hold });
}

void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getStr, click, NULL);
}

void addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getStr, [next]() {
        changeMenu(next);
    });
}

void addMenuNode(Menu* menu, const char* ptr, std::function<void()>click) {
    addMenuNode(menu, [ptr]() {
        return String(ptr);
    }, click);
}

void addMenuNode(Menu* menu, const char* ptr, Menu* next) {
    addMenuNode(menu, [ptr]() {
        return String(ptr);
    }, next);
}

void addMenuNode(Menu* menu, const uint16_t* icon, const char* ptr, Menu* next) {
    addMenuNode(menu,  [icon]() {return icon;}, [ptr]() {return String(ptr);}, next);
}

App app_menu = {
  .name = "Menu",
  .onStart = menu_onStart,
  .onEvent = menu_onEvent,
  .onDraw = menu_onDraw,
  .onStop = menu_onStop
};