#include <stdint.h>
#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>
#include <FMU.h>
#include <M5SDE.h>
#include <M5CADVKeyCB.h>
#include <M5Config.h>
#include <synth_wrapper.h>
M5Canvas canvas(&M5.Lcd);
M5CADVKeyCB keyHandler;
M5SDE sdex;
M5Config config;
FMU fmu;
SynthWrapper synth;

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

#define WIDTH 240
#define HEIGHT 135
#define ITEM_HEIGHT 26
#define BG_COLOR 0x0008
#define COLOR 0xffff
#define COLOR_1 0xfdc0
#define COLOR_2 0xffe0
#define TEXT_FONT &fonts::Font4 // 26 px in height

uint32_t sample_rate;
const SampleData* instruments = nullptr;
const SampleData* percussion = nullptr;

bool at_settings = false;
bool at_sd = false;

uint8_t volume = 50;
uint8_t base_note = 69;

uint16_t serial_tx_speed = 8000;
bool serial_plot = true;

// channel overrides
uint8_t channel_override_index = 0;
uint8_t previous_channel_override_index = 0;

uint8_t virtual_instrument_value = 0;
bool virtual_sustain_value = false;
int16_t virtual_vibrato_value = 1024;
int16_t virtual_bend_value = 1024;
uint8_t virtual_volume_value = 127;

bool virtual_instrument_override = false;
bool virtual_sustain_override = false;
bool virtual_vibrato_override = false;
bool virtual_bend_override = false;
bool virtual_volume_override = false;


void stopAllVoices(){synth.KillAllVoices();}

void open_sd(){
    config.close();
    sdex.goToAbsoluteDir("/AppData/CardStudio/samplepacks");
    sdex.open();
    at_sd = true;
}

M5SDE::ExplorerTheme sd_theme = {
    .directory_color = 0xf940,
    .background_color = BLACK,
    .border_color = 0xfb40, // orange
    .selection_color = 0x5940, // dim orange
    .text_color = 0xfb40,
    .item_height = 23,
    .item_window = 5,
    .font = &fonts::FreeSans12pt7b
};
M5Config::ExplorerTheme config_theme = {
    .background_color = 0x211a, // blue
    .border_color = 0x2c9f,
    .selection_color = 0x06e0,
    .item_height = 23,
    .item_window = 5,
    .font = &fonts::FreeSans12pt7b
};



M5Config::ConfigItem AudioSettings[] = {
    {
        "Volume", // name
        &volume, // pointer to variable
        10, // increment
        0, // minimum
        100,// maximum
        M5Config::ScrollType::TYPE_CLAMP
    },
    {
        "Base Note",
        &base_note,
        1,
        0,
        127
    }
};

M5Config::ConfigItem ChannelOverrideSettings[] = {
    {
        "Channel",
        &channel_override_index,
        1,
        0,
        15
    },
    {
        "Instrument override",
        &virtual_instrument_override
    },
    {
        "Instrument value",
        &virtual_instrument_value,
        1,
        0,
        127
    },
    {
        "Sustain override",
        &virtual_sustain_override
    },
    {
        "Sustain value",
        &virtual_sustain_value
    },
    {
        "Vibrato override",
        &virtual_vibrato_override
    },
    {
        "Vibrato value",
        &virtual_vibrato_value,
        4,
        INT16_MAX,
        INT16_MIN
    },
    {
        "Bend override",
        &virtual_bend_override
    },
    {
        "Bend value",
        &virtual_bend_value,
        128,
        INT16_MAX,
        INT16_MIN
    },
    {
        "Volume override",
        &virtual_volume_override
    },
    {
        "Volume value",
        &virtual_volume_value,
        128,
        0,
        127
    }
};

M5Config::ConfigItem IOSettings[] = {
    {
        "Serial plot",
        &serial_plot
    },
    {
        "Serial TX rate",
        &serial_tx_speed,
        1000,
        1000,
        44000
    }
};

M5Config::ConfigMenu AudioMenu = {
    .id = 0,
    .config_items = AudioSettings, 
    .size = sizeof(AudioSettings) / sizeof(AudioSettings[0])
};
M5Config::ConfigMenu ChannelOverrideMenu = {
    .id = 1,
    .config_items = ChannelOverrideSettings, 
    .size = sizeof(ChannelOverrideSettings) / sizeof(ChannelOverrideSettings[0])
};
M5Config::ConfigMenu IOMenu = {
    .id = 2,
    .config_items = IOSettings, 
    .size = sizeof(IOSettings) / sizeof(IOSettings[0])
};


M5Config::ConfigItem MainSettings[] = {
    {
        "Audio",
        &AudioMenu
    },
    {
        "Channel Overrides",
        &ChannelOverrideMenu
    },
    {
        "I/O",
        &IOMenu
    },
    {
        "Burn sample pack",
        open_sd
    },
    {
        "Stop all voices",
        stopAllVoices
    }
};

