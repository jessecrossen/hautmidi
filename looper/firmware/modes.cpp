#include "modes.h"

#include <string.h>
#include <EEPROM.h>

#include "audio.h"

// the maximum number of milliseconds of holding down a switch 
//  that qualifies as a tap as opposed to a hold
#define TAP_MILLISECONDS 500
// the maximum number of milliseconds between taps to count as a double tap
#define DOUBLE_TAP_MILLISECONDS 500
// the number of blocks to show the indicator that a loop has started
#define LOOP_START_BLOCKS (BLOCKS_PER_SECOND / 4)

// GENERIC ********************************************************************

// get/set the value
int Mode::read() {
  return(_value);
}
int Mode::write(int value) {
  value = clamp(value);
  if (value != read()) {
    _value = value;
    invalidate();
    onChange();
  }
  return(_value);
}
void Mode::invalidate() {
  _valid = false;
}
void Mode::setActive(bool active) {
  if (active != _active) {
    _active = active;
    if (_active) onActivate();
    else onDeactivate();
    invalidate();
  }
}
bool Mode::isActive() { return(_active); }
void Mode::update(LiquidCrystal *lcd) {
  if (! _valid) {
    display(lcd);
    _valid = true;
  }
}

// clamp the value to a range
int Mode::clamp(int value) { return(value); }
// update the LCD
void Mode::display(LiquidCrystal *lcd) {
  for (int y = 0; y < 2; y++) {
    lcd->setCursor(0, y);
    lcd->print("                ");
  }
}
// events
void Mode::onChange() { }
void Mode::onActivate() { }
void Mode::onDeactivate() { }

// LOOP SELECT ****************************************************************

void LoopSelectMode::_initTracks(int loopIndex) {
  int i;
  char path[64];
  // make sure the parent directory exists
  sprintf(path, "/%02d", loopIndex);
  SD.mkdir(path);
  // set paths for all tracks
  for (i = 0; i < TRACK_COUNT; i++) {
    sprintf(path, "/%02d/%d", loopIndex, i);
    _tracks[i]->setPath(path);
  }
  // set a path for the track sync points
  sprintf(path, "/%02d/sync", loopIndex);
  _sync->setPath(path);
  // set initial preroll for all tracks
  for (i = 0; i < TRACK_COUNT; i++) {
    _sync->setInitialPreroll(_tracks[i]);
  }
}

int LoopSelectMode::clamp(int value) {
  if (value < 0) value = 0;
  if (value > 99) value = 99;
  return(value); 
}

void LoopSelectMode::onChange() {
  // initialize tracks for the new loop index
  _initTracks(read());
}

void LoopSelectMode::display(LiquidCrystal *lcd) {
  int i;
  lcd->setCursor(0, 0);
  lcd->print("LOOP ");
  int v = read();
  if (v < 10) lcd->print("0");
  lcd->print(v);
  lcd->print("         ");
  // display track state
  lcd->setCursor(0, 1);
  Track *track;
  for (i = 0; i < TRACK_COUNT; i++) {
    track = _tracks[i];
    switch (track->state()) {
      case (Paused):
        if (track->masterBlocks() == 0) {
          lcd->print("-");
        }
        else {
          lcd->print("\x05");
        }
        break;
      case (Playing):
        // flash the symbol when a track is about to restart its loop
        if (track->playingBlock() + LOOP_START_BLOCKS > track->playBlocks()) 
          lcd->print("O");
        else lcd->print("\x04");
        break;
      case (MaybeRecording):
        lcd->print("*");
        break;
      case (Recording):
        lcd->print("\x03");
        break;
      default:
        lcd->print("?");
    }
    if (i + 1 < TRACK_COUNT) lcd->print("    ");
  }
}

// SOURCE *********************************************************************

int SourceMode::write(int value) {
  InputSource oldSource = _audio->source();
  if (value < 0) {
    value = 0;
    _audio->setSource(InputSourceLine);
  }
  if (value > 1) {
    value = 1;
    _audio->setSource(InputSourceMic);
  }
  InputSource newSource = _audio->source();
  if (newSource != oldSource) {
    invalidate();
    onChange();
  }
  return(newSource);
}
int SourceMode::read() {
  return(_audio->source() == InputSourceLine ? 0 : 1);
}

