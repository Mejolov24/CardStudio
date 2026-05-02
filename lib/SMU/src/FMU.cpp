#include <Arduino.h>
#include <SD.h>
#include "esp_partition.h"
#include "FMU.h"

uint32_t FMU::g_sample_rate = 22050;
uint32_t FMU::g_bit_depth   = 16;
uint32_t FMU::g_num_dirs    = 0;
uintptr_t FMU::flash_base   = 0;
spi_flash_mmap_handle_t FMU::mmap_handle;
SampleData FMU::instruments[128];
SampleData FMU::percussion[128];

FMU::Result FMU::burnSamplePack(const char* path) {
    File file = SD.open(path);
    if (!file) {
        Serial.println("SD: File not found!");
        return FMU::Result::FileNotFound;
    }

    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)0x40, ESP_PARTITION_SUBTYPE_ANY, "samples");

    if (!part) {
        Serial.println("Partition 'samples' missing!");
        return FMU::Result::PartitionNotFound;
    }

    if (file.size() > part->size) {
        Serial.println("ERROR: SPACK too large!");
        return FMU::Result::SizeMismatch;
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
    return FMU::Result::Success;
}
FMU::Result FMU::mapSamplePack() {
    const esp_partition_t* part = esp_partition_find_first(
        (esp_partition_type_t)0x40, ESP_PARTITION_SUBTYPE_ANY, "samples");

    if (!part) {
        Serial.println("FMU: Partition 'samples' not found!");
        return FMU::Result::PartitionNotFound;
    }

    // Map the partition to the CPU's memory space
    esp_err_t err = esp_partition_mmap(
        part, 0, part->size,
        SPI_FLASH_MMAP_DATA,
        (const void**)&flash_base,
        &mmap_handle
    );

    if (err != ESP_OK) {
        Serial.printf("FMU: mmap failed: %d\n", err);
        return FMU::Result::MmapFailed;
    }

    // --- HEADER VALIDATION ---
    if (memcmp((void*)flash_base, "SPK1", 4) != 0) {
        Serial.println("FMU: Invalid SPACK Magic!");
        return FMU::Result::InvalidMagic;
    }

    // Read header values using pointer arithmetic
    g_num_dirs    = *(uint32_t*)(flash_base + 4);
    g_sample_rate = *(uint32_t*)(flash_base + 8);
    g_bit_depth   = *(uint32_t*)(flash_base + 12);

    Serial.printf("FMU: Dirs=%u SR=%u BD=%u\n", g_num_dirs, g_sample_rate, g_bit_depth);

    if (g_num_dirs < 2) {
        Serial.println("FMU: Invalid SPACK - Not enough banks!");
        return FMU::Result::InvalidBankCount;
    }

    // Get offsets to the instrument and percussion tables
    uint32_t* directory = (uint32_t*)(flash_base + 16);
    uint32_t inst_offset = directory[0];
    uint32_t perc_offset = directory[1];

    // --- ZERO-COPY MAPPING ---
    // We point directly to the Flash memory instead of copying it to the Stack
    SampleDataRaw* flash_raw_inst = (SampleDataRaw*)(flash_base + inst_offset);
    SampleDataRaw* flash_raw_perc = (SampleDataRaw*)(flash_base + perc_offset);

    for (int i = 0; i < 128; i++) {
        // Map Instrument Bank
        if (flash_raw_inst[i].length > 0 && flash_raw_inst[i].length < 0xFFFFFF) {
            instruments[i].length     = flash_raw_inst[i].length;
            instruments[i].loop_start = flash_raw_inst[i].loop_start;
            instruments[i].loop_end   = flash_raw_inst[i].loop_end;
            
            // Transform file offsets into real CPU memory addresses
            instruments[i].data = (const int16_t*)(flash_base + flash_raw_inst[i].data);
            instruments[i].name = (const char*)(flash_base + flash_raw_inst[i].name);
        } else {
            instruments[i].length = 0;
        }

        // Map Percussion Bank
        if (flash_raw_perc[i].length > 0 && flash_raw_perc[i].length < 0xFFFFFF) {
            percussion[i].length     = flash_raw_perc[i].length;
            percussion[i].loop_start = flash_raw_perc[i].loop_start;
            percussion[i].loop_end   = flash_raw_perc[i].loop_end;

            percussion[i].data = (const int16_t*)(flash_base + flash_raw_perc[i].data);
            percussion[i].name = (const char*)(flash_base + flash_raw_perc[i].name);
        } else {
            percussion[i].length = 0;
        }
    }

    Serial.println("FMU: Audio system ready.");
    return FMU::Result::Success;
}