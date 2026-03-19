#include <Arduino.h>
#include <functional>
#include <SimpleList.h>
#include "app.h"
#include "menu.h"

#include "devices/display.h"

Menu* currentMenu;

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
    menu->list->add(MenuNode{ []() -> uint16_t {return 0;}, getStr, click, hold, [](){}, [](){} });
}

void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, 
                std::function<void()>click, std::function<void()>hold) {
    menu->list->add(MenuNode{ getIcon, getStr, click, hold, [](){}, [](){} });
}


void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getIcon, getStr, click, [menu]() {changeMenu(menu->parentMenu);});
}

void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getStr, click, [menu]() {changeMenu(menu->parentMenu);});
}

void addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getStr, [next]() {
        changeMenu(next);
    });
}

void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getIcon, getStr, [next]() {changeMenu(next);});
}

void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, std::function<void()>click) {
    addMenuNode(menu, [icon]() -> uint16_t {return icon ? *icon : 0;}, [ptr]() {return String(ptr);}, click);
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
    addMenuNode(menu, [icon]() -> uint16_t {return icon ? *icon : 0;}, [ptr]() {return String(ptr);}, next);
}

void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, App* back, Menu* next) {
    addMenuNode(menu, [icon]() -> uint16_t {return icon ? *icon : 0;}, [ptr]() {return String(ptr);}, 
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
    addMenuNode(menu, [icon]() -> uint16_t {return icon ? *icon : 0;}, [ptr]() {return String(ptr);}, [next]() {
        extern App *currentApp;
        currentApp = next;
        currentApp->onStart();
    }, [back]() {
        changeMenu(back);
    });
}

void addMenuNodeSetting(Menu* menu, const char* ptr, SettingsValue* value, std::function<String(uint8_t)>conversionFromEnum, Menu* back) {
    menu->list->add(MenuNode{ []() -> uint16_t {return 0;}, 
        [ptr, value, conversionFromEnum]() {
            return String(ptr) + (value->current == 0 ? "  " : "< ") + conversionFromEnum(value->current) + (value->current == value->max ? "  " : " >");
        },
        [](){},
        [back]() {changeMenu(back);}, 
        [value](){ value->current =  value->current == 0 ? 0 : value->current-1;}, 
        [value](){ value->current = value->current == value->max ? value->max : value->current+1;}}
    );
}

int drawMenu(U8G2 *u8g2, Menu* menu, int firstItem) {
    uint8_t xStartWritting = 0;
    u8g2->clearBuffer();
    const int visibleCount = 4;  // número de líneas visibles en pantalla
    String tmp;
    int tmpLen;

    int total = menu->list->size();

    // --- Seguridad: evitar índices fuera de rango ---
    if (menu->selected < 0)
        menu->selected = total - 1; // wrap around to the bottom
    else if (menu->selected >= total)
        menu->selected = 0; // wrap around to the top

    // --- Ajuste automático del scroll vertical (firstItem) ---
    // Mueve la "ventana" visible cuando el elemento seleccionado sale del rango actual
    if (menu->selected >= firstItem + visibleCount)
        firstItem = menu->selected - visibleCount + 1;
    else if (menu->selected < firstItem)
        firstItem = menu->selected;

    // --- Límite inferior (scroll hacia arriba) ---
    if (firstItem < 0) firstItem = 0;

    // --- Límite superior (scroll hacia abajo) ---
    // Si hay más elementos que líneas visibles, el máximo desplazamiento es total - visibleCount
    if (total > visibleCount) {
        if (firstItem > total - visibleCount)
            firstItem = total - visibleCount;
    } else {
        // Si caben todos, mantener siempre en 0
        firstItem = 0;
    }
    // --- Dibujar los ítems visibles ---
    for (int i = firstItem; i < total && i < firstItem + visibleCount; i++) {
        xStartWritting = 2;
        int drawColor = menu->selected == i ? 1 : 0;
        u8g2->setDrawColor(drawColor);
        u8g2->drawBox(0, (i - firstItem) * 16, 128, 16);
        u8g2->setDrawColor(!drawColor);
        if (menu->list->get(i).getIcon()) {
            u8g2->setFont(u8g2_font_open_iconic_all_2x_t);
            u8g2->drawGlyph(xStartWritting, (i - firstItem + 1) * 16, menu->list->get(i).getIcon());
            xStartWritting = 21;
        }
        u8g2->setFont(u8g2_font_7x14_mr);
        u8g2->drawStr(xStartWritting, (i - firstItem + 1) * 16 - 2, menu->list->get(i).getStr().c_str());
    }
    u8g2->sendBuffer();
    return firstItem;
}

void showPopupMenu(const char* message) {
    U8G2 *u8g2 = display_get();
    u8g2->setFont(u8g2_font_7x14_mr);
    int16_t strWidth = u8g2->getStrWidth(message);
    u8g2->setDrawColor(0);
    u8g2->drawRBox(((128 - strWidth) / 2) - 5, 10, strWidth + 10, 40, 2);
    u8g2->setDrawColor(1);
    u8g2->drawRFrame(((128 - strWidth) / 2) - 5, 10, strWidth + 10, 40, 2);
    u8g2->drawStr((128 - strWidth) / 2, 36, message);
    u8g2->sendBuffer();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
