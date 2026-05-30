#include <stdint.h>
#include <M5Config.h>

uint8_t item1 = 0;
bool item2 = 0;
void item3(){}
M5Config::ConfigMenu menu1;

M5Config::ConfigItem configs[] = {
    {
        "0-10", // name
        &item1, // pointer to variable
        1, // increment
        0, // minimum
        10,// maximum
        M5Config::ScrollType::TYPE_CLAMP
    },
    {
        "bool",
        &item2
    },
    {
        "function",
        item3 // function pointer
    },
    {
        "sub menu",
        &menu1 // pointer to ConfigMenu
    }
};

M5Config::ConfigMenu settings_menu = {
    .config_items = configs, 
    .size = sizeof(configs) / sizeof(configs[0])
};