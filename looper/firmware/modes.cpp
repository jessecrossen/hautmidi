#include "modes.h"

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

int LoopSelectMode::clamp(int value) {
  if (value < 0) value = 0;
  if (value > 99) value = 99;
  return(value); 
}

void LoopSelectMode::onChange() {
  // TODO
}

void LoopSelectMode::display(LiquidCrystal *lcd) {
  lcd->setCursor(0, 0);
  lcd->print("LOOP ");
  int v = read();
  if (v < 10) lcd->print("0");
  lcd->print(v);
  lcd->print("         ");
  // TODO
  lcd->setCursor(0, 1);
  lcd->print("                ");
}

// SOURCE *********************************************************************

int SourceMode::write(int value) {
  InputSource oldSource = _input->source();
  if (value < 0) {
    value = 0;
    _input->setSource(InputSourceLine);
  }
  if (value > 1) {
    value = 1;
    _input->setSource(InputSourceMic);
  }
  InputSource newSource = _input->source();
  if (newSource != oldSource) {
    invalidate();
    onChange();
  }
  return(newSource);
}
int SourceMode::read() {
  return(_input->source() == InputSourceLine ? 0 : 1);
}

void SourceMode::update(LiquidCrystal *lcd) {
  _input->update();
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
  float px = _input->peak() * 16.0;
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
  _input->setLineLevel(value);
  int newLevel = read();
  if (newLevel != oldLevel) {
    invalidate();
    onChange();
  }
  return(newLevel);
}
int LineGainMode::read() {
  return(_input->lineLevel());
}

int MicGainMode::write(int value) {
  int oldLevel = read();
  _input->setMicLevel(value);
  int newLevel = read();
  if (newLevel != oldLevel) {
    invalidate();
    onChange();
  }
  return(newLevel);
}
int MicGainMode::read() {
  return(_input->micLevel());
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
  float px = _input->peak() * 16.0;
  float delta;
  for (int x = 1; x <= 16; x++) {
    delta = (float)x - px;
    if (delta < 0.25) lcd->print("\x01");
    else if (delta <= 0.75) lcd->print("\x02");
    else lcd->print(" ");
  }
}

void LineGainMode::onActivate() {
  _oldSource = _input->source();
  _input->setSource(getSource());
}

void LineGainMode::onDeactivate() {
  _input->setSource(_oldSource);
}

void LineGainMode::update(LiquidCrystal *lcd) {
  _input->update();
  if ((! _valid) || (_sinceLastUpdate >= 100)) {
    display(lcd);
    _valid = true;
    _sinceLastUpdate = 0;
  }
}

// INTERFACE ******************************************************************

Interface::Interface(LiquidCrystal *lcd, Encoder *rotary, Bounce *button) {
  _lcd = lcd;
  _lcd->begin(16, 2);
  _createChars();
  _loadScreen();
  _rotary = rotary;
  _button = button;
  _input = new Input();
  _modeCount = 4;
  _modes = new Mode*[_modeCount];
  _modes[0] = new LoopSelectMode();
  _modes[1] = new SourceMode(_input);
  _modes[2] = new LineGainMode(_input);
  _modes[3] = new MicGainMode(_input);
  _modeIndex = 0;
  _modes[_modeIndex]->setActive(true);
}

void Interface::update() {
  // switch modes when the button is pressed
  if ((_button->update()) && (_button->fallingEdge())) {
    setModeIndex(_modeIndex + 1);
  }
  // update the mode's value when the rotary encoder turns
  int v = (int)floor(_rotary->read() / 4);
  if (v != _modes[_modeIndex]->read()) {
    int cv = _modes[_modeIndex]->write(v);
    if (cv != v) _rotary->write(cv * 4);
  }
  _modes[_modeIndex]->update(_lcd);
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
  delay(500);
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
