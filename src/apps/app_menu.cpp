#include "literals.h"
#include "app.h"
#include "display.h"
#include "app_menu.h"

extern App app_splash;
extern App app_snake;

Menu* currentMenu;

Menu mainMenu;
Menu subghzMenu;
Menu bleMenu;
Menu wifiMenu;
Menu gamesMenu;
Menu settingsMenu;

int row;

void menu_onStart() {
    row = 0;
    createMenu(&mainMenu, NULL, []() {
        addMenuNode(&mainMenu, &WAVE_ICON, MENU_ITEM_SUBGHZ, &app_splash, &subghzMenu);
        addMenuNode(&mainMenu, &BLUETOOTH_ICON, MENU_ITEM_BLE, &app_splash, &bleMenu);
        addMenuNode(&mainMenu, &WIFI_ICON, MENU_ITEM_WIFI, &app_splash, &wifiMenu);
        addMenuNode(&mainMenu, &PUZZLE_ICON, MENU_ITEM_GAMES, &app_splash, &gamesMenu);
        addMenuNode(&mainMenu, &WRENCH_ICON, MENU_ITEM_SETTINGS, &app_splash, &settingsMenu);
    });

    createMenu(&subghzMenu, &mainMenu, []() {
        addMenuNode(&subghzMenu, &GEAR_ICON, MENU_ITEM_CONFIG, &mainMenu);
        addMenuNode(&subghzMenu, &UP_ICON, MENU_ITEM_TRANSMIT, &mainMenu);
        addMenuNode(&subghzMenu, &DOWN_ICON, MENU_ITEM_RECEIVE, &mainMenu);
    });
    createMenu(&gamesMenu, &mainMenu, []() {
        addMenuNode(&gamesMenu, &CURSOR_DOWN_ICON, MENU_ITEM_SNAKE, &mainMenu, &app_snake);
    });
    currentMenu = &mainMenu;
    currentMenu->selected = 0;
    mainMenu.build();
    subghzMenu.build();
    gamesMenu.build();
}

void menu_onStop() {}

void menu_onEvent(int evt) {
    if (evt == BTN_BACK) {
        currentMenu->list->get(currentMenu->selected).hold();
    } else if (evt == BTN_OK) {
        currentMenu->list->get(currentMenu->selected).click();
    } else if (evt == BTN_UP) {
        currentMenu->selected--;
    } else if (evt == BTN_DOWN) {
        currentMenu->selected++;
    }
}

void menu_onDraw(U8G2 *u8g2) {
    u8g2->clearBuffer();
    const int visibleCount = 4;  // número de líneas visibles en pantalla
    String tmp;
    int tmpLen;

    int total = currentMenu->list->size();

    // --- Seguridad: evitar índices fuera de rango ---
    if (currentMenu->selected < 0)
        currentMenu->selected = total - 1; // wrap around to the bottom
    else if (currentMenu->selected >= total)
        currentMenu->selected = 0; // wrap around to the top

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
        int drawColor = currentMenu->selected == i ? 1 : 0;
        u8g2->setDrawColor(drawColor);
        u8g2->drawBox(0, (i - row) * 16, 128, 16);
        u8g2->setDrawColor(!drawColor);
        u8g2->setFont(u8g2_font_open_iconic_all_2x_t);
        u8g2->drawGlyph(2, (i - row + 1) * 16, currentMenu->list->get(i).getIcon());
        u8g2->setFont(u8g2_font_7x14_tr);
        u8g2->drawStr(21, (i - row + 1) * 16 - 2, currentMenu->list->get(i).getStr().c_str());
    }
    u8g2->sendBuffer();
}

// Adapted snippet taken from esp8266_deauther
void changeMenu(Menu* menu) {
    currentMenu = menu;
    menu->selected = 0;  
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

void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, 
                std::function<void()>click, std::function<void()>hold) {
    menu->list->add(MenuNode{ getIcon, getStr, click, hold });
}


void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getIcon, getStr, click, [menu]() {changeMenu(menu->parentMenu);});
}

void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getStr, click, NULL);
}

void addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getStr, [next]() {
        changeMenu(next);
    });
}

void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getIcon, getStr, [next]() {changeMenu(next);});
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

void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* next) {
    addMenuNode(menu, [icon]() -> uint16_t {return *icon;}, [ptr]() {return String(ptr);}, next);
}

void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, App* back, Menu* next) {
    addMenuNode(menu, [icon]() -> uint16_t {return *icon;}, [ptr]() {return String(ptr);}, 
    [next]() {
        changeMenu(next);
    },
    [back]() {
        extern App *currentApp;
        currentApp = back;
        currentApp->onStart();
    });
}

void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* back, App* next) {
    addMenuNode(menu, [icon]() -> uint16_t {return *icon;}, [ptr]() {return String(ptr);}, [next]() {
        extern App *currentApp;
        currentApp = next;
        currentApp->onStart();
    }, [back]() {
        changeMenu(back);
    });
}

App app_menu = {
  .name = "Menu",
  .onStart = menu_onStart,
  .onEvent = menu_onEvent,
  .onDraw = menu_onDraw,
  .onStop = menu_onStop
};