#include "controller.h"

#include "../debug.h"

// time in milliseconds after which to put the screen 
//  to sleep if there are no touches
#define SLEEP_TIMEOUT 30000

// CONTROLLER ****************************************************************

Controller::Controller() : View() {
  _root = NULL;
  _touchedView = NULL;
  _touch = new Touch;
  _screen = new Screen;
  _touchReleasedSinceWake = true;
}

void Controller::setRotation(uint8_t r) {
  if (r != _screen->rotation()) {
    _screen->setRotation(r);
    setRect({ .x = 0, .y = 0, .w = _screen->width(), .h = _screen->height() });
    invalidate();
  }
}

void Controller::begin() {
  _screen->begin();
  _touch->begin();
  // fill the screen
  setRotation(0);
  // start the update clock
  sinceLastUpdate = 0;
}

// change the root view in the controller
void Controller::showView(View *view) {
  if (view != _root) {
    if (_root) removeChild(_root);
    _root = view;
    if (_root) addChild(_root);
  }
}
void Controller::showNavigation(Navigation *nav) {
  nav->controller = this;
  nav->lastView = _root;
  showView(nav);
}

// all children should fill the screen
void Controller::layout() {
  View *child = _children;
  while (child != NULL) {
    child->setRect(_r);
    child = child->_nextChild;
  }
}

void Controller::draw(Screen *s) {
  s->fillRect(_r.x, _r.y, _r.w, _r.h, _cs.bg);
}

void Controller::update() {
  // see if enough time has ellapsed for a new frame
  if (sinceLastUpdate < FRAME_INTERVAL) return;
  sinceLastUpdate = sinceLastUpdate - FRAME_INTERVAL;
  // check for touches
  if (_touch->touched()) {
    sinceLastTouch = 0;
    _screen->setBrightness(120);
    if (_screen->isSleeping()) {
      _screen->setSleeping(false);
      _touchReleasedSinceWake = false;
    }
    // ignore the touch that wakes up the screen
    else if (_touchReleasedSinceWake) {
      _touchPoint = _touch->getScreenPoint(_screen);
      // update the touched view
      View *tv = getTouchedView(_touchPoint);
      if (_touchedView == NULL) _setTouchedView(tv);
      else if (_touchedView != NULL) _touchedView->drag(_touchPoint);
    }
  }
  else {
    _setTouchedView(NULL);
    _touchReleasedSinceWake = true;
  }
  // control display sleep
  if (sinceLastTouch > SLEEP_TIMEOUT) {
    uint8_t brightness = _screen->brightness();
    if (brightness > 0) {
      brightness = (brightness > 5) ? (brightness - 5) : 0;
      _screen->setBrightness(brightness);
    }
    else {
      _screen->setSleeping(true);
    }
  }
  else {
    // re-render views if we're not in sleep mode
    render(_screen);
  }
}

void Controller::_setTouchedView(View *tv) {
  if (tv != _touchedView) {
    if (_touchedView != NULL) _touchedView->release(_touchPoint);
    _touchedView = tv;
    if (_touchedView != NULL) _touchedView->touch(_touchPoint);
  } 
}

// NAVIGATION ****************************************************************

Navigation::Navigation() : View() {
  lastView = NULL;
  // make a top bar for the buttons
  _topBar = new HStack;
  _topBar->setHeight(60);
  addChild(_topBar);
  // go to the previous screen
  _prevButton = new Button("<", ButtonTypeMomentary);
  _prevButton->action = this;
  _topBar->addChild(_prevButton);
  // exit the navigation interface
  _exitButton = new Button("^", ButtonTypeMomentary);
  _exitButton->action = this;
  _topBar->addChild(_exitButton);
  // go to the next screen
  _nextButton = new Button(">", ButtonTypeMomentary);
  _nextButton->action = this;
  _topBar->addChild(_nextButton);
  // make screens to navigate through
  screens = new ScreenStack;
  addChild(screens);
}

void Navigation::layout() {
  int16_t bh = _topBar->rect().h;
  int16_t d = _r.w / 4;
  _prevButton->setWidth(d);
  _exitButton->setWidth(d * 2);
  _nextButton->setWidth(d);
  _topBar->setRect({ .x = _r.x, .y = _r.y, .w = _r.w, .h = bh });
  screens->setRect({ .x = _r.x, .y = (int16_t)(_r.y + bh), 
                     .w = _r.w, .h = (int16_t)(_r.h - bh) });
  // enable/disable buttons
  _prevButton->setEnabled(screens->canGoPrev());
  _nextButton->setEnabled(screens->canGoNext());
}

void Navigation::onButton(Button *target) {
  if (target == _prevButton) screens->goPrev();
  else if (target == _nextButton) screens->goNext();
  else if (target == _exitButton) {
    if ((lastView != NULL) && (controller != NULL)) {
      controller->showView(lastView);
    }
  }
}