void SourceMode::update(LiquidCrystal *lcd) {
  _audio->update();
  if ((! _valid) || (_sinceLastUpdate >= 100)) {
    display(lcd);
    _valid = true;
    _sinceLastUpdate = 0;
  }
}

void SourceMode::display(LiquidCrystal *lcd) {
  lcd->setCursor(0, 0);
  if (read() == 0) lcd->print("SOURCE: LINE    ");
  else lcd->print("SOURCE: MIC    ");
  // show the recent peak level
  lcd->setCursor(0, 1);
  float px = _audio->peak() * 16.0;
  float delta;
  for (int x = 1; x <= 16; x++) {
    delta = (float)x - px;
    if (delta < 0.25) lcd->print("\x01");
    else if (delta <= 0.75) lcd->print("\x02");
    else lcd->print(" ");
  }
}

// GAIN ***********************************************************************

int LineGainMode::write(int value) {
  int oldLevel = read();
  _audio->setLineLevel(value);
  int newLevel = read();
  if (newLevel != oldLevel) {
    invalidate();
    onChange();
  }
  return(newLevel);
}
int LineGainMode::read() {
  return(_audio->lineLevel());
}

int MicGainMode::write(int value) {
  int oldLevel = read();
  _audio->setMicLevel(value);
  int newLevel = read();
  if (newLevel != oldLevel) {
    invalidate();
    onChange();
  }
  return(newLevel);
}
int MicGainMode::read() {
  return(_audio->micLevel());
}

void LineGainMode::display(LiquidCrystal *lcd) {
  lcd->setCursor(0, 0);
  const char *label = getLabel();
  lcd->print(label);
  int v = (int)round(((float)read() * 100.0) / (float)getMax());
  if (v < 10) lcd->print("  ");
  else if (v < 100) lcd->print(" ");
  lcd->print(v);
  lcd->print("%");
  for (int i = strlen(label) + 4; i < 16; i++) { lcd->print(" "); }
  // show the recent peak level
  lcd->setCursor(0, 1);
  float px = _audio->peak() * 16.0;
  float delta;
  for (int x = 1; x <= 16; x++) {
    delta = (float)x - px;
    if (delta < 0.25) lcd->print("\x01");
    else if (delta <= 0.75) lcd->print("\x02");
    else lcd->print(" ");
  }
}

void LineGainMode::onActivate() {
  _oldSource = _audio->source();
  _audio->setSource(getSource());
}

void LineGainMode::onDeactivate() {
  _audio->setSource(_oldSource);
}

void LineGainMode::update(LiquidCrystal *lcd) {
  _audio->update();
  if ((! _valid) || (_sinceLastUpdate >= 100)) {
    display(lcd);
    _valid = true;
    _sinceLastUpdate = 0;
  }
}

// OUTPUT VOLUME **************************************************************

int VolumeMode::write(int value) {
  int oldLevel = read();
  _audio->setOutputLevel(value);
  int newLevel = read();
  if (newLevel != oldLevel) {
    invalidate();
    onChange();
  }
  return(newLevel);
}
int VolumeMode::read() {
  return(_audio->outputLevel());
}

void VolumeMode::display(LiquidCrystal *lcd) {
  lcd->setCursor(0, 0);
  const char *label = "VOLUME:";
  lcd->print(label);
  int v = (int)round(((float)read() * 100.0) / (float)getMax());
  if (v < 10) lcd->print("  ");
  else if (v < 100) lcd->print(" ");
  lcd->print(v);
  lcd->print("%");
  for (int i = strlen(label) + 4; i < 16; i++) { lcd->print(" "); }
  // show the recent peak level
  lcd->setCursor(0, 1);
  lcd->print("                ");
}

// INTERFACE ******************************************************************