M5Config::ConfigMenu MainMenu = {
    .config_items = MainSettings, 
    .size = sizeof(MainSettings) / sizeof(MainSettings[0])
};

void setup_samples(){
    fmu.mapSamplePack();
    instruments = fmu.getInstruments();
    percussion = fmu.getPercussion();
    sample_rate = fmu.getSampleRate();
    synth.setup(base_note,sample_rate);

}

void OnSelection(const char* path){
    sdex.close();
    at_sd = false;
    at_settings = false;
    canvas.setTextColor(COLOR_1);
    canvas.drawString("Burning...",0,HEIGHT/2,TEXT_FONT);
    canvas.pushSprite(0,0);
    fmu.burnSamplePack(path);
    canvas.clear();
    canvas.pushSprite(0,0);
    setup_samples();
}

void UpdateVirtualOverrides(){
    SynthWrapper::ChannelOverride channel_override = synth.GetChannelOverrides(channel_override_index);
    SynthCore::ChannelParameters channel_parameters = synth.GetChannelParameters(channel_override_index);
    virtual_instrument_override = channel_override.instrument_override;
    virtual_sustain_override = channel_override.sustain_override;
    virtual_vibrato_override = channel_override.vibrato_override;
    virtual_bend_override = channel_override.bend_override;
    virtual_volume_override = channel_override.volume_override;

    virtual_instrument_value = synth.getChannelSID(channel_override_index);
    virtual_sustain_value = channel_parameters.sustain;
    virtual_vibrato_value = channel_parameters.vibrato;
    virtual_bend_value = channel_parameters.pitch_bend;
    virtual_volume_value = channel_parameters.volume;
}

void HandleUIOverrides(){
    SynthCore::ChannelParameters parameters;
    SynthWrapper::ChannelOverride override_parameters;
    if (previous_channel_override_index != channel_override_index){
        UpdateVirtualOverrides();
        previous_channel_override_index = channel_override_index;
        return;}
    // set parameters
    parameters.sustain = virtual_sustain_value;
    parameters.vibrato = virtual_vibrato_value;
    parameters.pitch_bend = virtual_bend_value;
    parameters.volume = virtual_volume_value;

    override_parameters.sustain_override = virtual_sustain_override;
    override_parameters.vibrato_override = virtual_vibrato_override;
    override_parameters.bend_override = virtual_bend_override;
    override_parameters.volume_override = virtual_volume_override;

    // sync parameters
    synth.SetChannnelOverrides(channel_override_index,override_parameters);
    synth.SetChannelInstrument(true, channel_override_index, virtual_instrument_value);
    synth.SetChannelParameters(true, channel_override_index, parameters);
}

void OnUsage(M5Config::ConfigItem* item,M5Config::ConfigMenu* menu){
    switch (menu->id)
    {
    case 0:
        synth.setup(base_note,sample_rate);
        M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));
        break;
    case 1:
        HandleUIOverrides();
        config.render();
        break;
    case 2:
        break;
    default:
        break;
    }


}

void OnKey(uint8_t key, bool pressed){
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if(status.del){
        sdex.process_input(M5SDE::Input::back);
        config.process_input(M5Config::Input::BACK);
    }
    if (status.opt){
        if(!at_settings){config.open(); sdex.close();} else {config.close(); canvas.pushSprite(0,0);}
        at_settings = !at_settings;
    }
    if (status.enter){
        sdex.process_input(M5SDE::Input::select);
        config.process_input(M5Config::Input::SELECT);
        }
    if(!pressed) return;
    switch (key)
        {
        case 51:
            sdex.process_input(M5SDE::Input::up);
            config.process_input(M5Config::Input::UP);
            break;
        case 55:
            sdex.process_input(M5SDE::Input::down);
            config.process_input(M5Config::Input::DOWN);
            break;

        case 54: // left
        config.process_input(M5Config::Input::LEFT);
        break;

        case 56:// right
        config.process_input(M5Config::Input::RIGHT);
        break;

        default:
            break;
}
}

void MidiCallback(MidiMessage msg)
{
    synth.ProcessMidi(msg);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
    M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));

    Serial.begin();

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);

    keyHandler.SetupKeyboardCallback(OnKey);
    mp.setCallback(MidiCallback);

    config.begin(&canvas,OnUsage);
    config.goToMenu(&MainMenu);
    config.setTheme(&config_theme);

    sdex.setTheme(&sd_theme);
    sdex.begin(&canvas,OnSelection);

    setup_samples();

}

void safeSend(uint8_t byte) {
    if (byte == 255) Serial.write(254);
}

void loop() {
    M5Cardputer.update();
    keyHandler.KeyboardUpdate();

    if (!M5.Speaker.isPlaying()) {
        synth.updateAudioBuffer();
        M5.Speaker.playRaw(synth.getAudioBuffer(), BUFFER_SIZE, sample_rate);}
    while (Serial.available() > 0) {
            uint8_t incomingByte = Serial.read();
            mp.process(incomingByte);
        }
}