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
SynthWrapper synth;

#include <configs.h>

bool at_settings = false;
bool at_sd = false;
TaskHandle_t SerialTaskHandle = NULL;
SemaphoreHandle_t synthMutex = NULL;
int16_t serialCopyBuffer[MAX_CHANNELS][BUFFER_SIZE];
extern void delete_all_notes();
void stopAllVoices(){synth.KillAllVoices(); delete_all_notes();}

void SerialTask(void *pvParameters){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
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

void render(){
    canvas.pushSprite(0,0);
    canvas.clear();
}

void open_sd(){
    menu.close();
    sdex.goToAbsoluteDir("/AppData/CardStudio/samplepacks");
    sdex.open();
    at_sd = true;
}


void setup_samples(){
if (xSemaphoreTake(synthMutex, portMAX_DELAY) == pdTRUE) {
        stopAllVoices();
        delete_all_notes();
        fmu.mapSamplePack();
        sample_rate = fmu.getSampleRate();
        synth.setup(base_note, sample_rate);
        synth.setSamplePointers(fmu.getInstruments(), fmu.getPercussion());
        xSemaphoreGive(synthMutex);
    }
}
void OnSelection(const char* path){
    sdex.close();
    at_sd = false;
    at_settings = false;
    canvas.setTextColor(COLOR_1);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.drawString("Preparing Flash...",WIDTH/2,HEIGHT/2,TEXT_FONT);
    render();
    fmu.burnSamplePack(path);
    setup_samples();
}

void burning_progress(uint8_t progress){
    if(progress == 100){render(); return;}
    uint16_t max_width = WIDTH - 32;
    uint16_t inverse_progress = map(progress, 0, 100, 100, 0);
    uint16_t target_width = (max_width * progress) / 100;
    int color = (progress % 2 == 0) ? COLOR_2 : COLOR_1;
    canvas.setTextColor(color);
    canvas.drawRect(16, HEIGHT/2 + 32, max_width, 24,color);
    canvas.fillRect(16, HEIGHT/2 + 32, target_width, 24,COLOR_3);
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
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
    M5.Speaker.setVolume(round((255.0 * (volume / 100.0))));

    Serial.begin();
    synthMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        SerialTask,   /* Task function */
        "SerialTX_Task",  /* Name with human-readable diagnostic value */
        4096,             /* Stack size in bytes */
        NULL,             /* Parameters */
        1,                /* Priority (keep it lower than your audio generation task) */
        &SerialTaskHandle,/* Task handle */
        1                 /* Core ID (0 or 1) */
    );

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);

    keyHandler.SetupKeyboardCallback(OnKey);
    mp.setCallback(MidiCallback);

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

    if (!M5.Speaker.isPlaying()) {
    if (xSemaphoreTake(synthMutex, portMAX_DELAY) == pdTRUE) {
        synth.updateAudioBuffer();
        xSemaphoreGive(synthMutex);
    }
        M5.Speaker.playRaw(synth.getAudioBuffer(), BUFFER_SIZE, sample_rate);
        if (serial_plot){xTaskNotifyGive(SerialTaskHandle);}
    }
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