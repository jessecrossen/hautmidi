// pin mapping
#define FREQ_PIN 22
#define AMP_PIN 23
// integrated input indices
#define FREQ_IDX 0
#define AMP_IDX 1
#define INPUT_COUNT 2
// ADC params
#define ADC_MIN 0
#define ADC_MAX 4095
#define AMP_COMP 300
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
#define MIDI_BREATH_CC 2
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
  int amp; // the current amplitude to send to the breatch controller (0-127)
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
  // when the time has elapsed, integrate collected data
  for (i = 0; i < INPUT_COUNT; i++) {
    integrateInput(inputs + i);
  }
}

// initialize state
void initState(MIDIState *state) {
  state->note = MIDI_MIN;
  state->velocity = MIDI_MIN;
  state->bend = BEND_CENTER;
  state->amp = MIDI_MIN;
}
// the state as processed from inputs
MIDIState inState;
// the last state sent via MIDI
MIDIState outState;

// the minimum and maximum values received for amplitude
int ampMin = 0;
int ampMax = 0;

// map an input range onto an output range by linear interpolation
int map(int in, int inMin, int inMax, int outMin, int outMax) {
  // clamp
  if (in > inMax) in = inMax;
  if (in < inMin) in = inMin;
  // interpolate
  return(outMin + (((in - inMin) * (outMax - outMin)) / (inMax - inMin)));
}

void setup() {
  // configure pins
  initInput(&inputs[FREQ_IDX], FREQ_PIN);
  initInput(&inputs[AMP_IDX], AMP_PIN);
  // use the full capability of the ADC
  analogReadResolution(12);
  // initialize state structs
  initState(&inState);
  initState(&outState);
  // calibrate the amplitude pot
  ampMin = analogRead(AMP_PIN);
  ampMax = ampMin + 512;
}

void loop() {
  int v;
  // read analog inputs
  readInputs(1);
  
  inState.note = ROOT_NOTE;
  
  v = inputs[AMP_IDX].integrated;
  if (v > ampMax) ampMax = v;
  if (v < ampMin - AMP_COMP) ampMin = v + AMP_COMP;
  inState.amp = map(v, ampMin, ampMax, MIDI_MIN, MIDI_MAX);
  inState.velocity = (inState.amp > MIDI_MIN + 2) ? MIDI_MAX : MIDI_MIN;
  // interpret strip position, ignoring if there is no pressure on the 
  //  strip, because the voltage will float in that situation
  if (inState.amp > 0) {
    inState.bend = map(inputs[FREQ_IDX].integrated, 
      ADC_MIN, ADC_MAX, BEND_MIN, BEND_MAX);
  }
  
  /*
  Serial.print(inState.velocity);
  Serial.print(" ");
  Serial.print(inState.amp);
  Serial.print(" ");
  Serial.print(inState.bend);
  Serial.println("");
  */
  
  #if MIDI_ENABLED
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
    // update changed amplitude
    if (inState.amp != outState.amp) {
      usbMIDI.sendControlChange(MIDI_BREATH_CC, inState.amp, MIDI_CHANNEL);
    }
  #endif
  // copy updated state so we don't update again
  outState = inState;
}
