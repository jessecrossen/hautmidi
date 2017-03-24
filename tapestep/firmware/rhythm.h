#ifndef TAPESTEP_RHYTHM_H
#define TAPESTEP_RHYTHM_H

#include "scan.h"

// tempo rotary encoder pinout
#define TEMPO_BUTTON 9
#define TEMPO_A 8
#define TEMPO_B 7

void rhythm_init();
void rhythm_update();
// update the rhythm from the given scan
void rhythm_set(scan_t *state);
void loop_set(bool v);
// start/stop playback
void rhythm_start();
void rhythm_stop();

#endif
