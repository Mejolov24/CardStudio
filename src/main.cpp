#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <FMU.h>
FMU fmu;

// --- SD PINS ---
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12


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

    //fmu.burnSamplePack("/AppData/CardStudio/SamplePacks/output.spack");
    fmu.mapSamplePack();
    const auto* instruments = FMU::getInstruments();
    FMU::SampleData instrument;
    instrument = instruments[0];
    uint32_t sample_rate = fmu.getSampleRate();
    M5.Speaker.playRaw(instrument.data,instrument.length,sample_rate);
}

void loop() {
    M5Cardputer.update();
}