// pinout
#define MUX_A 10
#define MUX_B 11
#define MUX_C 12
#define MUX_COM1 14
#define MUX_COM2 15
#define MUX_COM3 16
#define SWITCH_A_UP 2
#define SWITCH_A_DN 3
#define SWITCH_B_UP 0
#define SWITCH_B_DN 1
// number of buttons
#define BUTTON_COUNT 20
// MIDI params
#define MIDI_CHANNEL 1
// uncomment to enable serial debugging
#define SERIAL_DEBUG 1

#include <EEPROM.h>

// milliseconds elapsed since the last integration
elapsedMillis t;
// accumulated button values
uint32_t v[BUTTON_COUNT];
// number of scans since the last integration
uint32_t scan_count = 0;
// calibration limits
typedef struct {
  uint16_t min;
  uint16_t max;
} calibration;
calibration calibrations[BUTTON_COUNT];
uint8_t calibrating = 0;
// tuning offset for midi note output
int32_t tune = 0;
// volume levels for notes
typedef struct {
  uint8_t number;
  uint8_t volume;
  uint8_t last_volume;
  uint8_t playing;
} note;
note notes[BUTTON_COUNT];

void log_event(const char *name, int n1, int n2) {
  #ifdef SERIAL_DEBUG
  Serial.print(name);
  Serial.print(" ");
  Serial.print(n1);
  Serial.print(" ");
  Serial.println(n2);
  #endif
}

void scan() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(MUX_C, (i >> 2) & 1);
    digitalWrite(MUX_B, (i >> 1) & 1);
    digitalWrite(MUX_A, i & 1);
    delayMicroseconds(1);
    v[i] += analogRead(MUX_COM1);
    v[i+8] += analogRead(MUX_COM2);
    if (i < BUTTON_COUNT - 16) v[i+16] += analogRead(MUX_COM3);
  }
  scan_count++;
}

void integrate() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    v[i] /= scan_count;
  }
  scan_count = 1;
}

void calibrate() {
  int i, x;
  // initialize calibration ranges
  if (calibrating == 0) {
    for (i = 0; i < BUTTON_COUNT; i++) {
      calibrations[i].min = calibrations[i].max = v[i];
    }
    calibrating = 1;
  }
  // update calibration range limits
  for (i = 0; i < BUTTON_COUNT; i++) {
    x = v[i];
    if (x < calibrations[i].min) calibrations[i].min = x;
    if (x > calibrations[i].max) calibrations[i].max = x;
  }
}

void save_calibration() {
  uint32_t i, j, addr = 0;
  uint8_t *cal;
  for (i = 0; i < BUTTON_COUNT; i++) {
    cal = (uint8_t *)&calibrations[i];
    for (j = 0; j < sizeof(calibration); j++) {
      EEPROM.write(addr++, *(cal + j));
    }
    log_event("svc", calibrations[i].min, calibrations[i].max);
  }
}

void load_calibration() {
  uint32_t i, j, addr = 0;
  uint8_t *cal;
  for (i = 0; i < BUTTON_COUNT; i++) {
    cal = (uint8_t *)&calibrations[i];
    for (j = 0; j < sizeof(calibration); j++) {
      *(cal + j) = EEPROM.read(addr++);
    }
    log_event("rdc", calibrations[i].min, calibrations[i].max);
  }
}

void set_volumes() {
  int i, x, range, dead_zone;
  // range volumes against the calibration
  for (i = 0; i < BUTTON_COUNT; i++) {
    x = v[i] - calibrations[i].min;
    range = calibrations[i].max - calibrations[i].min;
    dead_zone = range >> 4;
    if (x < dead_zone) x = 0;
    range -= dead_zone;
    x = (x * 127) / range;
    if (x > 127) x = 127;
    notes[i].last_volume = notes[i].volume;
    notes[i].volume = x;
  }
}

void silence_note(int i) {
  if (notes[i].playing) {
    usbMIDI.sendPolyPressure(notes[i].number, 0, MIDI_CHANNEL);
    log_event(" pp", notes[i].number, 0);
    usbMIDI.sendNoteOff(notes[i].number, 0, MIDI_CHANNEL);
    log_event("off", notes[i].number, 0);
    notes[i].playing = 0;
  }
}

void output_notes() {
  uint8_t number;
  for (int i = 0; i < BUTTON_COUNT; i++) {
    if (notes[i].volume > 0) {
      number = 60 + tune + i;
      if (number != notes[i].number) silence_note(i); 
      if (! notes[i].playing) {
        notes[i].number = number;
        usbMIDI.sendNoteOn(notes[i].number, notes[i].volume, MIDI_CHANNEL);
        log_event(" on", notes[i].number, notes[i].volume);
        notes[i].playing = 1;
      }
      if (notes[i].volume != notes[i].last_volume) {
        usbMIDI.sendPolyPressure(notes[i].number, notes[i].volume, MIDI_CHANNEL);
        log_event(" pp", notes[i].number, notes[i].volume);
      }
    }
    else {
      silence_note(i);
    }
  }
}

void output_ccs() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    if (notes[i].volume != notes[i].last_volume) {
      usbMIDI.sendControlChange(i + 1, notes[i].volume, MIDI_CHANNEL);
      log_event(" cc", i + 1, notes[i].volume);
    }
  }
}

void silence_notes() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    silence_note(i);
  }
}

void setup() {
  analogReadResolution(12);
  pinMode(MUX_COM1, INPUT);
  pinMode(MUX_COM2, INPUT);
  pinMode(MUX_COM3, INPUT);
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);
  pinMode(SWITCH_A_UP, INPUT_PULLUP);
  pinMode(SWITCH_A_DN, INPUT_PULLUP);
  pinMode(SWITCH_B_UP, INPUT_PULLUP);
  pinMode(SWITCH_B_DN, INPUT_PULLUP);
  delay(1000);
  load_calibration();
}

void loop() {
  while (t < 1) scan();
  t = 0;
  integrate();
  // select tuning offset
  if (! digitalRead(SWITCH_A_UP)) tune = 12;
  else if (! digitalRead(SWITCH_A_DN)) tune = -12;
  else tune = 0;
  // do actions by mode
  if (! digitalRead(SWITCH_B_DN)) {
    silence_notes();
    calibrate();
  }
  else {
    if (calibrating) {
      calibrating = 0;
      save_calibration();
    }
    set_volumes();
    if (! digitalRead(SWITCH_B_UP)) {
      silence_notes();
      output_ccs();
    }
    else {
      output_notes();
    }
  }
}
