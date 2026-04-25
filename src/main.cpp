#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "esp_partition.h"

// --- SD PINS ---
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

// --- STRUCT (20 bytes, MUST MATCH PYTHON) ---
struct SampleData {
    const char* name;
    const int16_t* data;
    uint32_t length;
    uint32_t loop_start;
    uint32_t loop_end;
};
struct __attribute__((packed)) SampleDataRaw {
    uint32_t name;
    uint32_t data;
    uint32_t length;
    uint32_t loop_start;
    uint32_t loop_end;
};

// --- GLOBALS ---
SampleData instruments[128];
SampleData percussion[128];

uintptr_t flash_base = 0;
spi_flash_mmap_handle_t mmap_handle;

uint32_t g_sample_rate = 22050;
uint32_t g_bit_depth   = 16;
uint32_t g_num_dirs    = 0;

bool burnSamplePack(const char* path) {
    File file = SD.open(path);
    if (!file) {
        Serial.println("SD: File not found!");
        return false;
    }

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)0x40, ESP_PARTITION_SUBTYPE_ANY, "samples");

    if (!part) {
        Serial.println("Partition 'samples' missing!");
        return false;
    }

    if (file.size() > part->size) {
        Serial.println("ERROR: SPACK too large!");
        return false;
    }

    Serial.println("Burning to Flash...");

    size_t erase_size = ((file.size() + 4095) / 4096) * 4096;
    esp_partition_erase_range(part, 0, erase_size);

    uint8_t buffer[4096];
    size_t offset = 0;

    while (file.available()) {
        size_t readLen = file.read(buffer, sizeof(buffer));
        esp_partition_write(part, offset, buffer, readLen);
        offset += readLen;
    }

    file.close();
    Serial.println("Burn complete.");
    return true;
}

void mapSamplePack() {
    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)0x40, ESP_PARTITION_SUBTYPE_ANY, "samples");

    if (!part) {
        Serial.println("Partition not found!");
        return;
    }

    esp_err_t err = esp_partition_mmap(
        part, 0, part->size,
        SPI_FLASH_MMAP_DATA,
        (const void**)&flash_base,
        &mmap_handle
    );

    if (err != ESP_OK) {
        Serial.printf("mmap failed: %d\n", err);
        return;
    }

    // --- HEADER ---
char magic[5] = {0};
    memcpy(magic, (void*)flash_base, 4);
    Serial.printf("Magic: %s\n", magic);

    if (strcmp(magic, "SPK1") != 0) {
        Serial.println("Invalid SPACK!");
        // Add this to see what is actually there:
        for(int i=0; i<16; i++) Serial.printf("%02X ", ((uint8_t*)flash_base)[i]);
        Serial.println();
        return;
    }

    g_num_dirs    = *(uint32_t*)(flash_base + 4);
    g_sample_rate = *(uint32_t*)(flash_base + 8);
    g_bit_depth   = *(uint32_t*)(flash_base + 12);

    Serial.printf("Dirs=%u SR=%u BD=%u\n",
        g_num_dirs, g_sample_rate, g_bit_depth);

    uint32_t* directory = (uint32_t*)(flash_base + 16);

    if (g_num_dirs < 2) {
        Serial.println("Not enough banks!");
        return;
    }

    uint32_t inst_offset = directory[0];
    uint32_t perc_offset = directory[1];

    // --- TEMP RAW BUFFERS ---
    SampleDataRaw raw_inst[128];
    SampleDataRaw raw_perc[128];

    memcpy(raw_inst, (void*)(flash_base + inst_offset), sizeof(raw_inst));
    memcpy(raw_perc, (void*)(flash_base + perc_offset), sizeof(raw_perc));

    // --- CONVERT RAW → RUNTIME ---
    for (int i = 0; i < 128; i++) {
        // Instruments
        if (raw_inst[i].length > 0) {
            instruments[i].length     = raw_inst[i].length;
            instruments[i].loop_start = raw_inst[i].loop_start;
            instruments[i].loop_end   = raw_inst[i].loop_end;

            instruments[i].data = (const int16_t*)(flash_base + raw_inst[i].data);
            instruments[i].name = (const char*)(flash_base + raw_inst[i].name);
        } else {
            instruments[i].length = 0;
        }

        // Percussion
        if (raw_perc[i].length > 0) {
            percussion[i].length     = raw_perc[i].length;
            percussion[i].loop_start = raw_perc[i].loop_start;
            percussion[i].loop_end   = raw_perc[i].loop_end;

            percussion[i].data = (const int16_t*)(flash_base + raw_perc[i].data);
            percussion[i].name = (const char*)(flash_base + raw_perc[i].name);
        } else {
            percussion[i].length = 0;
        }
    }

    Serial.println("Audio system ready.");
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    Serial.begin(115200);
    delay(2000);

    Serial.println("--- CARDSTUDIO START ---");

    M5.Speaker.setVolume(128);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        Serial.println("SD failed!");
        return;
    }

    Serial.println("SD OK.");

    burnSamplePack("/AppData/CardStudio/SamplePacks/output.spack");

    mapSamplePack();
}

void loop() {
    M5Cardputer.update();
}