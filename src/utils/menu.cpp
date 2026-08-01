#include <Arduino.h>
#include <functional>
#include <SimpleList.h>
#include "app.h"
#include "menu.h"

#include "devices/display.h"
#include "devices/radio.h"
#include "tasks/ui_task.h"

Menu* currentMenu;
extern App *currentApp;
extern Menu* currentMenu;

// Adapted snippet taken from esp8266_deauther
void changeMenu(Menu* menu) {
    currentMenu = menu;
    menu->selected = 0;
}

void menuHandleEvent(Menu* menu, int evt) {
    switch (evt) {
    case BTN_BACK:  menu->list->get(menu->selected).hold();  break;
    case BTN_OK:    menu->list->get(menu->selected).click(); break;
    case BTN_UP:    menu->selected--; break;
    case BTN_DOWN:  menu->selected++; break;
    case BTN_LEFT:  menu->list->get(menu->selected).left();  break;
    case BTN_RIGHT: menu->list->get(menu->selected).right(); break;
    }
}

void createMenu(Menu* menu, Menu* parent, std::function<void()>build) {
    // Si la app se reabre, liberamos la lista del onStart anterior para no
    // filtrarla. Los Menu son globales (list == nullptr en la 1ª llamada).
    if (menu->list) delete menu->list;
    menu->list       = new SimpleList<MenuNode>;
    menu->parentMenu = parent;
    menu->selected   = 0;
    menu->build      = build;
    menu->type       = MENU_LIST;
}

void createDynamicMenu(Menu* menu, Menu* parent, std::function<String()>getTitle, std::function<void()>build) {
    createMenu(menu, parent, build);
    menu->getTitle   = getTitle;
    menu->type       = DYNAMIC_LIST;
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
        currentApp = back;
        currentApp->onStart();
    });
}

void addMenuNode(Menu* menu, String label, App* back, Menu* next) {
    addMenuNode(menu, [label]() { return label;}, 
    [next]() {
        changeMenu(next);
    },
    [back]() {
        currentMenu = NULL;
        changeAppContext(back);
    });
}

void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* back, App* next) {
    addMenuNode(menu, [icon]() -> uint16_t {return icon ? *icon : 0;}, [ptr]() {return String(ptr);}, [next]() {
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

// Calcula el primer ítem visible desplazando la "ventana" para que el
// seleccionado quede dentro, con topes arriba/abajo. Compartido por la lista
// estática y la dinámica (antes estaba duplicado en ambas).
static int computeScrollWindow(int selected, int total, int visibleCount, int firstItem) {
    if (selected >= firstItem + visibleCount)
        firstItem = selected - visibleCount + 1;
    else if (selected < firstItem)
        firstItem = selected;
    if (firstItem < 0) firstItem = 0;
    if (total > visibleCount) {
        if (firstItem > total - visibleCount)
            firstItem = total - visibleCount;
    } else {
        firstItem = 0;   // caben todos: sin scroll
    }
    return firstItem;
}

int drawMenu(U8G2 *u8g2, Menu* menu, int firstItem) {
    if (menu->type == MENU_LIST)
        return drawStaticMenu(u8g2, menu, firstItem);
    if (menu->type == DYNAMIC_LIST)
        return drawDynamicList(u8g2, menu, firstItem);
    return 0;
}

int drawStaticMenu(U8G2 *u8g2, Menu* menu, int firstItem) {
    u8g2->setDrawColor(1);
    uint8_t xStartWritting = 0;
    const int visibleCount = 4;  // número de líneas visibles en pantalla
    int total = menu->list->size();
    // --- Seguridad: evitar índices fuera de rango ---
    if (menu->selected < 0)
        menu->selected = total - 1; // wrap around to the bottom
    else if (menu->selected >= total)
        menu->selected = 0; // wrap around to the top

    firstItem = computeScrollWindow(menu->selected, total, visibleCount, firstItem);
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
    return firstItem;
}

int drawDynamicList(U8G2 *u8g2, Menu* menu, int firstItem) {
    u8g2->setDrawColor(1);
    int visibleCount = 3;  // número de líneas visibles en pantalla
    int total = menu->list->size();
    // --- Seguridad: evitar índices fuera de rango ---
    // (antes de pintar la cabecera, para que el contador nunca muestre 0/N ni N+1/N)
    if (total > 0) {
        if (menu->selected < 0)
            menu->selected = total - 1; // wrap around to the bottom
        else if (menu->selected >= total)
            menu->selected = 0; // wrap around to the top
    }

    // --- Cabecera: contador alineado a la derecha y título recortado ---
    // El contador crece al pasar de 9 a 10 elementos, así que lo anclamos al
    // borde derecho y limitamos el título al hueco libre para que no se solapen.
    u8g2->setFont(u8g2_font_t0_11_mr);
    String counter = String(menu->selected + 1) + "/" + String(total);
    int counterX = 128 - u8g2->getStrWidth(counter.c_str());
    String title = menu->getTitle();
    int maxTitleWidth = counterX - 3;   // 3 px de separación
    while (title.length() > 0 && u8g2->getStrWidth(title.c_str()) > maxTitleWidth)
        title.remove(title.length() - 1);
    u8g2->drawStr(0, 8, title.c_str());
    u8g2->drawStr(counterX, 8, counter.c_str());
    u8g2->setFont(u8g2_font_7x14_tr);

    firstItem = computeScrollWindow(menu->selected, total, visibleCount, firstItem);
    // --- Dibujar los ítems visibles ---
    u8g2->setFont(u8g2_font_t0_12_mr);
    for (int i = firstItem; i < total && i < firstItem + visibleCount; i++) {
        int drawColor = menu->selected == i ? 1 : 0;
        u8g2->setDrawColor(drawColor);
        u8g2->drawBox(0, (i - firstItem + 1) * 14, 128, 16);
        u8g2->setDrawColor(!drawColor);
        u8g2->drawStr(2, (i - firstItem + 2) * 14 - 2, menu->list->get(i).getStr().c_str());
    }
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

void drawEmptyFolder(U8G2 *u8g2) {
    u8g2->setFont(u8g2_font_fub30_t_symbol);
    u8g2->drawStr(25, 40, "404");
    u8g2->setFont(u8g2_font_7x14_mr);
    u8g2->drawStr(10, 54, "Folder is empty.");
}

void drawRssi(U8G2 *u8g2) {
    int receivedRssi = radio_get()->getRssi();
    int barWidth = 40;
    u8g2->setDrawColor(1);
    u8g2->drawBox(85, 59, map(receivedRssi, -100, -11, 0, 40), 5);
    u8g2->drawFrame(85, 59, barWidth, 5);
    u8g2->setFont(u8g2_font_tiny5_tr);
    u8g2->drawStr(70, 64, "rssi");
}
