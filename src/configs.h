#include <stdint.h>
#include <M5Menu.h>
#include <M5SDE.h>
#include <M5Unified.h>
#include <synth_wrapper.h>
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

extern void stopAllVoices();
extern void open_sd();
extern SynthWrapper synth;
uint8_t volume = 50;
uint8_t base_note = 69;
uint32_t sample_rate;

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

void update_volume(){M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));}
void update_basenote(){synth.setup(base_note,sample_rate);}
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
M5Menu::MenuTheme menu_theme = {
    .background_color = 0x211a, // blue
    .border_color = 0x2c9f,
    .selection_color = 0x06e0,
    .item_height = 23,
    .item_window = 5,
    .font = &fonts::FreeSans12pt7b
};



M5Menu::MenuItem AudioSettings[] = {
    {
        "Volume", // name
        &volume, // pointer to variable
        10, // increment
        0, // minimum
        100,// maximum
        update_volume
    },
    {
        "Base Note",
        &base_note,
        1,
        0,
        127
    }
};

M5Menu::MenuItem ChannelOverrideSettings[] = {
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
    },
    {
        "Bend override",
        &virtual_bend_override
    },
    {
        "Bend value",
        &virtual_bend_value,
        128,
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

M5Menu::MenuItem IOSettings[] = {
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

M5Menu::Menu AudioMenu = {0, AudioSettings};
M5Menu::Menu ChannelOverrideMenu = {1, ChannelOverrideSettings};
M5Menu::Menu IOMenu = {2, IOSettings};
M5Menu::MenuItem MainSettings[] = {
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

M5Menu::Menu MainMenu = {3, MainSettings};


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
    
    override_parameters.instrument_override = virtual_instrument_override;
    override_parameters.sustain_override = virtual_sustain_override;
    override_parameters.vibrato_override = virtual_vibrato_override;
    override_parameters.bend_override = virtual_bend_override;
    override_parameters.volume_override = virtual_volume_override;

    // sync parameters
    synth.SetChannnelOverrides(channel_override_index,override_parameters);
    synth.SetChannelInstrument(true, channel_override_index, virtual_instrument_value);
    synth.SetChannelParameters(true, channel_override_index, parameters);
}