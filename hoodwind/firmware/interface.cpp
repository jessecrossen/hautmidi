#include "interface.h"
#include "debug.h"

Interface::Interface() : Controller() {
  // set up views
  _mainMenu = new MainMenu(this);
  showView(_mainMenu);
}

// MAIN MENU ******************************************************************

MainMenu::MainMenu(Controller *c) : VStack() {
  controller = c;
  // show an interface to select the scale
  _scaleButton = new Button("SCALE", ButtonTypeMomentary);
  _scaleButton->setHeight(60);
  _scaleButton->action = this;
  addChild(_scaleButton);
  // show an interface to select synth params
  _synthButton = new Button("SYNTH", ButtonTypeMomentary);
  _synthButton->setHeight(60);
  _synthButton->action = this;
  addChild(_synthButton);
  // show an interface to input the device
  _inputButton = new Button("INPUT", ButtonTypeMomentary);
  _inputButton->setHeight(60);
  _inputButton->action = this;
  addChild(_inputButton);
  // make target screens
  _scaleNavigation = new ScaleNavigation;
  _inputNavigation = new InputNavigation;
}
void MainMenu::onButton(Button *target) {
  if (target == _scaleButton)
    controller->showNavigation(_scaleNavigation);
  else if (target == _inputButton) 
    controller->showNavigation(_inputNavigation);
}

// SCALE **********************************************************************

ScaleMenu::ScaleMenu() : View() {
  int i;
  for (i = 0; i < PitchClassCount; i++) {
    _tonicButtons[i] = new Button(
        ScaleModel::nameOfPitchClass((PitchClass)i), ButtonTypeToggle);
    _tonicButtons[i]->action = this;
    addChild(_tonicButtons[i]);
  }
  for (i = 0; i < ScaleTypeCount; i++) {
    _typeButtons[i] = new Button(
        ScaleModel::shortNameOfScaleType((ScaleType)i), ButtonTypeToggle);
    _typeButtons[i]->action = this;
    addChild(_typeButtons[i]);
  }
  _scale = Instrument::instance()->scale();
  setModel(_scale);
}

void ScaleMenu::onButton(Button *target) {
  int i;
  for (i = 0; i < PitchClassCount; i++) {
    if (target == _tonicButtons[i]) {
      _scale->setTonic((PitchClass)i);
      return;
    }
  }
  for (i = 0; i < ScaleTypeCount; i++) {
    if (target == _typeButtons[i]) {
      _scale->setType((ScaleType)i);
      return;
    }
  }
}

void ScaleMenu::layout() {
  int i;
  PitchClass tonic = _scale->tonic();
  ScaleType type = _scale->type();
  int16_t bw = _r.w / 4;
  int16_t bh = bw;
  int16_t x = _r.x;
  int16_t y = _r.y;
  int16_t right = _r.x + _r.w;
  for (i = 0; i < PitchClassCount; i++) {
    _tonicButtons[i]->setToggled(i == tonic);
    _tonicButtons[i]->setRect({ .x = x, .y = y, .w = bw, .h = bh });
    x += bw;
    if (x + bw > right) {
      x = _r.x;
      y += bh;
    }
  }
  bw = _r.w / 4;
  bh = ((_r.y + _r.h) - y) / ((ScaleTypeCount + 2) / 4);
  for (i = 0; i < ScaleTypeCount; i++) {
    _typeButtons[i]->setToggled(i == type);
    _typeButtons[i]->setRect({ .x = x, .y = y, .w = bw, .h = bh });
    x += bw;
    if (x + bw > right) {
      x = _r.x;
      y += bh;
    }
  }
}

OctaveMenu::OctaveMenu() : View() {
  for (int i = 0; i < CenterOctaveCount; i++) {
    _octaveButtons[i] = new Button(
      ScaleModel::nameOfOctave(i + CenterOctaveMin), ButtonTypeToggle);
    _octaveButtons[i]->action = this;
    addChild(_octaveButtons[i]);
  }
  _scale = Instrument::instance()->scale();
  setModel(_scale);
}

void OctaveMenu::onButton(Button *target) {
  for (int i = 0; i < CenterOctaveCount; i++) {
    if (target == _octaveButtons[i]) {
      _scale->setCenterOctave(i + CenterOctaveMin);
      return;
    }
  }
}

