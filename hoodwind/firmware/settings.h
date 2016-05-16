#ifndef _HOODWIND_settings_h_
#define _HOODWIND_settings_h_

#include <EEPROM.h>

#include "instrument.h"

class Instrument;

class Settings {
  public:
    static void save(Instrument *instrument);
    static void load(Instrument *instrument);
};

#endif
