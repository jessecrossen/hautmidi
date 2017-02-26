#include <Encoder.h>
#include <Bounce.h>

#include "leds.h"
#include "shift.h"
#include "scan.h"

void setup() {
  Serial.begin(9600);
  shift_init();
  scan_init();
  chase_leds();
}

elapsedMillis t;

void loop() {
  update_leds();
  if (t > 250) {
    print_scan(scan());
    t = 0;
  }
}
