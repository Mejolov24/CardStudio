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
#define COLOR_1 WHITE
#define COLOR_2 GREEN
#define COLOR_3 BLUE

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
uint8_t virtual_vibrato_frequency_value = 0;
uint8_t virtual_vibrato_range_value = 0;
int16_t virtual_bend_value = 1024;
uint8_t virtual_volume_value = 127;
int8_t virtual_cents_offset = 0;

bool virtual_instrument_override = false;
bool virtual_sustain_override = false;
bool virtual_vibrato_frequency_override = false;
bool virtual_vibrato_range_override = false;
bool virtual_bend_override = false;
bool virtual_volume_override = false;

uint8_t digital_gain = 20;

uint16_t db_gain_table[24] = {
    0,    // -40 dB (effectively mute)
    10,   // -38 dB
    13,   // -36 dB
    16,   // -34 dB
    20,   // -32 dB
    25,   // -30 dB
    32,   // -28 dB
    41,   // -26 dB
    51,   // -24 dB
    65,   // -22 dB
    81,   // -20 dB
    102,  // -18 dB
    129,  // -16 dB
    162,  // -14 dB
    204,  // -12 dB
    257,  // -10 dB
    324,  // -8 dB
    408,  // -6 dB
    513,  // -4 dB
    646,  // -2 dB
    1024, //  0 dB (Unity Gain)
    1289, // +2 dB
    1622, // +4 dB
    2048  // +6 dB
};

void update_volume(){M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));}
void update_basenote(){synth.setup(base_note,sample_rate, (float)virtual_cents_offset);}
void update_digital_gain(){synthcore.set_digital_gain(db_gain_table[digital_gain]);}

String base_pitches[128] = {
    "C-1","C0","C1","C2","C3","C4","C5","C6","C7"
};

String midi_note_names[128] = {
    "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
    "C0",  "C#0",  "D0",  "D#0",  "E0",  "F0",  "F#0",  "G0",  "G#0",  "A0",  "A#0",  "B0",
    "C1",  "C#1",  "D1",  "D#1",  "E1",  "F1",  "F#1",  "G1",  "G#1",  "A1",  "A#1",  "B1",
    "C2",  "C#2",  "D2",  "D#2",  "E2",  "F2",  "F#2",  "G2",  "G#2",  "A2",  "A#2",  "B2",
    "C3",  "C#3",  "D3",  "D#3",  "E3",  "F3",  "F#3",  "G3",  "G#3",  "A3",  "A#3",  "B3",
    "C4",  "C#4",  "D4",  "D#4",  "E4",  "F4",  "F#4",  "G4",  "G#4",  "A4",  "A#4",  "B4",
    "C5",  "C#5",  "D5",  "D#5",  "E5",  "F5",  "F#5",  "G5",  "G#5",  "A5",  "A#5",  "B5",
    "C6",  "C#6",  "D6",  "D#6",  "E6",  "F6",  "F#6",  "G6",  "G#6",  "A6",  "A#6",  "B6",
    "C7",  "C#7",  "D7",  "D#7",  "E7",  "F7",  "F#7",  "G7",  "G#7",  "A7",  "A#7",  "B7",
    "C8",  "C#8",  "D8",  "D#8",  "E8",  "F8",  "F#8",  "G8",  "G#8",  "A8",  "A#8",  "B8",
    "C9",  "C#9",  "D9",  "D#9",  "E9",  "F9",  "F#9",  "G9"
};

String db_str_table[24] = {
    "-40 dB",
    "-38 dB",
    "-36 dB",
    "-34 dB",
    "-32 dB",
    "-30 dB",
    "-28 dB",
    "-26 dB",
    "-24 dB",
    "-22 dB",
    "-20 dB",
    "-18 dB",
    "-16 dB",
    "-14 dB",
    "-12 dB",
    "-10 dB",
    "-8 dB",
    "-6 dB",
    "-4 dB",
    "-2 dB",
    "0 dB",
    "+2 dB",
    "+4 dB",
    "+6 dB"
};


M5SDE::ExplorerTheme sd_theme = {
    .directory_color = YELLOW,
    .background_color = BLACK,
    .border_color = GREEN,
    .selection_color = DARKCYAN,
    .item_height = 23,
    .item_window = 5,
    .font = &fonts::FreeSans12pt7b
};
M5Menu::MenuTheme menu_theme = {
    .background_color = BLACK,
    .border_color = COLOR_3,
    .selection_color = DARKCYAN,
    .item_height = 23,
    .item_window = 5,
    .font = &fonts::FreeSans12pt7b
};



