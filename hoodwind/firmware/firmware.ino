#include "debug.h"
#include "instrument.h"
#include "interface.h"

Interface *interface;
Instrument *instrument;

void setup() {
  Serial.begin(38400);
/*  #if DEBUG*/
/*    while ((! Serial) && (elapsedMillis() < 1000));*/
/*  #endif*/
/*  LOG("READY");*/

  instrument = Instrument::instance();
  instrument->update();
  
  interface = new Interface();
  interface->begin();
}

void loop() {
  instrument->update();
  interface->update();
}
