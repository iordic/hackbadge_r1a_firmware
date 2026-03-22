#ifndef MENU_UTILS_H_
#define MENU_UTILS_H_


struct SettingsValue {
    uint8_t current;
    uint8_t max;
};

enum MenuType {
    MENU_LIST,
    DYNAMIC_LIST
};

/* Copied & adapted from esp8266_deauther licensed under MIT */
struct MenuNode {
    std::function<uint16_t()> getIcon; 
    std::function<String()>   getStr;
    std::function<void()>     click;
    std::function<void()>     hold;
    std::function<void()>     left;
    std::function<void()>     right;
};

struct Menu {
    SimpleList<MenuNode>  * list;
    Menu                  * parentMenu;
    std::function<String()> getTitle;
    int8_t                  selected;
    MenuType                type;
    std::function<void()>   build;
};

void changeMenu(Menu* menu);
void createDynamicMenu(Menu* menu, Menu* parent, std::function<String()>getTitle, std::function<void()>build);
void createMenu(Menu* menu, Menu* parent, std::function<void()>build);
void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click, std::function<void()>hold);
void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, std::function<void()>click, std::function<void()>hold);
void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, std::function<void()>click);
void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click);
void addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next);
void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, Menu* next);
void addMenuNode(Menu* menu, const char* ptr, std::function<void()>click);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, std::function<void()>click);
void addMenuNode(Menu* menu, const char* ptr, Menu* next);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* next);
void addMenuNode(Menu* menu, String label, App* back, Menu* next);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, App* back, Menu* next);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* back, App* next);
void addMenuNodeSetting(Menu* menu, const char* ptr, SettingsValue* value, std::function<String(uint8_t)>conversionFromEnum, Menu* back);
int drawMenu(U8G2 *u8g2, Menu* menu, int firstItem);
int drawStaticMenu(U8G2 *u8g2, Menu* menu, int firstItem);
int drawDynamicList(U8G2 *u8g2, Menu* menu, int firstItem);
void showPopupMenu(const char* message);
#endif