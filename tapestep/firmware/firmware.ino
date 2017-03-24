#include <Encoder.h>
#include <Bounce.h>

#include "leds.h"
#include "shift.h"
#include "mux.h"
#include "scan.h"
#include "rhythm.h"

elapsedMillis t;
scan_t lastState; // the state at the last integration time
scan_t currentState; // the state being scanned from the board
scan_t stateA; // the state stored in the A slot
scan_t stateB; // the state stored in the B slot

void setup() {
  Serial.begin(9600);
  shift_init();
  mux_init();
  scan_init();
  rhythm_init();
  chase_leds();
}

void loop() {
  update_leds();
  scan(&currentState);
  rhythm_update();
  if (t > 100) {
    // integrate measurements for capture
    integrate_scan(&currentState);
    // store state if buttons are pressed
    if ((currentState.storeA) && (! lastState.storeA)) stateA = currentState;
    if ((currentState.storeB) && (! lastState.storeB)) stateB = currentState;
    // update the rhythm from the selected state
    if (currentState.selectA) rhythm_set(&stateA);
    else if (currentState.selectB) rhythm_set(&stateB);
    else rhythm_set(&currentState);
    loop_set(currentState.loop);
    // start/stop playback
    if ((currentState.stop) && (! lastState.stop)) rhythm_stop();
    if ((currentState.go) && (! lastState.go)) rhythm_start();
    // get ready to start collecting data again
    lastState = currentState;
    reset_scan(&currentState);
    t = 0;
  }
}
