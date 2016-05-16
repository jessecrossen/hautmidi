#include "settings.h"

#define SETTINGS_VERSION 'A'

void Settings::save(Instrument *instrument) {
  int i, j;
  int address = 0;
  static uint8_t buffer[255];
  uint8_t bytes;
  // write the version so we can validate it when we read settings
  EEPROM.write(address++, SETTINGS_VERSION);
  // get the components to save settings for
  Model *models[3] = {
      instrument->input(),
      instrument->scale(),
      instrument->synth()
    };
  for (i = 0; i < 3; i++) {
    bytes = models[i]->storageBytes();
    EEPROM.write(address++, bytes);
    if (bytes > 0) {
      models[i]->store(buffer);
      for (j = 0; j < bytes; j++) {
        EEPROM.write(address++, buffer[j]);
      }
    }
  }
}

void Settings::load(Instrument *instrument) {
  int i, j;
  int address = 0;
  static uint8_t buffer[255];
  uint8_t bytes;
  uint8_t value;
  // validate the version
  value = EEPROM.read(address++);
  if (value != SETTINGS_VERSION) return;
  // get the components to save settings for
  Model *models[3] = {
      instrument->input(),
      instrument->scale(),
      instrument->synth()
    };
  for (i = 0; i < 3; i++) {
    bytes = models[i]->storageBytes();
    value = EEPROM.read(address++);
    if (value != bytes) return;
    if (bytes > 0) {
      for (j = 0; j < bytes; j++) {
        buffer[j] = EEPROM.read(address++);
      }
      models[i]->recall(buffer);
    }
  }
}
