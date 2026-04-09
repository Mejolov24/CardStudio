#include <stdint.h>
#ifndef M5CADVKeyCB_h
#define M5CADVKeyCB_h
#ifndef MAX_KEYS
    #define MAX_KEYS 16
#endif


class M5CADVKeyCB{
    private:
    typedef void (*KeyCallback)(uint8_t key, bool pressed);
    uint8_t previousKeys[MAX_KEYS];
    uint8_t previousCount = 0;
    KeyCallback KeyboardCallback = nullptr;

    public:
    void SetupKeyboardCallback(KeyCallback callback);
    void KeyboardUpdate();

};
#endif