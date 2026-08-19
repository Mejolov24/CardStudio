#include <stdint.h>
#include <M5GFX.h>
extern m5gfx::M5Canvas canvas;
struct Note
{
    uint16_t pos_x = 0;
    float offset_y = 0;
    uint16_t width = 0;
    float height = 0;
    uint16_t color = 0;
    uint8_t note = 0;
    uint8_t channel = 0;
    bool held = false;
    bool active = false;
};

#define MAX_NOTES 255
uint16_t channel_colors[16] = {RED, ORANGE, YELLOW, GREEN, GREEN, GREEN, CYAN, BLUE, TFT_LIGHTBLUE, BLUE, BLUE, PURPLE, PURPLE, PINK, PINK};
Note notes[MAX_NOTES];
uint8_t sorted_NID[MAX_NOTES];
uint8_t active_note_count = 0;

void removeIDFromSortedNID(uint8_t NID) {
    int targetIndex = -1;
    for (int i = 0; i < active_note_count; i++) {
        if (sorted_NID[i] == NID) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex != -1) {
        for (int i = targetIndex; i < active_note_count - 1; i++) {
            sorted_NID[i] = sorted_NID[i + 1];
        }
        // Update the count and clear the now-trailing slot
        active_note_count--;
        sorted_NID[active_note_count] = 255; // Use 255 as an empty marker
    }
}

uint8_t _allocateNID(){
  if (active_note_count == MAX_NOTES){
    uint8_t oldest_NID = sorted_NID[0];
    notes[oldest_NID].held = false;
    removeIDFromSortedNID(oldest_NID);
    return oldest_NID;
  }

for (int i = 0; i < MAX_NOTES ; i++) {
    if (!notes[i].active) {
      // Ensure this slot isn't already lingering in sorted_NID
      bool already_tracked = false;
      for (int j = 0; j < active_note_count; j++) {
        if (sorted_NID[j] == i) {
          already_tracked = true;
          break;
        }
      }
      if (!already_tracked) {
        return i;
      }
    }
  }
  return sorted_NID[0];
}

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define BLACK_KEY_DIVIDER 1.7

extern uint16_t octaves;
extern uint8_t visual_c_index;
extern uint8_t note_height;

