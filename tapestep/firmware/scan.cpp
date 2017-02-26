#include "scan.h"

#include "shift.h"

void scan_init() {
  pinMode(SCAN_MOMENTARIES, INPUT_PULLUP);
  pinMode(SCAN_TOGGLE1, INPUT_PULLUP);
  pinMode(SCAN_TOGGLE2, INPUT_PULLUP);
  pinMode(SCAN_TRACK0, INPUT_PULLUP);
  pinMode(SCAN_TRACK1, INPUT_PULLUP);
  pinMode(SCAN_TRACK2, INPUT_PULLUP);
  pinMode(SCAN_TRACK3, INPUT_PULLUP);
}

scan_t scan() {
  scan_t state;
  // set up the shifter
  shift_fill(1);
  shift_bit(0);
  shift_bit(1);
  shift_enable(1, 1);
  // ground each position in turn
  for (int i = 0; i < 12; i++) {
    // scan momentaries
    if (i == 0) state.go = ! digitalRead(SCAN_MOMENTARIES);
    else if (i == 1) state.stop = ! digitalRead(SCAN_MOMENTARIES);
    else if (i == 2) state.storeB = ! digitalRead(SCAN_MOMENTARIES);
    else if (i == 3) state.storeA = ! digitalRead(SCAN_MOMENTARIES);
    // scan toggles
    else if (i == 4) state.loop = ! digitalRead(SCAN_TOGGLE1);
    else if (i == 5) state.single = ! digitalRead(SCAN_TOGGLE1);
    else if (i == 6) state.selectA = ! digitalRead(SCAN_TOGGLE2);
    else if (i == 7) state.selectB = ! digitalRead(SCAN_TOGGLE2);
    // scan selectors
    if (! digitalRead(SCAN_TRACK0)) state.track[0] = i;
    if (! digitalRead(SCAN_TRACK1)) state.track[1] = i;
    if (! digitalRead(SCAN_TRACK2)) state.track[2] = i;
    if (! digitalRead(SCAN_TRACK3)) state.track[3] = i;
    // scan the next position
    shift_bit(1);
  }
  // disable the shifter for safety
  shift_enable(0, 0);
  return(state);
}

void print_scan(scan_t state) {
  Serial.print(state.go     ? "G " : "  ");
  Serial.print(state.stop   ? "S " : "  ");
  Serial.print(state.loop   ? "L " : "  ");
  Serial.print(state.single ? "S " : "  ");
  for (int i = 0; i < 4; i++) {
    Serial.print(state.track[i]);
    Serial.print(state.track[i] < 10 ? "  " : " ");
  }
  Serial.print(state.storeA  ? "a " : "  ");
  Serial.print(state.selectA ? "A " : "  ");
  Serial.print(state.selectB ? "B " : "  ");
  Serial.print(state.storeB  ? "b " : "  ");
  Serial.println("");
}

