#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <FMU.h>
FMU fmu;
#include <SynthCore.h>
SynthCore synth;
#include <M5CADVKeyCB.h>
M5CADVKeyCB keyHandler;

#include <MP.h>
MidiParser mp;

// --- SD PINS ---
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

uint32_t sample_rate;
const SampleData* instruments = nullptr;
const SampleData* percussion = nullptr;

uint8_t channel_instruments[16]; // contains the SID of the channel.


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
if (status.opt){
  fmu.burnSamplePack("/AppData/CardStudio/SamplePacks/output.spack");
  setup_samples();
}
}


void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    keyHandler.SetupKeyboardCallback(OnKey);

    Serial.begin(38400);
    delay(2000);

    Serial.println("--- CARDSTUDIO START ---");

    M5.Speaker.setVolume(128);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        Serial.println("SD failed!");
        return;
    }

    Serial.println("SD OK.");
    //synth.setBaseNote(72);
    setup_samples();
    mp.setCallback(ProcessMidi);
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
}