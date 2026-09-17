#include <MidiSD.h>

uint16_t read_be16(File &f) {
    uint8_t buf[2];
    f.read(buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

uint32_t read_be32(File &f) {
    uint8_t buf[4];
    f.read(buf, 4);
    return ((uint32_t)buf[0] << 24) | 
           ((uint32_t)buf[1] << 16) | 
           ((uint32_t)buf[2] << 8)  | 
            (uint32_t)buf[3];
}

void MidiSD::calculate_timing(uint16_t division)
{
    // MIDI default tempo = 500000 us per quarter note
    // = 120 BPM
    uint32_t current_tempo = 500000;
    uint32_t current_tick = 0;
    uint64_t current_time_us = 0;
    uint16_t tempo_index = 0;
    // Sort tempo changes by tick
    std::sort(
        tempo_changes,
        tempo_changes + tempo_count,
        [](const TempoChange &a, const TempoChange &b) {
            return a.tick < b.tick;
        }
    );

    // Sort MIDI events by tick
    std::sort(
        playback_buffer,
        playback_buffer + message_count,
        [](const TimedMidiMessage &a, const TimedMidiMessage &b) {
            return a.ticks < b.ticks;
        }
    );

    for (uint32_t i = 0; i < message_count; i++) {
        uint32_t event_tick = playback_buffer[i].ticks;
        // Process tempo changes before this event
        while (
            tempo_index < tempo_count &&
            tempo_changes[tempo_index].tick <= event_tick
        ) {
            uint32_t tempo_tick = tempo_changes[tempo_index].tick;
            // Advance time up to the tempo change
            if (tempo_tick > current_tick) {
                uint32_t delta_ticks = tempo_tick - current_tick;
                current_time_us += ((uint64_t)delta_ticks * current_tempo) / division;
                current_tick = tempo_tick;
            }
            // Change tempo
            current_tempo = tempo_changes[tempo_index].us_per_quarter;
            tempo_index++;
        }
        // Advance time from current position to event
        if (event_tick > current_tick) {
            uint32_t delta_ticks = event_tick - current_tick;
            current_time_us += ((uint64_t)delta_ticks * current_tempo) / division;
            current_tick = event_tick;
        }
        playback_buffer[i].time_us = current_time_us;
    }
}

void MidiSD::parse_track(File &file, uint32_t track_len, uint16_t division)
{
    uint32_t track_start = file.position();
    uint32_t track_end = track_start + track_len;
    uint32_t current_ticks = 0;
    uint8_t running_status = 0;

    while (file.position() < track_end) {
        // Read delta-time VLQ
        uint32_t delta_ticks = 0;
        bool vlq_done = false;
        for (int i = 0; i < 4; i++) {
            if (file.position() >= track_end) {return;}
            uint8_t byte = file.read();
            delta_ticks = (delta_ticks << 7) | (byte & 0x7F);
            if (!(byte & 0x80)) {vlq_done = true; break;}
        }
        if (!vlq_done) {return;}

        current_ticks += delta_ticks;
        if (file.position() >= track_end) {return;}
        uint8_t status = file.read();

        // Running status
        if (status < 0x80) {
            if (running_status == 0) {return;}

            file.seek(file.position() - 1);
            status = running_status;
        }
        else if (status < 0xF0) {running_status = status;}
        // Meta event
        if (status == 0xFF) {
            if (file.position() >= track_end) {return;}
            uint8_t meta_type = file.read();

            // Meta-event length
            uint32_t meta_len = 0;
            bool vlq_done = false;
            for (int i = 0; i < 4; i++) {
                if (file.position() >= track_end) {return;}
                uint8_t byte = file.read();
                meta_len = (meta_len << 7) | (byte & 0x7F);

                if (!(byte & 0x80)) {vlq_done = true;break;}
            }

            if (!vlq_done) {return;}
            if (file.position() + meta_len > track_end) {return;}

            // Set Tempo
            if (meta_type == 0x51 && meta_len == 3) {

                uint32_t tempo =
                    ((uint32_t)file.read() << 16) |
                    ((uint32_t)file.read() << 8) |
                    (uint32_t)file.read();

                if (tempo_count < MAX_MIDI_TEMPOS) {
                    tempo_changes[tempo_count].tick = current_ticks;
                    tempo_changes[tempo_count].us_per_quarter = tempo;
                    tempo_count++;
                }
            }
            else {file.seek(file.position() + meta_len);}
            // End of Track
            if (meta_type == 0x2F) {
                file.seek(track_end);
                return;
            }
            continue;
        }

        // SysEx
        if (status == 0xF0 || status == 0xF7) {
            uint32_t sysex_len = 0;
            bool vlq_done = false;
            for (int i = 0; i < 4; i++) {
                if (file.position() >= track_end) {return;}
                uint8_t byte = file.read();
                sysex_len = (sysex_len << 7) | (byte & 0x7F);
                if (!(byte & 0x80)) {vlq_done = true;break;}
            }
            if (!vlq_done) {return;}
            if (file.position() + sysex_len > track_end) {return;}
            file.seek(file.position() + sysex_len);
            continue;
        }

        // MIDI channel message
        if (status >= 0x80 && status <= 0xEF) {
            uint8_t type = status & 0xF0;
            uint8_t channel = status & 0x0F;
            uint8_t data1 = 0;
            uint8_t data2 = 0;

            if (file.position() >= track_end) {return;}
            data1 = file.read();
            if (data1 & 0x80) {continue;}
            // Program Change / Channel Pressure
            // only have one data byte.
            if (type != 0xC0 && type != 0xD0) {
                if (file.position() >= track_end) {return;}
                data2 = file.read();
                if (data2 & 0x80) {continue;}
            }
            if (message_count >= MAX_MIDI_EVENTS) {return;}

            TimedMidiMessage &timed =playback_buffer[message_count++];
            timed.ticks = current_ticks;
            timed.time_us = 0;
            timed.message.type = (MidiType)type;
            timed.message.channel = channel;
            timed.message.data1 = data1;
            timed.message.data2 = data2;
            // Note On with velocity 0 = Note Off
            if (type == 0x90 && data2 == 0) {timed.message.type = MidiType::NoteOff;}
        }

        // System Common
        else if (status >= 0xF1 && status <= 0xF6) {
            running_status = 0;
            uint8_t bytes_to_skip = 0;
            switch (status) {
                case 0xF1:
                    bytes_to_skip = 1;
                    break;
                case 0xF2:
                    bytes_to_skip = 2;
                    break;
                case 0xF3:
                    bytes_to_skip = 1;
                    break;
                case 0xF6:
                    bytes_to_skip = 0;
                    break;
                default:
                    return;
            }

            if (file.position() + bytes_to_skip > track_end) {return;}
            file.seek(file.position() + bytes_to_skip);
        }

        else {
            continue;
        }
    }
    file.seek(track_end);
}


void MidiSD::load_midi(const char* path)
{
    message_count = 0;
    tempo_count = 0;

    File file = SD.open(path);

    if (!file) {
        Serial.println("ERROR: Could not open MIDI file");
        return;
    }

    uint8_t magic[4];

    if (file.read(magic, 4) != 4 ||
        memcmp(magic, "MThd", 4) != 0) {

        Serial.println("ERROR: Invalid MIDI header");

        file.close();
        return;
    }

    uint32_t header_len = read_be32(file);

    if (header_len < 6) {

        Serial.println("ERROR: Invalid MIDI header length");

        file.close();
        return;
    }

    uint16_t format = read_be16(file);
    uint16_t num_tracks = read_be16(file);
    uint16_t division = read_be16(file);

    Serial.printf(
        "MIDI: format=%u tracks=%u division=%u\n",
        format,
        num_tracks,
        division
    );

    // We currently support PPQN MIDI timing.
    if (division & 0x8000) {

        Serial.println(
            "ERROR: SMPTE MIDI timing is not supported"
        );

        file.close();
        return;
    }

    // Skip extra header bytes
    if (header_len > 6) {

        file.seek(
            file.position() + (header_len - 6),
            SeekSet
        );
    }

    uint16_t tracks_parsed = 0;

    while (
        file.available() &&
        tracks_parsed < num_tracks
    ) {

        uint8_t chunk_id[4];

        if (file.read(chunk_id, 4) != 4) {
            break;
        }

        uint32_t chunk_len = read_be32(file);

        if (memcmp(chunk_id, "MTrk", 4) == 0) {

            parse_track(
                file,
                chunk_len,
                division
            );

            tracks_parsed++;
        }
        else {

            file.seek(
                file.position() + chunk_len,
                SeekSet
            );
        }
    }

    file.close();

    // Convert absolute MIDI ticks to real playback time.
    calculate_timing(division);

    Serial.printf(
        "MIDI loaded: %lu events, %u tempo changes\n",
        (unsigned long)message_count,
        tempo_count
    );
}

void MidiSD::set_midi_state(
    MidiState state,
    uint16_t value
)
{
    switch (state) {

        case MidiState::PLAY:

            if (paused) {

                total_paused_us +=
                    current_us - pause_start_us;

                paused = false;
                playing = true;
            }
            else if (!playing) {

                start_us = current_us;

                total_paused_us = 0;

                playback_index = 0;

                playing = true;
                paused = false;
            }

            break;


        case MidiState::PAUSE:

            if (playing && !paused) {

                pause_start_us = current_us;

                playing = false;
                paused = true;
            }

            break;


        case MidiState::STOP:

            playing = false;
            paused = false;

            playback_index = 0;

            total_paused_us = 0;

            break;


        default:
            break;
    }
}
void MidiSD::tick_midi(uint64_t us)
{
    current_us = us;

    if (!playing) {
        return;
    }

    uint64_t time_offset_us =
        current_us -
        start_us -
        total_paused_us;

    while (
        playback_index < message_count &&
        time_offset_us >=
            playback_buffer[playback_index].time_us
    ) {

        if (callback) {
            callback(
                playback_buffer[playback_index].message
            );
        }

        playback_index++;
    }
}


void MidiSD::set_callback(MidiCallback callback_){
    if(callback_){callback = callback_;}
}