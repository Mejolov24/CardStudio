// WARNING: THIS CODE IS FOR TESTING AND DOES NOT REFLECT THE FINAL MAIN.CPP //

#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>

#include <FMU.h>
#include <MP.h>
#include <SynthCore.h>
#include <M5SDE.h>
#include <M5CADVKeyCB.h>
#include <M5Config.h>
M5Canvas canvas(&M5.Lcd);
M5CADVKeyCB keyHandler;
M5SDE sdex;
M5Config config;
FMU fmu;
SynthCore synth;
MidiParser mp;

#include "config_definitions.h"

// --- SD PINS ---
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

uint8_t channel_instruments[16]; // contains the SID of the channel.
bool at_settings = false;
bool at_sd = false;

uint8_t volume = 50;
uint8_t base_note = 69;

uint16_t serial_tx_speed = 8000;
bool serial_plot = false;

// channel overrides
uint8_t channel_override_index;
uint8_t virtual_volume_override_value = false;
bool virtual_sustain_override = false;
bool virtual_sustain_override_value = false;
bool virtual_volume_override = false;
//

struct VirtualChannelOverride{
    bool sustain_override = false;
    bool sustain_override_value = false;
    bool volume_override_value = false;
    uint8_t volume_override = false;
};

hw_timer_t *timer = NULL;

// since double buffering is needed, i implemented channel tx buffers because buffer generation is "instant"
// and serial plotting is not.
#define BUFFER_SIZE 256
int16_t _BufferA[BUFFER_SIZE];
int16_t _BufferB[BUFFER_SIZE];
bool _buffer_index = 0;

int16_t channel_TX_buffers[16][BUFFER_SIZE];
uint32_t tx_buffer_index = 0;

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
    .item_window = 4,
    .font = &fonts::FreeSans12pt7b
};
M5Config::ExplorerTheme config_theme = {
    .background_color = 0x211a, // blue
    .border_color = 0x2c9f,
    .selection_color = 0x06e0,
    .item_height = 23,
    .item_window = 4,
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
        "Channel id",
        &channel_override_index,
        1,
        0,
        15
    },
    {
        "Sustain override",
        &virtual_sustain_override
    },
    {
        "Sustain value",
        &virtual_sustain_override_value
    },
    {
        "Volume override",
        &virtual_volume_override
    },
    {
        "Volume value",
        &virtual_volume_override_value,
        1,
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
    }
};

M5Config::ConfigMenu MainMenu = {
    .config_items = MainSettings, 
    .size = sizeof(MainSettings) / sizeof(MainSettings[0])
};



uint8_t getSIDorFallback(uint8_t SID,bool is_percussion){
    const SampleData* current_array = instruments;
    if (is_percussion) current_array = percussion;
    if (SID > 127) SID = 0;
    if ((current_array + SID)->length != 0) return SID;
    
    uint8_t categoryStart = (SID / 8) * 8;
    uint8_t categoryEnd = categoryStart + 7;

    for (uint8_t i = categoryStart; i <= categoryEnd; i++){
        if (current_array[i].length != 0) return i;
    }

    for (uint8_t i = 0; i < 128; i++){
        if (current_array[i].length != 0) return i;
    }
    return 0;
}
void setChannelInstrument(uint8_t channel, u8_t instrument, bool is_percussion){
    instrument = getSIDorFallback(instrument, is_percussion);
    channel_instruments[channel] = instrument;
}