Interface::Interface(LiquidCrystal *lcd, Encoder *rotary, Bounce *button, 
                      Bounce **switches) {
  int i;
  _modeCount = 0;
  // set up the screen
  _needsSave = false;
  _lcd = lcd;
  _lcd->begin(16, 2);
  _createChars();
  _loadScreen();
  // bind variables
  _rotary = rotary;
  _button = button;
  _switches = switches;
  _audio = new AudioDevice();
  _tracks = new Track*[TRACK_COUNT];
  _sync = new Sync(_tracks, TRACK_COUNT);
  for (i = 0; i < TRACK_COUNT; i++) {
    _tracks[i] = new Track(_audio, _sync);
    _tracks[i]->index = i;
  }
  // start the SD card
  if (! SD.begin(10)) {
    _failScreen("SD CARD INIT");
    return;
  }
  // set up the interface
  _modeCount = 5;
  _modes = new Mode*[_modeCount];
  _mainScreen = new LoopSelectMode(_tracks, _sync);
  _modes[0] = _mainScreen;
  _modes[1] = new SourceMode(_audio);
  _modes[2] = new LineGainMode(_audio);
  _modes[3] = new MicGainMode(_audio);
  _modes[4] = new VolumeMode(_audio);
  _modeIndex = 0;
  _modes[_modeIndex]->setActive(true);
  // update all switches without responding so a transition from the initial 
  //  state doesn't look like a tap
  for (i = 0; i < TRACK_COUNT; i++) {
    _switches[i]->update();
  }
}

void Interface::load() {
  size_t i, j;
  // if the header doesn't match the number of modes, 
  //  treat stored data as invalid
  int b = 0;
  if (EEPROM.read(b++) != _modeCount) return;
  // read the value of each mode
  byte buffer[sizeof(int)];
  int *v = (int *)buffer;
  for (i = 0; i < (size_t)_modeCount; i++) {
    for (j = 0; j < sizeof(int); j++) {
      buffer[j] = EEPROM.read(b++);
    }
    _modes[i]->write(*v);
    if (i == (size_t)_modeIndex) _rotary->write(_modes[i]->read() * 4);
  }
}

void Interface::save() {
  size_t i, j;
  // write a header bytes showing how many modes are stored
  int b = 0;
  EEPROM.write(b++, _modeCount);
  // store the value of each mode
  byte buffer[sizeof(int)];
  int *v = (int *)buffer;
  for (i = 0; i < (size_t)_modeCount; i++) {
    *v = _modes[i]->read();
    for (j = 0; j < sizeof(int); j++) {
      EEPROM.write(b++, buffer[j]);
    }
  }
  _needsSave = false;
}

