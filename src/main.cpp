#include <Arduino.h>
#include "M5Cardputer.h"
#include <map>

#include <M5CADVKeyCB.h>
M5CADVKeyCB keyHandler;
#include <SynthCore.h>
SynthCore synth;
using voiceCfg = SynthCore::VoiceConfig; 

#include "output.h"

#define SAMPLE_RATE 22050
int16_t audio_buffer[512];

std::map<uint8_t, uint8_t> hidNoteMap = {
{43,0}, // C 
{53,1}, // C#
{20,2}, // D
{30,3}, // D#
{26,4}, // E
{8,5}, // F
{32,6}, // F#
{21,7}, // G
{33,8}, // G#
{23,9}, // A
{34,10}, // A#
{28,11}, // B / Y

{24,12}, // C 
{36,13}, // C#
{12,14}, // D
{37,15}, // D#
{18,16}, // E
{19,17}, // F
{39,18}, // F#
{47,19}, // G
{45,20}, // G#
{48,21}, // A
{46,22}, // A#
{49,23}, // B / 

{29,24}, // C 
{22,25}, // C#
{27,26}, // D
{7,27}, // D#
{6,28}, // E
{25,29}, // F
{10,30}, // F#
{5,31}, // G
{11,32}, // G#
{17,33}, // A
{13,34}, // A#
{16,35}, // B / Y
};

void OnKey(uint8_t key, bool pressed){
  /*Serial.print(key);
  Serial.print(" ");
  Serial.print(pressed);
  Serial.println();*/
  auto it = hidNoteMap.find(key);
  if (it == hidNoteMap.end()) return;
  if (pressed){
  voiceCfg voice;
  voice.note = hidNoteMap[key]+48;
  voice.sample = ocarina;
  voice.sample_length = ocarina_len;
  synth.addVoice(voice);
}
  else{synth.removeVoice(hidNoteMap[key]+48,0);}
}

void setup() {
  //Serial.begin(9600);
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5.Speaker.begin();
  M5.Speaker.setVolume(255);

  keyHandler.SetupKeyboardCallback(OnKey);
  synth.setBaseNote(60);

    synth.updateAudioBuffer(audio_buffer, 512);
  M5.Speaker.playRaw(audio_buffer, 512, SAMPLE_RATE);
}

void loop() {
  // put your main code here, to run repeatedly:
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();

  if (!M5.Speaker.isPlaying()) {
  synth.updateAudioBuffer(audio_buffer, 512);
  M5.Speaker.playRaw(audio_buffer, 512, SAMPLE_RATE);}


}

