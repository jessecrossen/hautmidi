// pin mapping
#define LED_PIN 13
#define BEND_PIN 23
#define MOD_PIN 22
#define VELO_PIN 21
#define VALVE_COUNT 4
#define VALVE_PIN(x) (15 + x)
// integrated input indices
#define BEND_IDX 0
#define MOD_IDX 1
#define VELO_IDX 2
#define INPUT_COUNT 3
// input ranges
#define VELO_FACTOR 281
#define VELO_MIN 10
#define VELO_MAX 4096
#define MOD_FACTOR 461
#define MOD_MIN 4
#define MOD_MAX 4096
// ADC params
#define ADC_MIN 0
#define ADC_MAX 4095
// whether to enable MIDI output
#define MIDI_ENABLED 1
// MIDI params
#define BEND_MIN 0
#define BEND_CENTER 8192
#define BEND_MAX 16383
#define MIDI_MIN 0
#define MIDI_MAX 127
#define MIDI_CHANNEL 1
#define MIDI_MOD_CC 1
#define ROOT_NOTE 48

// make a struct to store values from an analog input
typedef struct {
  int pin; // the pin to read from
  unsigned int sum; // the sum of samples read
  unsigned int samples; // the number of samples read
  int integrated; // the integrated input value
} AnalogInput;

// make a struct to store current and future instrument state
typedef struct {
  int note; // the current base note number (0-127)
  int velocity; // the current velocity (0-127)
  int bend; // the current pitch bend (BEND_CENTER-BEND_MAX)
  int mod; // the current value for the CV 1 modulation controller (0-127)
} MIDIState;

// initialize an analog input structure
void initInput(AnalogInput *input, int pin) {
  pinMode(pin, INPUT);
  input->pin = pin;
  input->sum = 0;
  input->samples = 0;
  input->integrated = 0;
}
// aggregate data into an analog input
void sampleInput(AnalogInput *in) {
  // protect from overflow
  if (in->samples < (2 << 20)) {
    in->sum += analogRead(in->pin);
    in->samples++;
  }
}
// integrate and reset an analog input
void integrateInput(AnalogInput *in) {
  in->integrated = in->sum / in->samples;
  in->sum = 0;
  in->samples = 0;
}
// analog inputs
AnalogInput inputs[INPUT_COUNT];
// integrate all inputs for the given number of milliseconds
void readInputs(unsigned int milliseconds) {
  int i;
  // collect data for the given amount of time
  elapsedMillis elapsed;
  while (elapsed < milliseconds) {
    for (i = 0; i < INPUT_COUNT; i++) {
      sampleInput(inputs + i);
    }
  }
  // when the time has elapse, integrate collected data
  for (i = 0; i < INPUT_COUNT; i++) {
    integrateInput(inputs + i);
  }
}

// initialize state
void initState(MIDIState *state) {
  state->note = MIDI_MIN;
  state->velocity = MIDI_MIN;
  state->bend = BEND_CENTER;
  state->mod = MIDI_MIN;
}
// the state as processed from inputs
MIDIState inState;
// the last state sent via MIDI
MIDIState outState;

// map an input range onto an output range by linear interpolation
int map(int in, int inMin, int inMax, int outMin, int outMax) {
  // clamp
  if (in > inMax) in = inMax;
  if (in < inMin) in = inMin;
  // interpolate
  return(outMin + (((in - inMin) * (outMax - outMin)) / (inMax - inMin)));
}

unsigned int linearize(unsigned int v, unsigned int c) {
  return((v * c) / (ADC_MAX - v));
}

// the threshold for touch events
int touchMin = 0;

void setup() {
  int i, t;
  // configure pins
  pinMode(LED_PIN, OUTPUT);
  initInput(&inputs[BEND_IDX], BEND_PIN);
  initInput(&inputs[MOD_IDX], MOD_PIN);
  initInput(&inputs[VELO_IDX], VELO_PIN);
  for (i = 0; i < VALVE_COUNT; i++) {
    pinMode(VALVE_PIN(i), INPUT);
  }
  // use the full capability of the ADC
  analogReadResolution(12);
  // initialize state structs
  initState(&inState);
  initState(&outState);
  // calibrate touch inputs
  for (i = 0; i < VALVE_COUNT; i++) {
    t = touchRead(VALVE_PIN(i));
    if (t > touchMin) touchMin = t;
  }
  // the capacitance of a touch input must at least 
  //  double to count as a touch
  touchMin *= 2;
}

void loop() {
  unsigned int v;
  int detune;
  // read analog inputs
  readInputs(1);
  // interpret strip pressure
  v = linearize(inputs[MOD_IDX].integrated, MOD_FACTOR);
  inState.mod = map(v, MOD_MIN, MOD_MAX, MIDI_MIN, MIDI_MAX);
  // interpret strip position, ignoring if there is no pressure on the 
  //  strip, because the voltage will float in that situation
  if (inState.mod > 0) {
    inState.bend = map(inputs[BEND_IDX].integrated, 
      ADC_MIN, ADC_MAX, BEND_CENTER, BEND_MAX);
  }
  // interpret the velocity sensor
  v = linearize(inputs[VELO_IDX].integrated, VELO_FACTOR);
  inState.velocity = map(v, VELO_MIN, VELO_MAX, MIDI_MIN, MIDI_MAX);
  // read valves
  detune = 0;
  if (touchRead(VALVE_PIN(0)) >= touchMin) detune += 1;
  if (touchRead(VALVE_PIN(1)) >= touchMin) detune += 2;
  if (touchRead(VALVE_PIN(2)) >= touchMin) detune += 3;
  if (touchRead(VALVE_PIN(3)) >= touchMin) detune += 5;
  inState.note = ROOT_NOTE - detune;
  #ifdef MIDI_ENABLED
    // update changed note
    if ((inState.note != outState.note) && (inState.velocity > 0)) {
      usbMIDI.sendNoteOff(outState.note, 0, MIDI_CHANNEL);
      usbMIDI.sendNoteOn(inState.note, inState.velocity, MIDI_CHANNEL);
    }
    // update changed velocity
    if (inState.velocity != outState.velocity) {
      // starting notes
      if ((inState.velocity > 0) && (outState.velocity == 0)) {
        usbMIDI.sendNoteOn(inState.note, inState.velocity, MIDI_CHANNEL);
      }
      // ending notes
      else if ((inState.velocity == 0) && (outState.velocity > 0)) {
        usbMIDI.sendNoteOff(inState.note, 0, MIDI_CHANNEL);
      }
      // continuing notes
      else if (inState.velocity > 0) {
        usbMIDI.sendPolyPressure(inState.note, inState.velocity, MIDI_CHANNEL);
      }
    }
    // update changed pitch bend
    if (inState.bend != outState.bend) {
      usbMIDI.sendPitchBend(inState.bend, MIDI_CHANNEL);
    }
    // update changed modulation
    if (inState.mod != outState.mod) {
      usbMIDI.sendControlChange(MIDI_MOD_CC, inState.mod, MIDI_CHANNEL);
    }
  #endif
  // copy updated state so we don't update again
  outState = inState;
}
