#include <Arduino.h>
#include "M5Cardputer.h"
M5Canvas canvas(&M5.Lcd);
#include <map>
#include "Keyboardmap.h"

#include <M5CADVKeyCB.h>
M5CADVKeyCB keyHandler;
#include <SynthCore.h>
SynthCore synth;
using voiceCfg = SynthCore::VoiceConfig; 

#include "built-in-samples.h"

#define SAMPLE_RATE 22050
int16_t audio_buffer[512];

static int32_t Disw;
static int32_t Dish;
static int32_t Disa; // display area

uint8_t octave = 3;
uint8_t transpose = 0;
uint8_t instrument_index = 0;
uint8_t base_note = 48;
uint8_t volume = 255;
bool at_settings = false;

struct Sample { // add this later on to main library.
  const char* name;
  const int16_t* sample;
  uint32_t sample_len;
  bool is_loop; // yet to be implemented
  uint32_t loop_start;
  uint32_t loop_end;
};

Sample builtInSamples[] = {
  {"Ocarina", ocarina, ocarina_len },
  {"Piano", piano, piano_len },
  {"Organ", organ, organ_len },
  {"Guitar", guitar, guitar_len }
};

#define TEXT_COLOR_2 0x005a
#define TEXT_COLOR 0x761f
#define BG_COLOR 0xe71c
#define TEXT_FONT &fonts::Font4 // 26 px in height

void update_ui(){
  canvas.clearDisplay();
  if(!at_settings){
    canvas.pushSprite(0,0);
    return;
  }
  canvas.setAddrWindow(0, 0, Disw, Dish);
  canvas.setTextColor(TEXT_COLOR);
  canvas.drawString("Instrument :",0, 0, TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR_2);
  canvas.drawString(builtInSamples[instrument_index].name, (Disw / 2) + 20,0, TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR);
  canvas.drawString("Octave :",0,26,TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR_2);
  canvas.drawString(String(octave), (Disw / 2) + 20,26, TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR);
  canvas.drawString("Transpose :",0,52,TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR_2);
  canvas.drawString(String(transpose), (Disw / 2) + 20,52, TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR);
  canvas.drawString("Base note :",0,78,TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR_2);
  canvas.drawString(String(base_note), (Disw / 2) + 20,78, TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR);
  canvas.drawString("Volume :",0,104,TEXT_FONT);
  canvas.setTextColor(TEXT_COLOR_2);
  canvas.drawString(String(volume), (Disw / 2) + 20,104, TEXT_FONT);

  canvas.pushSprite(0,0);
}

void play_note_hid(uint8_t key, bool pressed){
  auto it = hidNoteMap.find(key);
  if (it == hidNoteMap.end()) return;
  if (pressed){
  voiceCfg voice;
  voice.note = hidNoteMap[key]+base_note;
  voice.sample = builtInSamples[instrument_index].sample;
  voice.sample_length = builtInSamples[instrument_index].sample_len;
  synth.addVoice(voice);
}
  else{synth.removeVoice(hidNoteMap[key]+base_note,0);}
}

void OnKey(uint8_t key, bool pressed){
  Serial.print(key);
  Serial.print(" ");
  Serial.print(pressed);
  Serial.println();
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
if (status.opt){
  at_settings = !at_settings;
}
if (!at_settings){
play_note_hid(key,pressed);
}
else if(pressed){
switch (key)
{
case 51:
  octave ++;
  break;

  case 55:
  octave --;
  break;

  case 54:
  transpose --;
  break;

  case 56:
  transpose ++;
  break;

  case 29:
  instrument_index --;
  break;

  case 27:
  instrument_index ++;
  break;
  
  case 45:
  volume -= 14;
  break;

  case 46:
  volume += 14;
  break;

default:
  break;
}
}
octave = (octave % 10 + 10) % 10;
transpose = (transpose % 12 + 12) % 12;
instrument_index = (instrument_index % 4 + 4) % 4;
base_note = (octave * 12) + transpose;
M5.Speaker.setVolume(volume);
if(instrument_index == 0){
  synth.setBaseNote(60);
}
else{synth.setBaseNote(69);}
update_ui();
}

void setup() {
  Serial.begin(9600);
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5.Speaker.begin();
  M5.Speaker.setVolume(255);

  keyHandler.SetupKeyboardCallback(OnKey);
  synth.setBaseNote(60);

    synth.updateAudioBuffer(audio_buffer, 512);
  M5.Speaker.playRaw(audio_buffer, 512, SAMPLE_RATE);

  // Setup rendering
  Disw = M5.Lcd.width();
  Dish = M5.Lcd.height();
  Disa = Dish * Disw;
  canvas.createSprite(240, 135);
}

void loop() {
  // put your main code here, to run repeatedly:
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();

  if (!M5.Speaker.isPlaying()) {
  synth.updateAudioBuffer(audio_buffer, 512);
  M5.Speaker.playRaw(audio_buffer, 512, SAMPLE_RATE);}


}

