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

struct ChannelOverride{
    bool instrument_override = false;
    bool sustain_override = false;
    bool vibrato_override = false;
    bool bend_override = false;
    bool volume_override = false;
};

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


