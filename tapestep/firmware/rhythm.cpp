#include <Encoder.h>
#include <Bounce.h>

#include "rhythm.h"

#include "leds.h"

// storage type for beats
typedef struct {
  unsigned int time;
  int track;
  int note;
  bool accent;
} beat_t;
// beats to play
beat_t beats[16];
int beatCount = 0;

// the time to wait before resetting the tempo tap interface
#define TEMPO_TAP_RESET 2000 // milliseconds
// the limits to place on tempo
#define MIN_TEMPO 30
#define MAX_TEMPO 380
// the extents of a raw beat position reading for calibration
#define MIN_POS 96
#define MAX_POS 4000
// the LED brightness for different beat strengths
#define BRIGHT_NORMAL 0x10
#define BRIGHT_ACCENT 0x80

// rotary encoder
Encoder tempoEncoder(TEMPO_A, TEMPO_B);
Bounce tempoButton(TEMPO_BUTTON, 10);

// current tempo in BPM
int tempo = 60;
// time of last tempo tap
elapsedMillis sinceLastTempoTap;
// a list of tapped tempo intervals
int taps[256];
int tapCount = 0;
// the current position in the measure (a 32-bit fraction)
unsigned int time = 0;
// the position at last update
unsigned int lastTime = 0;
// the amount to advance the time per microsecond
unsigned int timeAdvance = 0;
// an error accumulator for time advancement
unsigned int timeRemainder = 0;
unsigned int timeError = 0;
unsigned int lastError = 0;
// time since last conversion of current time
elapsedMicros timer = 0;
unsigned int lastTimer = 0;
// whether time is advancing
bool playing = 0;
// the number of measures played since the last reset
int measureCount = 0;
// whether to loop playback
bool loopPlayback = 0;

void set_tempo(int newTempo, bool setEncoder) {
  // store and clamp tempo
  tempo = newTempo;
  if (tempo < MIN_TEMPO) tempo = MIN_TEMPO;
  if (tempo > MAX_TEMPO) tempo = MAX_TEMPO;
  // compute the duration of a measure in microseconds
  float measureMicroseconds = (4. * 60000000.) / (float)tempo;
  // update the advance time and remainder
  float adv = (float)0x100000000 / measureMicroseconds;
  timeAdvance = (unsigned int)adv;
  adv = (float)0x100000000 * (adv - (float)timeAdvance);
  timeRemainder = (unsigned int)(adv + 0.5);
  // update the encoder if requested
  if (setEncoder) tempoEncoder.write(tempo * 4);
}

void resetTime() {
  lastTime = 0xFFFFFFFF;
  time = 0;
  lastError = timeError = 0;
  lastTimer = timer;
  measureCount = 0;
}

void tap_tempo() {
  // reset if it's been a while since the last tap
  if (sinceLastTempoTap >= TEMPO_TAP_RESET) {
    tapCount = 0;
  }
  // otherwise add to the list of taps
  else if (tapCount < 256) {
    taps[tapCount++] = sinceLastTempoTap;
    // average the tap time to get a tempo
    int tapSum = 0;
    for (int i = 0; i < tapCount; i++) {
      tapSum += taps[i];
    }
    set_tempo((60000 * tapCount) / tapSum, 1);
    // reset the clock to sync up with the tapped beat
    resetTime();
  }
  sinceLastTempoTap = 0;
}

void rhythm_init() {
  pinMode(TEMPO_BUTTON, INPUT_PULLUP);
  sinceLastTempoTap = TEMPO_TAP_RESET;
  set_tempo(60, 1);
}

void rhythm_update() {
  // tapping on the tempo rotary sets the tempo
  tempoButton.update();
  if (tempoButton.risingEdge()) tap_tempo();
  // turning the tempo knob adjusts the tempo
  int te = tempoEncoder.read() / 4;
  if (te < MIN_TEMPO) tempoEncoder.write(MIN_TEMPO * 4);
  else if (te > MAX_TEMPO) tempoEncoder.write(MAX_TEMPO * 4);
  else if (te != tempo) set_tempo(te, 0);
  // if we're not playing, there's nothing more to do
  if (! playing) return;
  // see how much time has elapsed since last update, handling rollover
  unsigned int elapsed = 0;
  unsigned int t = timer;
  if (t >= lastTimer) elapsed = t - lastTimer;
  else elapsed = (0xFFFFFFFF - lastTimer) + t;
  lastTimer = t;
  // advance the clock, tracking the remainder
  time += elapsed * timeAdvance;
  timeError += elapsed * timeRemainder;
  // when the error rolls over, transfer to the time
  if (timeError < lastError) time++;
  // light the tempo light on the beat and check looping
  bool wrapped = lastTime > time;
  if (wrapped) {
    if ((! loopPlayback) && (measureCount > 0)) {
      playing = 0;
      return;
    }
    else {
      measureCount++;
      light_led(TEMPO_LED, BRIGHT_ACCENT);
    }
  }
  else if (((lastTime < 0x40000000) && (time >= 0x40000000)) ||
           ((lastTime < 0x80000000) && (time >= 0x80000000)) ||
           ((lastTime < 0xC0000000) && (time >= 0xC0000000))) {
    light_led(TEMPO_LED, BRIGHT_NORMAL);
  }
  // see if any beats need to be played
  for (int i = 0; i < beatCount; i++) {
    t = beats[i].time;
    if (((lastTime < t) && (time >= t)) ||
        ((wrapped) && ((lastTime < t) || (time >= t)))) {
      light_led(TRACK1_LED + beats[i].track, 
                beats[i].accent ? BRIGHT_ACCENT : BRIGHT_NORMAL);
    }
  }
  // save the current time for next iteration
  lastTime = time;
  lastError = timeError;
}

int mapNote(int track, int offset) {
  return(32 + ((track * 12) + offset));
}
int mapTime(int position) {
  if (position <= MIN_POS) return(0);
  unsigned int t = (0x10000 * (position - MIN_POS)) / (MAX_POS - MIN_POS);
  if (t >= 0xFFFF) t = 0xFFFF;
  return(t | (t << 16));
}

void loop_set(bool v) {
  loopPlayback = v;
}

// transfer beats from a scan state
void rhythm_set(scan_t *state) {
  int i;
  scan_beat_t b;  
  beatCount = 0;
  for (i = 0; i < 16; i++) {
    b = state->beats[i];
    if (b.track >= 0) {
      beats[beatCount].time = mapTime(b.position);
      beats[beatCount].track = b.track;
      beats[beatCount].note = mapNote(b.track, state->track[b.track]);
      beats[beatCount].accent = b.accent;
      beatCount++;
    }
  }
}

void rhythm_start() {
  // reset when starting
  playing = 1;
  resetTime();
}

void rhythm_stop() {
  playing = 0;
}
