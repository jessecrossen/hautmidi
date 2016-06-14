#include <LiquidCrystal.h>

#include <Encoder.h>
#include <Bounce.h>

#include "modes.h"
#include "audio.h"

// foot switches
#define SWITCH_COUNT 4
#define FOOT_SWITCH_DEBOUNCE 10 // milliseconds
int switchPins[SWITCH_COUNT] = { 30, 31, 32, 33 };
Bounce switches[SWITCH_COUNT] = {
  Bounce(switchPins[0], FOOT_SWITCH_DEBOUNCE),
  Bounce(switchPins[1], FOOT_SWITCH_DEBOUNCE),
  Bounce(switchPins[2], FOOT_SWITCH_DEBOUNCE),
  Bounce(switchPins[3], FOOT_SWITCH_DEBOUNCE)
};

// interface modes
Interface *interface = NULL;

void setup() {
  int i;
  // start the interface
  interface = new Interface(
    new LiquidCrystal(24, 25, 26, 27, 28, 29),
    new Encoder(2, 1),
    new Bounce(3, 10));
  // configure button/switch pins
  pinMode(3, INPUT_PULLUP);
  for (i = 0; i < SWITCH_COUNT; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
  }
}

void loop() {
  interface->update();
}
