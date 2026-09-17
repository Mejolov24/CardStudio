#define BUFFER_SIZE 256
#include <stdint.h>
#include <SPI.h>
#include <SD.h>
#include <M5Cardputer.h>
#include <FMU.h>
#include <M5Menu.h>
#include <M5Menu.h>
#include <M5SDE.h>
#include <M5CADVKeyCB.h>
#include <synth_wrapper.h>
#include <falling_notes.h>
#include <map>
#include "Keyboardmap.h"
#include <MidiSD.h>
#include "logo.h"
#include "boot_chime_sample.h"

#define FPS 30
#define RENDER_US (1000000 / FPS)
uint32_t lastFrameTime = 0;

M5Canvas canvas(&M5.Lcd);
M5CADVKeyCB keyHandler;
M5SDE sdex;
M5Menu menu;
FMU fmu;
SynthCore synthcore;
MidiParser mp;
MidiSD midi_sd;
SynthWrapper synth;

bool at_settings = false;
bool at_sd = false;
TaskHandle_t SerialTaskHandle = NULL;
SemaphoreHandle_t synthMutex = NULL;
SemaphoreHandle_t serialSemaphore = NULL;
int16_t serialCopyBuffer[MAX_CHANNELS][BUFFER_SIZE];
extern void delete_all_notes();
void stopAllVoices(){synth.KillAllVoices(); delete_all_notes();}

enum SDState{
    PICK_SPACK,
    PICK_MIDI
};
SDState current_sd_state;
#include <configs.h>

void SerialTask(void *pvParameters){
    while(true){
        if (xSemaphoreTake(serialSemaphore, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(synthMutex, portMAX_DELAY) == pdTRUE) {
            for (int channel_id = 0; channel_id < MAX_CHANNELS; channel_id++){
                for(int index = 0; index < BUFFER_SIZE; index++){
                    serialCopyBuffer[channel_id][index] = synth.channel_TX_buffers[channel_id][index];
                }
            }
            xSemaphoreGive(synthMutex);
            }
        for (int channel_id = 0; channel_id < MAX_CHANNELS; channel_id++){
                    Serial.write(0xAA);
                    Serial.write(0x55);
                    Serial.write(channel_id);
                    Serial.write((uint8_t*)serialCopyBuffer[channel_id], BUFFER_SIZE * sizeof(int16_t));
                }
        }
    }
}

void render(){
    canvas.pushSprite(0,0);
    canvas.clear();
}

void open_sd(){
    menu.close();
    switch (current_sd_state)
    {
    case PICK_SPACK:{
        sdex.goToAbsoluteDir("/AppData/CardStudio/samplepacks");
        break;
    }

    case PICK_MIDI:{
        sdex.goToAbsoluteDir("/Music/Midi");
        break;
    }
    
    default:
        break;
    }

    sdex.open();
    at_sd = true;
}


void setup_samples(){
if (xSemaphoreTake(synthMutex, portMAX_DELAY) == pdTRUE) {
        stopAllVoices();
        delete_all_notes();
        fmu.mapSamplePack();
        sample_rate = fmu.getSampleRate();
        synth.setup(base_note, sample_rate, (float)virtual_cents_offset);
        synth.setSamplePointers(fmu.getInstruments(), fmu.getPercussion());
        xSemaphoreGive(synthMutex);
    }
}

