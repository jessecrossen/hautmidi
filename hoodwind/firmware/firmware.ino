#include "debug.h"
#include "instrument.h"
#include "interface.h"

Interface *interface;
Instrument *instrument;

void setup() {
  Serial.begin(38400);
  
  delay(100);

  instrument = Instrument::instance();
  instrument->update();
  
  interface = new Interface();
  interface->begin();
}

void loop() {
  instrument->update();
  interface->update();
}