void ProcessMidi(MidiMessage msg) {
    SynthCore::ChannelParameters params = synth.getChannelParameters(msg.channel);
    bool is_percussion (msg.channel == 9);
    switch (msg.type) {
        
        case MidiType::NoteOn:
            if (msg.data2 > 0) {
                if (!is_percussion){
                    synth.createVoice(instruments + channel_instruments[msg.channel],msg.data1,msg.data2,msg.channel,false);
                }
                else {synth.createVoice(percussion + getSIDorFallback(msg.data1,true),msg.data1,msg.data2,msg.channel,true);}
            } else {
                synth.releaseVoiceByNote(msg.data1,msg.channel);
            }
            break;
        case MidiType::NoteOff:
                    synth.releaseVoiceByNote(msg.data1, msg.channel);
                    break;
        case MidiType::ProgramChange:
                    setChannelInstrument(msg.channel, getSIDorFallback(msg.data1,is_percussion), is_percussion);
                    break;

        case MidiType::ControlChange:
            if (msg.data1 == 64) {
                params.sustain = (msg.data2 >= 64);
                Serial.println(params.sustain);
            }
            if (msg.data1 == 7){
                params.volume = msg.data2;
            }
            break;

        case MidiType::PitchBend:{
            int16_t offset = msg.getPitchBend(); 
            int16_t scaled_offset = offset >> 6; 
            params.pitch_bend = 1024 + scaled_offset;
            break;}

        default:
            break;
    }
    synth.setChannelParameters(msg.channel,params);
}

void setup_samples(){
    fmu.mapSamplePack();
    instruments = fmu.getInstruments();
    percussion = fmu.getPercussion();
    sample_rate = fmu.getSampleRate();

}


void OnUsage(M5Config::ConfigItem* item,M5Config::ConfigMenu* menu){
    switch (menu->id)
    {
    case 0:
        synth.setBaseNote(base_note);
        M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));
        break;
    case 1:
    
    case 2:
        if(serial_plot){
        timerAlarmWrite(timer, serial_tx_speed, true);
        timerAlarmEnable(timer);
        }
        else{if (timer) timerAlarmDisable(timer);}
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

void OnSelection(const char* path){// returns the absolute path of selected item
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

volatile bool sendFlag = false;
void IRAM_ATTR sendSample() {
  sendFlag = true;
}

int16_t* getAudioBuffer(){
  if (!_buffer_index) return _BufferB; 
  else return _BufferA;
}

void updateAudioBuffer(){
  int16_t* _current_buffer;
  if (!_buffer_index){_current_buffer = _BufferA;}
  else {_current_buffer = _BufferB;}

  for (int i = 0; i < BUFFER_SIZE; i++){
    synth.stepAudio();
    _current_buffer[i] = synth.master_mix;
    for(uint16_t ch = 0; ch < 16; ch++){channel_TX_buffers[ch][i] = synth.channel_output[ch];}
  }
  _buffer_index = !_buffer_index;
  tx_buffer_index = 0;
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
    M5.Speaker.setVolume(volume);

    Serial.begin();
    timer = timerBegin(0, 80, true); 
    timerAttachInterrupt(timer, &sendSample, true);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);

    keyHandler.SetupKeyboardCallback(OnKey);
    mp.setCallback(ProcessMidi);

    config.begin(&canvas,OnUsage);
    config.goToMenu(&MainMenu);
    config.setTheme(&config_theme);

    sdex.setTheme(&sd_theme);
    sdex.begin(&canvas,OnSelection);

    setup_samples();
}

void loop() {
    M5Cardputer.update();
    keyHandler.KeyboardUpdate();

    if (!M5.Speaker.isPlaying()) {
        updateAudioBuffer();
        M5.Speaker.playRaw(getAudioBuffer(), BUFFER_SIZE, sample_rate);}
    while (Serial.available() > 0) {
            uint8_t incomingByte = Serial.read();
            mp.process(incomingByte);
        }
        
    if (sendFlag) {
        sendFlag = false;
        for(int i = 0; i < 16; i++) {
            int16_t val = channel_TX_buffers[i][tx_buffer_index * (sample_rate / serial_tx_speed)]; 
            Serial.write(255);          // Header
            Serial.write(i);            // Channel ID
            Serial.write(val >> 8);     // High Byte (MSB)
            Serial.write(val & 0xFF);   // Low Byte (LSB)
        }
        tx_buffer_index ++;
    }
}