void OctaveMenu::layout() {
  int i;
  uint8_t selected = _scale->centerOctave() - CenterOctaveMin;
  int16_t bw = _r.w / 2;
  int16_t bh = _r.h / 4;
  int16_t x = _r.x;
  int16_t y = _r.y;
  int16_t right = _r.x + _r.w;
  for (i = 0; i < CenterOctaveCount; i++) {
    _octaveButtons[i]->setToggled(i == selected);
    _octaveButtons[i]->setRect({ .x = x, .y = y, .w = bw, .h = bh });
    x += bw;
    if (x + bw > right) {
      x = _r.x;
      y += bh;
    }
  }
}

ScaleNavigation::ScaleNavigation() : Navigation() {
  _scaleMenu = new ScaleMenu;
  screens->addChild(_scaleMenu);
  _octaveMenu = new OctaveMenu;
  screens->addChild(_octaveMenu);
}

// SYNTH **********************************************************************



// INPUT **********************************************************************

InputDisplay::InputDisplay() : View() {
  int i;
  // bind to the input model
  _input = Instrument::instance()->input();
  setModel(_input);
  // make a stack to lay out the sliders
  _sliders = new VStack;
  addChild(_sliders);
  // use a smaller text scheme on sliders and status buttons
  int16_t sliderHeight = 20;
  TextScheme smallText = _ts;
  smallText.font = &LiberationMono_10_Bold;
  smallText.size = 10;
  TextScheme smallTextLeft = smallText;
  smallTextLeft.xalign = 0.05;
  // add the toggle buttons to display the register
  _lowRegisterButton = new Button("LOW", ButtonTypeToggle);
  _lowRegisterButton->setTextScheme(smallText);
  _lowRegisterButton->setEnabled(false);
  addChild(_lowRegisterButton);
  _highRegisterButton = new Button("HIGH", ButtonTypeToggle);
  _highRegisterButton->setTextScheme(smallText);
  _highRegisterButton->setEnabled(false);
  addChild(_highRegisterButton);
  // add the bite slider
  _biteSlider = new Slider("BITE");
  _biteSlider->setHeight(sliderHeight);
  _biteSlider->setTextScheme(smallTextLeft);
  _biteSlider->setEnabled(false);
  _sliders->addChild(_biteSlider);
  // add key sliders
  for (i = 0; i < InputKeyCount; i++) {
    _keySliders[i] = new Slider(InputModel::nameOfKey(i));
    _keySliders[i]->setHeight(sliderHeight);
    _keySliders[i]->setTextScheme(smallTextLeft);
    _keySliders[i]->setEnabled(false);
  }
  for (i = InputKeyCount - 1; i >= 0; i--) {
    _sliders->addChild(_keySliders[i]);
  }
  // add the calibration toggle
  _calibrateButton = new Button("CALIBRATE", ButtonTypeToggle);
  _calibrateButton->action = this;
  addChild(_calibrateButton);
}

void InputDisplay::onButton(Button *target) {
  if (target == _calibrateButton) {
    _input->setCalibrating(_calibrateButton->toggled());
  }
}

void InputDisplay::layout() {
  int i;
  // do layout
  int16_t x = _r.x;
  int16_t y = _r.y;
  int16_t w = _r.w / 2;
  int16_t h = _biteSlider->rect().h;
  int16_t sliderCount = InputKeyCount + 1;
  _lowRegisterButton->setRect({ .x = x, .y = y, .w = w, .h = h });
  x += w;
  _highRegisterButton->setRect({ .x = x, .y = y, .w = w, .h = h });
  y += h;
  h *= sliderCount;
  _sliders->setRect({ .x = _r.x, .y = y, .w = _r.w, .h = h });
  y += h;
  h = (_r.y + _r.h) - y;
  _calibrateButton->setRect({ .x = _r.x, .y = y, .w = _r.w, .h = h });
  // update from the model
  for (i = 0; i < InputKeyCount; i++) {
    _keySliders[i]->setValue(_input->key(i));
  }
  _biteSlider->setValue(_input->bite());
  _highRegisterButton->setToggled(_input->highRegister());
  _lowRegisterButton->setToggled(_input->lowRegister());
  _calibrateButton->setToggled(_input->calibrating());
}

InputNavigation::InputNavigation() : Navigation() {
  _inputDisplay = new InputDisplay;
  screens->addChild(_inputDisplay);
}
