#ifndef TAPESTEP_MUX_H
#define TAPESTEP_MUX_H

#include <elapsedMillis.h>

#define MUX_A 14
#define MUX_B 15
#define MUX_C 16

#define MUX_COM1 18
#define MUX_COM2 17

// initialize mux pins
void mux_init();
// read all analog inputs, after the given delay in microseconds
void mux_read(int *v, int delay);
void mux_read(int *v);

#endif
