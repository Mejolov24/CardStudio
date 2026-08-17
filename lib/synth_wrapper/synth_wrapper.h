#ifndef SYNTH_WRAPPER_h
    #define SYNTH_WRAPPER_h
#include <stdint.h>
#include <SynthCore.h>
#include <MP.h>
extern SynthCore synthcore;
extern MidiParser mp;
#ifndef BUFFER_SIZE
    #define BUFFER_SIZE 256
#endif


class SynthWrapper{
    public:
        struct ChannelOverride{
            bool instrument_override = false;
            bool sustain_override = false;
            bool vibrato_override = false;
            bool bend_override = false;
            bool volume_override = false;
        };

    private:
        #define BUFFER_SIZE 256
        int16_t _BufferA[BUFFER_SIZE];
        int16_t _BufferB[BUFFER_SIZE];
        bool _buffer_index = 0;

        int16_t channel_TX_buffers[16][BUFFER_SIZE];

        const SampleData* instruments = nullptr;
        const SampleData* percussion = nullptr;

        uint8_t getSIDorFallback(uint8_t SID,bool is_percussion);

        SynthCore::ChannelParameters channels_parameters[16];
        uint8_t channels_sid[16];
        ChannelOverride channels_overrides[16];
    public:
        void setup(uint8_t base_note, uint16_t sampling_rate);
        void ProcessMidi(MidiMessage msg);
        void SetChannnelOverrides(uint8_t channel, ChannelOverride overrides);
        void SetChannelParameters(bool override, uint8_t channel, SynthCore::ChannelParameters parameters);
        void KillAllVoices();
        void SetChannelInstrument(bool override, uint8_t channel, uint8_t instrument);
        void setSamplePointers(const SampleData* inst, const SampleData* perc);
        void updateAudioBuffer();
        int16_t* getAudioBuffer();
        ChannelOverride GetChannelOverrides(uint8_t channel);
        SynthCore::ChannelParameters GetChannelParameters(uint8_t channel);
        uint8_t getChannelSID(uint8_t channel);


};
#endif
