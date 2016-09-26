#include <LiquidCrystal.h>
#include <Encoder.h>
#include <Bounce.h>
#include <SPI.h>

#include "modes.h"
#include "audio.h"

// foot switches
#define SWITCH_COUNT 4
#define FOOT_SWITCH_DEBOUNCE 10 // milliseconds

// interface modes
Interface *interface = NULL;

void setup() {
  Serial.begin(38400);
  int i;
  int switchPins[SWITCH_COUNT] = { 30, 31, 32, 33 };  
  // configure button/switch pins
  pinMode(3, INPUT_PULLUP);
  Bounce **switches = new Bounce*[SWITCH_COUNT];
  for (i = 0; i < SWITCH_COUNT; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
    switches[i] = new Bounce(switchPins[i], FOOT_SWITCH_DEBOUNCE);
  }
  // configure SPI pins for the audio shield
  SPI.setMOSI(7);
  SPI.setSCK(14);
  // start the interface
  interface = new Interface(
    new LiquidCrystal(24, 25, 26, 27, 28, 29),
    new Encoder(2, 1),
    new Bounce(3, 10),
    switches);
  // load stored interface state
  interface->load();
}

void loop() {
  interface->update();
}
