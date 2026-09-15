#include <MP.h>
#include <SD.h>
#include <algorithm>
#define MAX_MIDI_EVENTS 12000
#define MAX_MIDI_TEMPOS 100

class MidiSD{
    public:
    enum class MidiState{
        BPM,

        PLAY,
        PAUSE,
        STOP
    };

    private:

    struct TimedMidiMessage {
        MidiMessage message;
        uint32_t ticks;
        uint64_t time_us;
    };
    struct TempoChange {
        uint32_t tick;
        uint32_t us_per_quarter;
    };

    TempoChange tempo_changes[MAX_MIDI_TEMPOS];
    uint16_t tempo_count = 0;

    uint64_t current_us = 0;
    uint64_t start_us = 0;
    uint64_t pause_start_us = 0;
    uint64_t total_paused_us = 0;
    uint32_t playback_index = 0;

    void calculate_timing(uint16_t division);

    typedef void (*MidiCallback)(MidiMessage midi_message);
    MidiCallback callback = nullptr;
    bool paused = false;
    bool playing = false;
    MidiSD::TimedMidiMessage playback_buffer[MAX_MIDI_EVENTS];
    uint32_t message_count = 0;
    void parse_track(File &file, uint32_t track_len, uint16_t division);

    public:
    void set_callback(MidiCallback callback_);
    void load_midi(const char* path);
    void tick_midi(uint64_t us);
    void set_midi_state(MidiState state, uint16_t value = 0);
};