#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <stdint.h>

// Human-readable MIDI types
enum class MidiType : uint8_t {
    NoteOff           = 0x80,
    NoteOn            = 0x90,
    Aftertouch        = 0xA0,
    ControlChange     = 0xB0,
    ProgramChange     = 0xC0,
    ChannelPressure   = 0xD0,
    PitchBend         = 0xE0,
    SystemMessage     = 0xF0,
    Unknown           = 0x00
};

struct MidiMessage {
    MidiType type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;

    // Helper to get 14-bit pitch bend immediately
    int16_t getPitchBend() const {
        if (type == MidiType::PitchBend) {
            return (int16_t)((data2 << 7) | data1) - 8192;
        }
        return 0;
    }
};

class MidiParser {
public:
    typedef void (*MidiCallback)(MidiMessage msg);

    MidiParser() : _callback(nullptr), _runningStatus(0), _state(WAIT_STATUS), _data1(0) {}

    void setCallback(MidiCallback cb) { _callback = cb; }

    void process(uint8_t byte) {
        if (byte >= 0xF8) return; // Ignore Real-time for now

        if (byte >= 0x80) {
            _runningStatus = byte;
            _state = (byte < 0xF0) ? WAIT_DATA1 : WAIT_STATUS;
            return;
        }

        if (_state == WAIT_DATA1) {
            _data1 = byte;
            uint8_t mainType = _runningStatus & 0xF0;
            if (mainType == 0xC0 || mainType == 0xD0) {
                dispatch(_runningStatus, _data1, 0);
            } else {
                _state = WAIT_DATA2;
            }
        } 
        else if (_state == WAIT_DATA2) {
            dispatch(_runningStatus, _data1, byte);
            _state = WAIT_DATA1; 
        }
    }

private:
    MidiCallback _callback;
    uint8_t _runningStatus;
    uint8_t _data1;
    enum State { WAIT_STATUS, WAIT_DATA1, WAIT_DATA2 } _state;

    void dispatch(uint8_t status, uint8_t d1, uint8_t d2) {
        if (_callback) {
            _callback({
                static_cast<MidiType>(status & 0xF0),
                static_cast<uint8_t>((status & 0x0F) + 1),
                d1,
                d2
            });
        }
    }
};

#endif