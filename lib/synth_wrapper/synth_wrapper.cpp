#include <synth_wrapper.h>
int16_t* SynthWrapper::getAudioBuffer(){
  if (!_buffer_index) return _BufferB; 
  else return _BufferA;
}

void SynthWrapper::updateAudioBuffer(){
  int16_t* _current_buffer;
  if (!_buffer_index){_current_buffer = _BufferA;}
  else {_current_buffer = _BufferB;}

  for (int i = 0; i < BUFFER_SIZE; i++){
    synthcore.stepAudio();
    _current_buffer[i] = synthcore.master_mix;
    for(uint16_t ch = 0; ch < 16; ch++){channel_TX_buffers[ch][i] = synthcore.channel_output[ch];}
  }
  _buffer_index = !_buffer_index;
  tx_buffer_index = 0;
}


uint8_t SynthWrapper::getSIDorFallback(uint8_t SID,bool is_percussion){
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

void SynthWrapper::ProcessMidi(MidiMessage msg) {
    SynthCore::ChannelParameters params = channels_parameters[msg.channel];
    bool is_percussion (msg.channel == 9);
    switch (msg.type) {
        
        case MidiType::NoteOn:
            if (msg.data2 > 0) {
                if (!is_percussion){
                    synthcore.createVoice(instruments + channels_sid[msg.channel],msg.data1,msg.data2,msg.channel,false);
                }
                else {synthcore.createVoice(percussion + getSIDorFallback(msg.data1,true),msg.data1,msg.data2,msg.channel,true);}
            } else {
                synthcore.releaseVoiceByNote(msg.data1,msg.channel);
            }
            break;
        case MidiType::NoteOff:
                    synthcore.releaseVoiceByNote(msg.data1, msg.channel);
                    break;
        case MidiType::ProgramChange:
                    SetChannelInstrument(false,msg.channel,msg.data1);
                    break;

        case MidiType::ControlChange:
            if (msg.data1 == 64) {
                params.sustain = (msg.data2 >= 64);
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
        SetChannelParameters(false,msg.channel,params);
}


// checks if override matches the channel override values, then applies changes.
// override is true when changing from ui, and false from incoming midi data
void SynthWrapper::SetChannelParameters(bool override, uint8_t channel, SynthCore::ChannelParameters parameters){
    ChannelOverride ch_override = channels_overrides[channel];
    if (override == ch_override.sustain_override) channels_parameters[channel].sustain = parameters.sustain;
    if (override == ch_override.vibrato_override) channels_parameters[channel].vibrato = parameters.vibrato;
    channels_parameters[channel].pitch_bend = parameters.pitch_bend;
    if (override == ch_override.volume_override) channels_parameters[channel].volume = parameters.volume;
    synthcore.setChannelParameters(channel,channels_parameters[channel]);
}

void SynthWrapper::SetChannelInstrument(bool override, uint8_t channel, uint8_t instrument){
    bool is_percussion = (channel == 9);
    ChannelOverride ch_override = channels_overrides[channel];
    if (override == ch_override.instrument_override) channels_sid[channel] = getSIDorFallback(instrument,is_percussion);
}



void SynthWrapper::SetChannnelOverrides(uint8_t channel, ChannelOverride overrides){
  channels_overrides[channel] = overrides;
}

SynthWrapper::ChannelOverride SynthWrapper::GetChannelOverrides(uint8_t channel){
  return channels_overrides[channel];
}

SynthCore::ChannelParameters SynthWrapper::GetChannelParameters(uint8_t channel){
  return synthcore.getChannelParameters(channel);
}

void SynthWrapper::setup(uint8_t base_note, uint16_t sampling_rate){
  synthcore.setup(base_note,sampling_rate);
}

void SynthWrapper::KillAllVoices(){
  synthcore.KillAllVoices();
}

void SynthWrapper::setSamplePointers(const SampleData* inst, const SampleData* perc) {
    instruments = inst;
    percussion = perc;
}
uint8_t SynthWrapper::getChannelSID(uint8_t channel){
    return channels_sid[channel];
}