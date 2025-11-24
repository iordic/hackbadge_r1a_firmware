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
