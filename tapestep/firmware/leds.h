#ifndef TAPESTEP_LEDS_H
#define TAPESTEP_LEDS_H

#include <elapsedMillis.h>

typedef struct {
  byte pin;
  byte bright;
} led_t;

#define LED_COUNT 5
#define TEMPO_LED 0
#define TRACK1_LED 1
#define TRACK2_LED 2
#define TRACK3_LED 3
#define TRACK4_LED 4

// light the LED with the given index
void light_led(int i);
void light_led(int i, byte bright);

// update brightness values for all LEDs
void update_leds();

// light all LEDs in turn from left to right
//  NOTE: this is a blocking call
void chase_leds();

#endif
