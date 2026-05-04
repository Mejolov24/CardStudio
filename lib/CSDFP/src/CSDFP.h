# ifndef SDFP
# define SDFP
#include <stdint.h>

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
    SelectionCallback Callback = nullptr;
    bool _active = false;

    public:
    void setSelectionCallback(SelectionCallback callback);
    void open(const char* path); // opens the file picker
    void close(); // closes the file picker
    void process_input(Input input); 
};



#endif