void handle_flash_burn(const char* path){
    bool success = false;
    canvas.setTextColor(COLOR_1);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.drawString("Preparing Flash...",WIDTH/2,HEIGHT/2,TEXT_FONT);
    render();
    FMU::Result result = fmu.burnSamplePack(path);
    canvas.setTextColor(RED);
    switch (result)
    {
    case FMU::Result::Success:
        at_settings = false;
        success = true;
        break;
    case FMU::Result::PartitionNotFound :
        canvas.drawString("Partition Not found!",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;
    case FMU::Result::FlashEraseError:
        canvas.drawString("Flash Errase error!",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;
    case FMU::Result::FlashWriteError :
        canvas.drawString("Flash Write error!",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;
    case FMU::Result::InvalidBankCount :
        canvas.drawString("Invalid sample count!",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;
    case FMU::Result::InvalidMagic :
        canvas.drawString("This is not an .spack!",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;
    case FMU::Result::MmapFailed :
        canvas.drawString("Memory Error!",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;
    case FMU::Result::SizeMismatch :
        canvas.drawString(".spack too big",WIDTH/2,HEIGHT/2,TEXT_FONT);
        break;

    default:
        break;
    }
    if(!success){
        render();
        delay(1000);
        at_settings = true;
        at_sd = true;
        sdex.open();
    }
    canvas.setTextColor(WHITE);
    render();
    setup_samples();
}

void OnSelection(const char* path){
    sdex.close();
    at_sd = false;
    switch (current_sd_state)
    {
    case PICK_SPACK:
        handle_flash_burn(path);
        break;
    case PICK_MIDI:
        midi_sd.load_midi(path);
        midi_sd.set_midi_state(MidiSD::MidiState::PLAY);
        break;
    default:
        break;
    }
    at_settings = false;
    render();
}

void burning_progress(uint8_t progress){
    if(progress == 100){render(); return;}
    uint16_t max_width = WIDTH - 32;
    uint16_t inverse_progress = map(progress, 0, 100, 100, 0);
    uint16_t target_width = (max_width * progress) / 100;
    int color = (progress % 2 == 0) ? COLOR_2 : COLOR_1;
    canvas.setTextColor(color);
    canvas.fillRect(16, HEIGHT/2 + 32, target_width, 24,COLOR_3);
    canvas.drawRect(16, HEIGHT/2 + 32, max_width, 24,color);
    canvas.drawString("Burning Flash...",WIDTH/2,HEIGHT/2,TEXT_FONT);
    canvas.drawString(String(progress),WIDTH/2,HEIGHT/2 + 46,TEXT_FONT);
    render();
}

void OnUsage(M5Menu::MenuItem* item, M5Menu::Menu* _menu){
    if (_menu->id == 1) {HandleUIOverrides(); menu.render();}
}

void MidiCallback(MidiMessage msg)
{
    switch (msg.type) {
        case MidiType::NoteOn:
            if (msg.data2 > 0 and msg.channel != 9) {hold_note(msg.data1, msg.channel);}
            else {release_note(msg.data1,msg.channel);}
            break;
        case MidiType::NoteOff:
            release_note(msg.data1, msg.channel);
            break;
        }
    synth.ProcessMidi(msg);
}


void handle_virtual_piano(uint8_t key, bool pressed){
    auto it = hidNoteMap.find(key);
    if (it == hidNoteMap.end()) return;
    MidiMessage virtual_midi;
    uint8_t note_offset = hidNoteMap[key];
    if (pressed){virtual_midi.type = MidiType::NoteOn;}
    else{virtual_midi.type = MidiType::NoteOff;}
    virtual_midi.channel = virtual_piano_channel;
    virtual_midi.data1 = (virtual_piano_octave * 12) + note_offset;
    MidiCallback(virtual_midi);
}

void OnKey(uint8_t key, bool pressed){
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if(!at_settings and !at_sd){handle_virtual_piano(key, pressed);}
    if(status.del){
        sdex.process_input(M5SDE::Input::back);
        menu.process_input(M5Menu::Input::BACK);
    }
    if (status.opt){
        if(!at_settings){menu.open(); sdex.close();} else {menu.close(); canvas.pushSprite(0,0);}
        at_settings = !at_settings;
    }
    if (status.enter){
        sdex.process_input(M5SDE::Input::select);
        menu.process_input(M5Menu::Input::SELECT);
        }
    if(!pressed) return;
    switch (key)
        {
        case 51:
            sdex.process_input(M5SDE::Input::up);
            menu.process_input(M5Menu::Input::UP);
            break;
        case 55:
            sdex.process_input(M5SDE::Input::down);
            menu.process_input(M5Menu::Input::DOWN);
            break;

        case 54: // left
        menu.process_input(M5Menu::Input::LEFT);
        break;

        case 56:// right
        menu.process_input(M5Menu::Input::RIGHT);
        break;

        default:
            break;
    }
}

void setup_sd(){
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);
    SD.mkdir("/AppData");
    SD.mkdir("/AppData/CardStudio");
    SD.mkdir("/Music");
    SD.mkdir("/Music/Midi");

}

void setup_serial(){
    Serial.begin();
    synthMutex = xSemaphoreCreateMutex();
    serialSemaphore = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(
        SerialTask,   /* Task function */
        "SerialTX_Task",  /* Name with human-readable diagnostic value */
        4096,             /* Stack size in bytes */
        NULL,             /* Parameters */
        1,                /* Priority (keep it lower than your audio generation task) */
        &SerialTaskHandle,/* Task handle */
        1                 /* Core ID (0 or 1) */
    );
}

bool at_boot = true;

void bootAnimationTask(void *pvParameters){
    M5.Speaker.setVolume(255);
    vTaskDelay(pdMS_TO_TICKS(600)); // warmup for speaker I2S
    canvas.pushImage(0, 0, 240, 135, logo);
    render();
    synthcore.createVoice(&output[2],66,127,0);
    synthcore.createVoice(&output[2],70,127,0);
    synthcore.createVoice(&output[2],73,127,0);
    synthcore.createVoice(&output[2],77,127,0);
    //vTaskDelay(pdMS_TO_TICKS(125));
    //synthcore.createVoice(&output[2],92,127,0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    at_boot = false;
    M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));
    synth.KillAllVoices();
    vTaskDelete(NULL);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
    xTaskCreate(bootAnimationTask,"BootAnim", 2048,NULL,1,NULL);
    setup_serial();
    setup_sd();

    //callbacks
    keyHandler.SetupKeyboardCallback(OnKey);
    mp.setCallback(MidiCallback);
    midi_sd.set_callback(MidiCallback);
    menu.begin(&canvas,render,OnUsage);
    menu.goToMenu(&MainMenu);
    menu.setTheme(&menu_theme);
    sdex.setTheme(&sd_theme);
    sdex.begin(&canvas,OnSelection);
    fmu.begin(burning_progress);
    setup_samples();
    lastFrameTime = micros();
}

void loop() {
    uint32_t us = micros();

    M5Cardputer.update();
    keyHandler.KeyboardUpdate();
    midi_sd.tick_midi(us);

    if (!M5.Speaker.isPlaying()) {
    if (xSemaphoreTake(synthMutex, portMAX_DELAY) == pdTRUE) {
        synth.updateAudioBuffer();
        xSemaphoreGive(synthMutex);
    }
        M5.Speaker.playRaw(synth.getAudioBuffer(), BUFFER_SIZE, at_boot ? 8000 : sample_rate);
        if (serial_plot){xSemaphoreGive(serialSemaphore);}
    }
    if(at_boot) return;
    while (Serial.available() > 0) {
            uint8_t incomingByte = Serial.read();
            mp.process(incomingByte);
        }
    if (!at_settings and (us - lastFrameTime >= RENDER_US) ){
        float dt = (us - lastFrameTime) / 1000000.0f;
        lastFrameTime = us;
        render_tick(dt);
        render();
    }
}