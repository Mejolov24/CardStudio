#include <Arduino.h>
#include "M5Cardputer.h"
#include "M5CADVKeyCB.h"

void M5CADVKeyCB::SetupKeyboardCallback(KeyCallback callback) {
    KeyboardCallback = callback;
}

void M5CADVKeyCB::KeyboardUpdate() {
    
    if (M5Cardputer.Keyboard.isChange()) {
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
      uint8_t* currentKeys = status.hid_keys.data();
      uint8_t currentCount = status.hid_keys.size();

      if (status.opt){KeyboardCallback(0,true);} // lazy opt implementation, change later on.

      // pressed
      for (uint8_t i = 0; i < currentCount; i++){
        uint8_t key = currentKeys[i];
        bool found = false;

        for (uint8_t k = 0; k < previousCount; k++){
          if (previousKeys[k] == key){found = true; break;}}
        
        if (!found && KeyboardCallback){KeyboardCallback(key,true);} // check if this is a freshly pressed key
      }

      // released (same as pressed but inverted current and previous counts for release detection)
      for (uint8_t i = 0; i < previousCount; i++){
        uint8_t key = previousKeys[i];
        bool found = false;

        for (uint8_t k = 0; k < currentCount; k++){
          if (currentKeys[k] == key){found = true; break;}}
        
        if (!found && KeyboardCallback){KeyboardCallback(key,false);}
    }
  previousCount = currentCount;
  for (uint8_t i = 0; i < currentCount; i++) { // simply store previous keys
      previousKeys[i] = currentKeys[i];
  }
  }
}

