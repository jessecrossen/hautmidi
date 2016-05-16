#include "instrument.h"

Instrument* Instrument::instance() {
   static Instrument* singleton = NULL;
   if (singleton == NULL) singleton = new Instrument;
   return(singleton);
}

Instrument::Instrument() {
  _input = new InputModel;
  _scale = new ScaleModel;  
  _synth = new SynthModel;
  // load initial settings
  Settings::load(this);
  // save settings when they change
  _input->onStorageChanged = Instrument::save;
  _scale->onStorageChanged = Instrument::save;
  _synth->onStorageChanged = Instrument::save;
}

InputModel *Instrument::input() { return(_input); }
ScaleModel *Instrument::scale() { return(_scale); }
SynthModel *Instrument::synth() { return(_synth); }

void Instrument::update() {
  _input->read();
}

void Instrument::save() {
  Settings::save(Instrument::instance());
}

void Instrument::load() {
  Settings::load(Instrument::instance());
}
