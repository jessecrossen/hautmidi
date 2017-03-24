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

// a wiper location
typedef struct {
  int track;
  int position;
  bool accent;
} scan_beat_t;

// an interface state
typedef struct {
  int scanCount;
  bool stop, go, single, loop;
  bool storeA, storeB, selectA, selectB;
  byte track[4] = { 0, 0, 0, 0 };
  scan_beat_t beats[16] = { 
      { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 },
      { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 },
      { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 },
      { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }
    };  
} scan_t;

// initialize the scanner
void scan_init();
// scan state from the device
void scan(scan_t *state);
// integrate accumulated readings to reduce noise
void integrate_scan(scan_t *state);
// reset scan state for another run
void reset_scan(scan_t *state);
// log a scan state to the serial monitor for debugging
void print_digital(scan_t *state);
void print_accents(scan_t *state);
void print_tracks(scan_t *state);
void print_positions(scan_t *state);

#endif
