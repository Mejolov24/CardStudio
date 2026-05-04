# ifndef SDFP
# define SDFP
#include <stdint.h>
#include <Arduino.h>
#include <SD.h>
#include <M5Cardputer.h>
extern M5Canvas canvas;

enum class Input : uint8_t{
    up = 0,
    down = 1,
    left = 2,
    right = 3,
    select = 4,
};

class CSDFP{
    private:
    typedef void (*SelectionCallback)(const char path);
    M5Canvas* _canvas;
    SelectionCallback Callback = nullptr;
    bool _active = false;
    void render();

    public:
    void begin(M5Canvas* targetCanvas);
    void setSelectionCallback(SelectionCallback callback);
    void open(const char* path); // opens the file picker
    void close(); // closes the file picker
    void process_input(Input input);
};



#endif