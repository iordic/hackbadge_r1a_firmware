#ifndef MENU_UTILS_H_
#define MENU_UTILS_H_


/* Copied & adapted from esp8266_deauther licensed under MIT */
struct MenuNode {
    std::function<uint16_t()>getIcon; 
    std::function<String()>getStr; // function used to create the displayed string
    std::function<void()>    click;  // function that is executed when node is clicked
    std::function<void()>    hold;   // function that is executed when node is pressed for > 800ms
};

struct Menu {
    SimpleList<MenuNode>* list;
    Menu                * parentMenu;
    int8_t               selected;
    std::function<void()> build; // function that is executed when button is clicked
};

void changeMenu(Menu* menu);
void createMenu(Menu* menu, Menu* parent, std::function<void()>build);
void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click, std::function<void()>hold);
void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, std::function<void()>click, std::function<void()>hold);
void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, std::function<void()>click);
void addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click);
void addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next);
void addMenuNode(Menu* menu, std::function<uint16_t()>getIcon, std::function<String()>getStr, Menu* next);
void addMenuNode(Menu* menu, const char* ptr, std::function<void()>click);
void addMenuNode(Menu* menu, const char* ptr, Menu* next);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* next);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, App* back, Menu* next);
void addMenuNode(Menu* menu, const uint16_t *icon, const char* ptr, Menu* back, App* next);
#endif