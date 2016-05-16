#ifndef _HOODWIND_instrument_h_
#define _HOODWIND_instrument_h_

#include "input.h"
#include "scale.h"
#include "synth.h"
#include "settings.h"

class Settings;

class Instrument {
  public:
    Instrument();
  
    static Instrument *instance();
    
    void update();
    
    InputModel *input();
    ScaleModel *scale();
    SynthModel *synth();
    
    static void save();
    static void load();
  
  private:
    InputModel *_input;
    ScaleModel *_scale;
    SynthModel *_synth;
};

#endif
