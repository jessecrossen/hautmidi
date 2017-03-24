#include "mux.h"

void mux_init() {
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);
  pinMode(MUX_COM1, INPUT);
  pinMode(MUX_COM2, INPUT);
  analogReadResolution(12);
}

void mux_read(int *v, int delay) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(MUX_C, (i >> 2) & 1);
    digitalWrite(MUX_B, (i >> 1) & 1);
    digitalWrite(MUX_A, i & 1);
    delayMicroseconds(delay);
    v[i] = analogRead(MUX_COM1);
    v[i+8] = analogRead(MUX_COM2);
  }
}
void mux_read(int *v) {
  mux_read(v, 1);
}