M5Menu::MenuItem AudioSettings[] = {
    {
        "Volume", // name
        &volume, // pointer to variable
        5, // increment
        0, // minimum
        100,// maximum
        update_volume
    },
    {
        "Digital Gain",
        &digital_gain,
        db_str_table,
        update_digital_gain
    },
    {
        "Base Note",
        &base_note,
        midi_note_names,
        update_basenote
    },
    {
        "Cents Tunning",
        &virtual_cents_offset,
        1,
        INT8_MIN,
        INT8_MAX,
        update_basenote
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
        "Vibrato hz override",
        &virtual_vibrato_frequency_override
    },
    {
        "Vibrato hz value",
        &virtual_vibrato_frequency_value,
        1,
        0,
        30
    },
    {
        "Vibrato range override",
        &virtual_vibrato_range_override
    },
    {
        "Vibrato range value",
        &virtual_vibrato_range_value,
        1,
        0,
        12
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
uint16_t octaves = 3;
uint8_t visual_c_index = 5;
uint8_t note_height = 24;

uint8_t virtual_piano_octave = 5;
uint8_t virtual_piano_channel = 0;
M5Menu::MenuItem PianoRollSettings[] = {
    {
        "Octaves",
        &octaves,
        1,
        1,
        5
    },
    {
        "Base Note",
        &visual_c_index,
        base_pitches
    },
    {
        "Note Height",
        &note_height,
        5,
        1,
        135
    }
};


M5Menu::MenuItem VirtualPianoSettings[] = {
    {
        "Octave",
        &virtual_piano_octave,
        base_pitches
    },
    {
        "Channel",
        &virtual_piano_channel,
        1,
        0,
        15
    }
};
extern SDState current_sd_state;
extern MidiSD midi_sd;
void select_midi(){
    current_sd_state = PICK_MIDI;
    open_sd();
}
void select_spack(){
    current_sd_state = PICK_SPACK;
    open_sd();
}
void play_midi(){midi_sd.set_midi_state(MidiSD::MidiState::PLAY);}
void pause_midi(){midi_sd.set_midi_state(MidiSD::MidiState::PAUSE);}
void stop_midi(){midi_sd.set_midi_state(MidiSD::MidiState::STOP); stopAllVoices();}
M5Menu::MenuItem MidiSettings[] = {
    {
        "Select Midi File",
        select_midi
    },
    {
        "Play",
        play_midi
    },
    {
        "Pause",
        pause_midi
    },
    {
        "Stop",
        stop_midi
    }
};

M5Menu::Menu AudioMenu = {0, AudioSettings};
M5Menu::Menu ChannelOverrideMenu = {1, ChannelOverrideSettings};
M5Menu::Menu PianoRollMenu = {2, PianoRollSettings};
M5Menu::Menu VirtualPianoMenu = {3, VirtualPianoSettings};
M5Menu::Menu MidiMenu = {4, MidiSettings};
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
        "Piano Roll",
        &PianoRollMenu
    },
    {
        "Virtual Piano",
        &VirtualPianoMenu
    },
    {
        "Midi Settings",
        &MidiMenu
    },
    {
        "Burn sample pack",
        select_spack
    },
    {
        "Serial plot",
        &serial_plot
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
    virtual_vibrato_range_override = channel_override.vibrato_range_override;
    virtual_vibrato_frequency_override = channel_override.vibrato_frequency_override;
    virtual_bend_override = channel_override.bend_override;
    virtual_volume_override = channel_override.volume_override;

    virtual_instrument_value = synth.getChannelSID(channel_override_index);
    virtual_sustain_value = channel_parameters.sustain;
    virtual_vibrato_range_value = channel_parameters.vibrato_range;
    virtual_vibrato_frequency_value = channel_parameters.vibrato_frequency;
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
    parameters.vibrato_frequency = (float)virtual_vibrato_frequency_value / 100.0f;
    parameters.vibrato_range = virtual_vibrato_range_value;
    parameters.pitch_bend = virtual_bend_value;
    parameters.volume = virtual_volume_value;
    
    override_parameters.instrument_override = virtual_instrument_override;
    override_parameters.sustain_override = virtual_sustain_override;
    override_parameters.vibrato_range_override = virtual_vibrato_range_override;
    override_parameters.vibrato_frequency_override = virtual_vibrato_frequency_override;
    override_parameters.bend_override = virtual_bend_override;
    override_parameters.volume_override = virtual_volume_override;

    // sync parameters
    synth.SetChannnelOverrides(channel_override_index,override_parameters);
    synth.SetChannelInstrument(true, channel_override_index, virtual_instrument_value);
    synth.SetChannelParameters(true, channel_override_index, parameters);
}