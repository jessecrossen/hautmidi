#ifndef TAPESTEP_SHIFT_H
#define TAPESTEP_SHIFT_H

#include <elapsedMillis.h>

#define SHIFT_SER 10
#define SHIFT_CLK 12

#define SHIFT_OE1 11
#define SHIFT_OE2 13

// initialize shifter pins and state
void shift_init();
// set the enabled state of the two registers
void shift_enable(bool upper, bool lower);
// fill the shifter with the given bit and return the new state as a 16-bit int
int shift_fill(bool bit);
// push the given bit in and return the new state as a 16-bit int
int shift_bit(bool bit);

#endif
