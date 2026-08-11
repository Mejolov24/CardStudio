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

void stopAllVoices(){synth.KillAllVoices();}

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
    fmu.mapSamplePack();
    sample_rate = fmu.getSampleRate();
    synth.setup(base_note,fmu.getSampleRate());
    synth.setSamplePointers(fmu.getInstruments(), fmu.getPercussion());
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

void safeSend(uint8_t byte) {
    if (byte == 255) Serial.write(254);
}

void OnUsage(M5Menu::MenuItem* item, M5Menu::Menu* _menu){
    if (_menu->id == 1) {HandleUIOverrides(); menu.render();}
}

void OnKey(uint8_t key, bool pressed){
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
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

    menu.begin(&canvas,render,OnUsage);
    menu.goToMenu(&MainMenu);
    menu.setTheme(&menu_theme);

    sdex.setTheme(&sd_theme);
    sdex.begin(&canvas,OnSelection);

    setup_samples();
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