void hold_note(uint8_t note, uint16_t channel){
  uint8_t visual_base_note = (visual_c_index * 12);
  if (note < visual_base_note) return;
  uint8_t note_id = _allocateNID();
  Note& current_note = notes[note_id];

  uint16_t total_white_keys = (octaves * 7);
  while (SCREEN_WIDTH % total_white_keys != 0) {
      total_white_keys++;
  }
  uint16_t white_key_width = SCREEN_WIDTH / total_white_keys;
  uint16_t black_key_width = white_key_width / BLACK_KEY_DIVIDER;

  uint8_t semitone = note % 12;
  // Algorithmic white key offset within the octave (0 to 6)
  bool is_black = (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
  int semitone_diff = note - visual_base_note;
  int octave = semitone_diff / 12;
  uint8_t white_offset = (semitone < 5) ? (semitone / 2) : ((semitone + 1) / 2);
  uint16_t white_key_index = (octave * 7) + white_offset;
  uint16_t white_key_x = white_key_index * white_key_width;
  if (white_key_index >= total_white_keys) return;
  current_note.note = note;
  current_note.channel = channel;
  current_note.offset_y = 0;
  current_note.height = 0;
  current_note.held = true;
  current_note.active = true;
  current_note.color = channel_colors[channel];

  if (semitone < 0) {
    semitone += 12;
    octave--;
  }
  if(is_black){
    current_note.width = black_key_width -1;
    current_note.pos_x = white_key_x + white_key_width - (black_key_width / 2);
  }
  else{
    current_note.width = white_key_width - 1;
    current_note.pos_x = white_key_x;
  }

  sorted_NID[active_note_count] = note_id;
  active_note_count ++;
}

void release_note(uint8_t note, uint8_t channel){
    for (int i = active_note_count - 1; i >= 0 ; i--){
      uint8_t nid = sorted_NID[i];
      Note& current_note = notes[nid];
      if (current_note.channel != channel){continue;}
      if (current_note.note != note){continue;}
      current_note.held = false;
      break;
    }
}

void delete_all_notes(){
  for(uint16_t note_index = 0; note_index < MAX_NOTES; note_index++){
    notes[note_index].held = false;
    notes[note_index].active = false;
    active_note_count = 0;
  for(int i = 0; i < MAX_NOTES; i++) {
      sorted_NID[i] = 255;
    }
  }
}

void render_tick(float dt){
  uint16_t total_white_keys = (octaves * 7);
  while (SCREEN_WIDTH % total_white_keys != 0) {total_white_keys++;}
  uint8_t visual_base_note = (visual_c_index * 12);
  uint16_t white_key_width = SCREEN_WIDTH / total_white_keys;
  uint16_t black_key_width = white_key_width / BLACK_KEY_DIVIDER;
  uint16_t black_key_height = note_height / 2;
  float note_speed = 120.0f;
  // render of keyboard
  for (uint16_t key_index = 0; key_index < total_white_keys; key_index++){
    //white keys
    uint16_t key_x = key_index * white_key_width;
    uint16_t key_y = SCREEN_HEIGHT - note_height;
    canvas.fillRect(key_x,key_y, white_key_width, note_height, WHITE);
    canvas.drawRect(key_x,key_y, white_key_width, note_height, BLACK);
  }
  // Render of colored white keys
  for(int i = active_note_count - 1; i >= 0; i--){
    uint8_t note_id = sorted_NID[i];
    Note& current_note = notes[note_id];
    if (!current_note.held) continue;
    uint8_t note = current_note.note;
    uint8_t semitone = note % 12;
    bool is_black = (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
    if (is_black) continue;
    int semitone_diff = note - visual_base_note;
    int octave = semitone_diff / 12;
    uint8_t white_offset = (semitone < 5) ? (semitone / 2) : ((semitone + 1) / 2);
    uint16_t white_key_index = (octave * 7) + white_offset;
    uint16_t white_key_x = white_key_index * white_key_width;
    uint16_t key_y = SCREEN_HEIGHT - note_height;
    canvas.fillRect(white_key_x,key_y, white_key_width, note_height, current_note.color);
  }

  // render of black keys
  for (uint16_t key_index = 0; key_index < total_white_keys; key_index++){
    uint8_t scale_pos = key_index % 7;
    if (scale_pos == 2 or scale_pos == 6) {
      continue; 
    }
    uint16_t white_key_x = key_index * white_key_width;
    uint16_t key_x = white_key_x + white_key_width - (black_key_width / 2);
    uint16_t key_y = SCREEN_HEIGHT - note_height;
    canvas.fillRect(key_x,key_y, black_key_width, black_key_height, BLACK);
  }

  // Render of colored black keys
  for(int i = active_note_count - 1; i >= 0; i--){
    uint8_t note_id = sorted_NID[i];
    Note& current_note = notes[note_id];
    if (!current_note.held) continue;
    uint8_t note = current_note.note;
    uint8_t semitone = note % 12;
    bool is_black = (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
    if (!is_black) continue;
    int semitone_diff = note - visual_base_note;
    int octave = semitone_diff / 12;
    uint8_t white_offset = (semitone < 5) ? (semitone / 2) : ((semitone + 1) / 2);
    uint16_t white_key_index = (octave * 7) + white_offset;
    uint16_t white_key_x = white_key_index * white_key_width;
    uint16_t key_y = SCREEN_HEIGHT - note_height;
    uint16_t key_x = white_key_x + white_key_width - (black_key_width / 2);
    canvas.fillRect(key_x,key_y, black_key_width, black_key_height, current_note.color);
  }

  // render of ascending keys
  for(int i = active_note_count - 1; i >= 0; i--){
    uint8_t note_id = sorted_NID[i];
    Note& current_note = notes[note_id];
    int32_t render_y = SCREEN_HEIGHT - (current_note.offset_y + note_height);
    if(!current_note.active) continue;
    if(render_y + current_note.height <= 0){
      current_note.active = false;
      removeIDFromSortedNID(note_id);
      continue;}
    current_note.offset_y += note_speed * dt;
    canvas.fillRect(current_note.pos_x, render_y, current_note.width, current_note.height, current_note.color);
    if(!current_note.held) continue;
    current_note.height += note_speed * dt;
  }
}