#include "shift.h"

int shift_state;

void shift_init() {
  pinMode(SHIFT_SER, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_OE1, OUTPUT);
  pinMode(SHIFT_OE2, OUTPUT);
}

void shift_enable(bool upper, bool lower) {
  digitalWrite(SHIFT_OE1, upper ? LOW : HIGH);
  digitalWrite(SHIFT_OE2, lower ? LOW : HIGH);
}

int shift_fill(bool bit) {
  for (int i = 0; i < 17; i++) shift_bit(bit);
  return(shift_state);
}

int shift_bit(bool bit) {
  digitalWrite(SHIFT_SER, bit);
  digitalWrite(SHIFT_CLK, 1);
  delayMicroseconds(1);
  digitalWrite(SHIFT_CLK, 0);
  // shift our internal copy of the shifter's state
  int last_state = shift_state;
  shift_state >>= 1;
  shift_state |= bit << 15;
  return(last_state);
}

int shift_set(int v, bool next_bit) {
  for (int i = 0; i < 16; i++) {
    shift_bit(v & 0x1);
    v >>= 1;
  }
  // push a dummy bit into the pipe to commit the value
  return(shift_bit(next_bit));
}
int shift_set(int v) {
  return(shift_set(v, 0));
}
