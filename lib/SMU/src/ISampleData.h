#ifndef I_AUDIO_DATA_H
#define I_AUDIO_DATA_H
#include <stdint.h>

struct SampleData {
    const char* name;
    const int16_t* data;
    uint32_t length;
    uint32_t loop_start;
    uint32_t loop_end;
};
#endif