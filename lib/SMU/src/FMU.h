#include "esp_partition.h"
#include "ISampleData.h"
#include <stdint.h>
#include <SD.h>

#ifndef FMU_H
#define FMU_H

class FMU {
public:
    enum class Result : uint8_t {
        Success = 0,
        FileNotFound,
        PartitionNotFound,
        InvalidMagic,
        FlashEraseError,
        FlashWriteError,
        SizeMismatch,
        MmapFailed,
        InvalidBankCount
};

    static Result burnSamplePack(const char* path);
    static Result mount();
    static Result mapSamplePack();
    static const SampleData* getInstruments() {return instruments; }
    static const SampleData* getPercussion() {return percussion; }
    static uint32_t getSampleRate() { return g_sample_rate; }
    static uint32_t getBitDepth()   { return g_bit_depth; }

private:
    struct __attribute__((packed)) SampleDataRaw {
        uint32_t name;
        uint32_t data;
        uint32_t length;
        uint32_t loop_start;
        uint32_t loop_end;
    };

    static SampleData instruments[128];
    static SampleData percussion[128];
    static uintptr_t flash_base;
    static spi_flash_mmap_handle_t mmap_handle;
    static uint32_t g_sample_rate;
    static uint32_t g_bit_depth;
    static uint32_t g_num_dirs;

};

#endif