#include "scan.h"

#include "shift.h"
#include "mux.h"

// thresholds for detecting digital conditions from analog readings
#define ACCENT_THRESH 3500
#define TRACK_THRESH  3000

void scan_init() {
  pinMode(SCAN_MOMENTARIES, INPUT_PULLUP);
  pinMode(SCAN_TOGGLE1, INPUT_PULLUP);
  pinMode(SCAN_TOGGLE2, INPUT_PULLUP);
}

void scan_digital(scan_t *state) {
  int i;
  // restore pullups for upper pins (see end of function for why)
  pinMode(SCAN_TRACK0, INPUT_PULLUP);
  pinMode(SCAN_TRACK1, INPUT_PULLUP);
  pinMode(SCAN_TRACK2, INPUT_PULLUP);
  pinMode(SCAN_TRACK3, INPUT_PULLUP);
  // set up the shifter
  shift_enable(1, 1);
  shift_fill(1);
  shift_bit(0);
  // reset parts of the state that won't definitely be set
  for (i = 0; i < 4; i++) {
    state->track[i] = 0;
  }
  // ground each position in turn
  for (i = 0; i < 12; i++) {
    shift_bit(1);
    // scan momentaries
    if (i == 0) state->go |= ! digitalRead(SCAN_MOMENTARIES);
    else if (i == 1) state->stop |= ! digitalRead(SCAN_MOMENTARIES);
    else if (i == 2) state->storeB |= ! digitalRead(SCAN_MOMENTARIES);
    else if (i == 3) state->storeA |= ! digitalRead(SCAN_MOMENTARIES);
    // scan toggles
    else if (i == 4) state->single = ! digitalRead(SCAN_TOGGLE1);
    else if (i == 5) state->loop = ! digitalRead(SCAN_TOGGLE1);
    else if (i == 6) state->selectB = ! digitalRead(SCAN_TOGGLE2);
    else if (i == 7) state->selectA = ! digitalRead(SCAN_TOGGLE2);
    // scan selectors
    if (! digitalRead(SCAN_TRACK0)) state->track[0] = i;
    if (! digitalRead(SCAN_TRACK1)) state->track[1] = i;
    if (! digitalRead(SCAN_TRACK2)) state->track[2] = i;
    if (! digitalRead(SCAN_TRACK3)) state->track[3] = i;
  }
  // put upper input pins into a high-impedence state so voltage doesn't
  //  flow back onto the accent strips
  pinMode(SCAN_TRACK0, INPUT);
  pinMode(SCAN_TRACK1, INPUT);
  pinMode(SCAN_TRACK2, INPUT);
  pinMode(SCAN_TRACK3, INPUT);
}

void scan_analog(scan_t *state) {
  int i, j;
  int m[16];
  int v[16];
  int readDelay = 100; // microseconds
  // reset the parts of the state that don't accumulate
  for (i = 0; i < 16; i++) {
    state->beats[i].track = -1;
    state->beats[i].accent = 0;
  }
  // scan for beat accents
  shift_enable(1, 1);
  shift_set(0x0080);
  for (i = 0; i < 8; i++) {
    mux_read(m, readDelay);
    for (j = 0; j < 16; j++) {
      if (m[j] > ACCENT_THRESH) state->beats[j].accent = 1;
    }
    shift_bit(0);
  }
  // scan for which track beats are on
  for (i = 0; i < 16; i++) v[i] = 0;
  shift_set(0xC000);
  shift_enable(1, 0);  
  for (i = 0; i < 4; i++) {
    mux_read(m, readDelay);
    for (j = 0; j < 16; j++) {
      if (m[j] > TRACK_THRESH) {
        // if a read is high on multiple tracks, it's probably floating
        if (v[j]) state->beats[j].track = -1;
        else {
          state->beats[j].track = i;
          v[j] = 1;
        }
      }
    }
    shift_bit(0);
    shift_bit(0);
  }
  // scan for beat positions, averaging opposing measurements from each side
  //  and accumulating for later integration
  for (i = 0; i < 16; i++) v[i] = 0;
  shift_set(0x5500);
  mux_read(m, readDelay);
  for (i = 0; i < 16; i++) {
    v[i] = m[i];
  }
  shift_set(0xAA00);
  mux_read(m, readDelay);
  for (i = 0; i < 16; i++) {
    v[i] += 4095 - m[i];
    state->beats[i].position += v[i] >> 1;
  }
}

void scan(scan_t *state) {
  scan_digital(state);
  scan_analog(state);
  state->scanCount++;
  // disable the shifter when not scanning for safety
  shift_enable(0, 0);
}

void integrate_scan(scan_t *state) {
  int c = state->scanCount;
  for (int i = 0; i < 16; i++) {
    state->beats[i].position /= c;
  }
  state->scanCount = 1;
}

void reset_scan(scan_t *state) {
  state->stop = state->go = state->storeA = state->storeB = 0;
}

void print_digital(scan_t *state) {
  Serial.print(state->go     ? "G " : "  ");
  Serial.print(state->stop   ? "S " : "  ");
  Serial.print(state->loop   ? "L " : "  ");
  Serial.print(state->single ? "S " : "  ");
  for (int i = 0; i < 4; i++) {
    Serial.print(state->track[i]);
    Serial.print(state->track[i] < 10 ? "  " : " ");
  }
  Serial.print(state->storeA  ? "a " : "  ");
  Serial.print(state->selectA ? "A " : "  ");
  Serial.print(state->selectB ? "B " : "  ");
  Serial.print(state->storeB  ? "b " : "  ");
  Serial.println("");
}

void print_accents(scan_t *state) {
  for (int i = 0; i < 16; i++) {
    Serial.print(state->beats[i].accent);
    Serial.print(" ");
  }
  Serial.println("");
}

void print_tracks(scan_t *state) {
  int t;
  for (int i = 0; i < 16; i++) {
    t = state->beats[i].track;
    if (t >= 0) Serial.print(t);
    else Serial.print("-");
    Serial.print(" ");
  }
  Serial.println("");
}

void print_positions(scan_t *state) {
  for (int i = 0; i < 16; i++) {
    Serial.print(state->beats[i].position);
    Serial.print(" ");
  }
  Serial.println("");
}

