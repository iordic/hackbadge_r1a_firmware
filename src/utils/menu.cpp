#include <Arduino.h>
#include <functional>
#include <SimpleList.h>
#include "app.h"
#include "menu.h"

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
    menu->list->add(MenuNode{ NULL, getStr, click, hold, NULL, NULL, NULL });
}

void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, 
                std::function<void()>click, std::function<void()>hold) {
    menu->list->add(MenuNode{ getIcon, getStr, click, hold, NULL, NULL, NULL });
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