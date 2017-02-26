#ifndef TAPESTEP_SCAN_H
#define TAPESTEP_SCAN_H

#include <elapsedMillis.h>

// switch pinout
#define SCAN_MOMENTARIES 19
#define SCAN_TOGGLE1 23
#define SCAN_TOGGLE2 21
#define SCAN_TRACK0 0
#define SCAN_TRACK1 1
#define SCAN_TRACK2 2
#define SCAN_TRACK3 5

// an interface state
typedef struct {
  bool stop, go, single, loop;
  byte track[4] = { 0, 0, 0, 0 };
  bool storeA, storeB, selectA, selectB;
} scan_t;

// initialize the scanner
void scan_init();
// scan and return the current state of the switches
scan_t scan();
// log a scan state to the serial monitor for debugging
void print_scan(scan_t state);

#endif