void Interface::update() {
  int i;
  static size_t lastBlock[TRACK_COUNT] = { 0, 0, 0, 0 };
  Track *track;
  // switch modes when the button is pressed
  if ((_button->update()) && (_button->fallingEdge())) {
    setModeIndex(_modeIndex + 1);
  }
  // update the mode's value when the rotary encoder turns
  int rv = (int)floor(_rotary->read() / 4);
  int mv = _modes[_modeIndex]->read();
  if (rv != mv) {
    int wv = _modes[_modeIndex]->write(rv);
    if (wv != rv) _rotary->write(wv * 4);
    if (wv != mv) {
      _needsSave = true;
      _sinceLastChange = 0;
    }
  }
  // update caches for all tracks
  bool trackHasEmptyCache = false;
  for (i = 0; i < TRACK_COUNT; i++) {
    track = _tracks[i];
    track->updateCaches();
    if (track->isPlaybackCacheEmpty()) trackHasEmptyCache = true;
  }
  // count the number of tracks recording so we never have more than one
  int tracksRecording = 0;
  for (i = 0; i < TRACK_COUNT; i++) {
    track = _tracks[i];
    if (track->isRecording()) tracksRecording++;
  }
  for (i = 0; i < TRACK_COUNT; i++) {
    track = _tracks[i];
    if (track == NULL) continue;
    TrackState oldState = track->state();
    TrackState newState = oldState;
    if (_switches[i]->update()) {
      // when the switch is depressed, we're either beginning 
      //  recording on the track (if it's held down) or toggling playback,
      //  but we won't know for a bit which it's going be
      if ((_switches[i]->fallingEdge())) {
        //  ...however, if another track is recording already, we don't start 
        //  recording because only one track is allowed to record at a time
        if (tracksRecording > 0) {
          // reset the state timer even though state is not actually changing,
          //  because we want to treat this as a normal tap
          track->sinceStateChange = 0;
        }
        else {
          newState = MaybeRecording;
          tracksRecording++;
        }
      }
      // when the switch is released, we're either toggling playback or 
      //  stopping recording depending on how much time has passed,
      //  or erasing the track if we're close enough to the last tap time
      else if (_switches[i]->risingEdge()) {
        // a short press means toggle the playback state
        if (track->sinceStateChange <= TAP_MILLISECONDS) {
          // check for a double-tap to erase
          if (track->sinceLastTap <= DOUBLE_TAP_MILLISECONDS) {
            track->erase();
            _mainScreen->invalidate();
            newState = Paused;
          }
          // handle a single tap
          else {
            bool setTrackActive = ! track->isActive();
            // if the track is empty, don't allow it to play
            if (track->masterBlocks() == 0) setTrackActive = false;
            track->setIsActive(setTrackActive);
            newState = track->isActive() ? Playing : Paused;
          }
          track->sinceLastTap = 0;
        }
        // a long press means stop recording and enter playback mode
        else {
          track->setIsActive(true);
          newState = Playing;
        }
      }
    }
    else if ((oldState == MaybeRecording) && 
             (track->sinceStateChange > TAP_MILLISECONDS)) {
      newState = Recording;
    }
    track->setState(newState);
    if (newState != oldState) _mainScreen->invalidate();
    // see if we've crossed any block boundaries that require display changes
    size_t block = track->playingBlock();
    size_t endBoundary = track->playBlocks() - LOOP_START_BLOCKS;
    if (lastBlock[i] > block) _mainScreen->invalidate();
    else if ((lastBlock[i] <= endBoundary) && (block > endBoundary)) 
      _mainScreen->invalidate();
    lastBlock[i] = block;
  }
  // if no tracks are recording, use track 0 as a passthru device
  _tracks[0]->setIsPassthru(tracksRecording == 0);
  // do operations that take time only after caches have blocks in them
  if (! trackHasEmptyCache) {
    _modes[_modeIndex]->update(_lcd);
    // see if we need to save settings
    if ((_needsSave) && (_sinceLastChange >= 2000)) {
      save();
    }
  }
}

int Interface::setModeIndex(int index) {
  while (index >= _modeCount) index -= _modeCount;
  while (index < 0) index += _modeCount;
  if (index != _modeIndex) {
    _modes[_modeIndex]->setActive(false);
    _modeIndex = index;
    _rotary->write(_modes[_modeIndex]->read() * 4);
    _modes[_modeIndex]->setActive(true);
  }
  return(_modeIndex);
}

void Interface::_loadScreen() {
  _lcd->setCursor(0, 0);
  _lcd->print("       \xCE\xCC\xDF      ");
  _lcd->setCursor(0, 1);
  _lcd->print("       \x06\x06       ");
}

void Interface::_failScreen(const char *message) {
  _lcd->setCursor(0, 0);
  _lcd->print("     FAILED     ");
  _lcd->setCursor(0, 1);
  _lcd->print("                ");
  int pad = (16 - strlen(message)) / 2;
  _lcd->setCursor(pad, 1);
  _lcd->print(message);
}

void Interface::_createChars() {
  byte fullBar[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
  };
  byte halfBar[8] = {
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };
  byte record[8] = {
    B00000,
    B00000,
    B01110,
    B11111,
    B11111,
    B11111,
    B01110,
    B00000
  };
  byte play[8] = {
    B00000,
    B01000,
    B01100,
    B01110,
    B01111,
    B01110,
    B01100,
    B01000
  };
  byte pause[8] = {
    B11011,
    B11011,
    B11011,
    B11011,
    B11011,
    B11011,
    B11011,
    B11011
  };
  byte note[8] = {
    B00000,
    B00100,
    B00110,
    B00101,
    B00101,
    B00100,
    B11100,
    B11100
  };
  _lcd->createChar(1, fullBar);
  _lcd->createChar(2, halfBar);
  _lcd->createChar(3, record);
  _lcd->createChar(4, play);
  _lcd->createChar(5, pause);
  _lcd->createChar(6, note);
}
