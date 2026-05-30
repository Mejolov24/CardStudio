// WARNING: THIS CODE IS FOR TESTING AND DOES NOT REFLECT THE FINAL MAIN.CPP //

#include <SPI.h>
#include <SD.h>
#include <FMU.h>
#include <MP.h>
#include <SynthCore.h>
#include "M5Cardputer.h"
#include <M5SDE.h>
#include <M5CADVKeyCB.h>
M5Canvas canvas(&M5.Lcd);
FMU fmu;
M5CADVKeyCB keyHandler;
SynthCore synth;
MidiParser mp;
M5SDE sdex;
using input = M5SDE::Input;
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

uint8_t base_note = 69;
int8_t transpose = 0;
uint8_t volume = 0;

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
                    synth.createVoice(instruments + channel_instruments[msg.channel],msg.data1,msg.data2,msg.channel);
                }
                else {synth.createVoice(percussion + getSIDorFallback(msg.data1,true),msg.data1,msg.data2,msg.channel);}
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

void OnKey(uint8_t key, bool pressed){
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
if(status.del){
    sdex.process_input(input::back);
}
if (status.opt){
    at_settings = !at_settings;
    if (at_sd){
        sdex.close();
        at_sd = false;
    }
}
    if (status.enter){
            
        }
        if(!pressed) return;
        switch (key)
    {
    case 51:
        if(cursor_index != 0){cursor_index --;}
        sdex.process_input(input::up);
        break;
    case 55:
        cursor_index ++;
        sdex.process_input(input::down);
        break;

    case 56:
    if (cursor_index == 0){transpose ++;}
    else if(cursor_index == 1){volume += 10;}
    break;

    case 54:
    if (cursor_index == 0){transpose --;}
    else if(cursor_index == 1){volume -= 10;}
    break;

    default:
        break;
}
    synth.setBaseNote(base_note - transpose);
    M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));
}

void OnSelection(const char* path){// returns the absolute path of selected item
    sdex.close();
    at_sd = false;
    canvas.setTextColor(COLOR_1);
    canvas.drawString("Burning...",0,HEIGHT/2,TEXT_FONT);
    canvas.pushSprite(0,0);
    fmu.burnSamplePack(path);
    setup_samples();
}

hw_timer_t *timer = NULL;
volatile bool sendFlag = false;
void IRAM_ATTR sendSample() {
  sendFlag = true;
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    canvas.createSprite(240, 135);
    keyHandler.SetupKeyboardCallback(OnKey);

    Serial.begin();
    timer = timerBegin(0, 80, true); 
    timerAttachInterrupt(timer, &sendSample, true);
    timerAlarmWrite(timer, 8000, true);
    timerAlarmEnable(timer);

    M5.Speaker.setVolume(128);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        Serial.println("SD failed!");
        return;
    }

    Serial.println("SD OK.");
    setup_samples();
    mp.setCallback(ProcessMidi);
    sdex.begin(&canvas,OnSelection);
}

void loop() {
    M5Cardputer.update();
    keyHandler.KeyboardUpdate();

    if (!M5.Speaker.isPlaying()) {
        synth.updateAudioBuffer();
        M5.Speaker.playRaw(synth.getAudioBuffer(), 256, sample_rate);}
    while (Serial.available() > 0) {
            uint8_t incomingByte = Serial.read();
            mp.process(incomingByte);
        }
if (sendFlag) {
    sendFlag = false;
    for(int i = 0; i < 16; i++) {
        // Get the 16-bit signed value
        int16_t val = synth.channel_output[i]; 

        Serial.write(255);          // Header
        Serial.write(i);            // Channel ID
        Serial.write(val >> 8);     // High Byte (MSB)
        Serial.write(val & 0xFF);   // Low Byte (LSB)
    }
}
}