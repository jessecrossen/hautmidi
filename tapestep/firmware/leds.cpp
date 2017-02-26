#include "leds.h"

// state of all LEDs
led_t leds[LED_COUNT] = {
    { 3 },  // tempo
    { 4 },  // track 1
    { 6 },  // track 2
    { 22 }, // track 3
    { 20 }  // track 4
  };
  
// time since led brightnesses were last reduced
elapsedMillis sinceLastDim;

void update_leds() {
  bool dim = sinceLastDim >= 10;
  if (dim) sinceLastDim = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    analogWrite(leds[i].pin, leds[i].bright);
    if (dim) {
      if (leds[i].bright > 8) leds[i].bright -= leds[i].bright / 8;
      else if (leds[i].bright > 0) leds[i].bright--;
    }
  }
}

void light_led(int i, unsigned char bright) {
  if (i >= LED_COUNT) return;
  leds[i].bright = bright;
  update_leds();
}
void light_led(int i) { light_led(i, 0x80); }

void chase_leds() {
  update_leds();
  elapsedMillis t = 0;
  elapsedMillis frame = 250;
  int i = 0;
  while (t < 500) {
    if (frame >= 75) {
      light_led(i++);
      frame = 0;
    }
    update_leds();
  }
}
