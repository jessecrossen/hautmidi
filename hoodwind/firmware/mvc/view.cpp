#include "view.h"

#include "../debug.h"

// VIEW ***********************************************************************

// constructor
View::View() {
  _parent = _nextChild = _nextModelView = NULL;
  _children = NULL;
  _model = NULL;
  _visible = true;
  _enabled = false;
  _cs = {
      .bg = TFT_RGB(0, 0, 0),
      .fg = TFT_RGB(255, 255, 255),
      .active = TFT_RGB(64, 64, 64),
      .accent = TFT_RGB(255, 0, 0)
    };
  _ts = {
      .font = fontWithHeight(22),
      .xalign = 0.5,
      .yalign = 0.5
    };
  invalidate();
}

// rect access
Rect View::rect() { return(_r); }
void View::setRect(Rect r) {
  if ((_r.x != r.x) || (_r.y != r.y) || (_r.w != r.w) || (_r.h != r.h)) {
    _r = r;
    invalidate();
  }
}
void View::setWidth(coord_t w) {
  if (w != _r.w) {
    _r.w = w;
    invalidate();
  }
}
void View::setHeight(coord_t h) {
  if (h != _r.h) {
    _r.h = h;
    invalidate();
  }
}

// color scheme access
ColorScheme View::colorScheme() { return(_cs); }
void View::setColorScheme(ColorScheme cs) {
  if ((_cs.bg != cs.bg) || (_cs.fg != cs.fg) || 
      (_cs.active != cs.active) || (_cs.accent != cs.accent)) {
    _cs = cs;
    invalidate();
  }
}

// text scheme access
TextScheme View::textScheme() { return(_ts); }
void View::setTextScheme(TextScheme ts) {
  if ((_ts.font != ts.font) || 
      (_ts.xalign != ts.xalign) || (_ts.yalign != ts.yalign)) {
    _ts = ts;
    invalidate();
  }
}

// visible access
bool View::visible() { return(_visible); }
void View::setVisible(bool e) {
  if (e != _visible) {
    _visible = e;
    invalidate();
    if (_parent != NULL) _parent->invalidate();
  }
}

// enabled access
bool View::enabled() { return(_enabled); }
void View::setEnabled(bool e) {
  if (e != _enabled) {
    _enabled = e;
    invalidate();
  }
}

// model access
Model *View::model() { return(_model); }
void View::setModel(Model *m) {
  if (m != _model) { 
    if (_model != NULL) _model->removeView(this);
    _model = m;
    _model->addView(this);
  }
}

// redraw
void View::invalidate() {
  _invalid = true;
}
void View::render(Screen *s) {
  // skip invisible containers
  if (! _visible) return;
  // redraw the view itself if it's changed
  if (_invalid) {
    layout();
    draw(s);
  }
  // render subviews
  View *child = _children;
  while (child) {
    if (child->visible()) {
      // if this view has been redrawn, all of its child views 
      //  should also be redrawn, since we may have painted over them
      if (_invalid) child->invalidate();
      child->render(s);
    }
    child = child->_nextChild;
  }
  // when we're done redrawing, the rect is valid
  _invalid = false;
}
void View::draw(Screen *s) {
  // implement this in subclasses
}
void View::layout() {
  // implement this in subclasses
}

// child views
void View::addChild(View *child) {
  if (_children == NULL) {
    _children = child;
    _children->_nextChild = NULL;
  }
  else {
    View *node = _children;
    while (node->_nextChild != NULL) node = node->_nextChild;
    node->_nextChild = child;
    child->_nextChild = NULL;
  }
  child->_parent = this;
  invalidate();
}
void View::removeChild(View *child) {
  View *prev = NULL;
  View *node = _children;
  while (node != NULL) {
    if (node == child) {
      // handle the first child being removed
      if (prev == NULL) _children = node->_nextChild;
      // handle later children being removed
      else prev->_nextChild = node->_nextChild;
      // remove this view as the child's parent
      child->_parent = NULL;
    }
    prev = node;
    node = node->_nextChild;
  }
  invalidate();
}

// handle touches
View *View::getTouchedView(Point p) {
  // skip invisible views
  if (! _visible) return(NULL);
  // pass to child views if any intersect
  View *result;
  View *node = _children;
  while (node != NULL) {
    Rect r = node->rect();
    if ((p.x >= r.x) && (p.y >= r.y) &&
        (p.x <= r.x + r.w) && (p.y <= r.y + r.h)) {
      result = node->getTouchedView(p);
      if (result != NULL) return(result);
    }
    node = node->_nextChild;
  }
  // if no child nodes caught the touch assume this one did
  if (_enabled) return(this);
  return(NULL);
}

void View::touch(Point p) {
  // implement behaviors in the subclass
}
void View::drag(Point p) {
  // implement behaviors in the subclass
}
void View::release(Point p) {
  // implement behaviors in the subclass
}

// LAYOUT CONTAINERS **********************************************************

void HStack::layout() {
  coord_t w;
  coord_t x = _r.x;
  View *child = _children;
  while (child != NULL) {
    w = child->rect().w;
    child->setRect({ .x = x, .y = _r.y, .w = w, .h = _r.h});
    x += w;
    child = child->_nextChild;
  }
}

void VStack::layout() {
  coord_t h;
  coord_t y = _r.y;
  View *child = _children;
  while (child != NULL) {
    h = child->rect().h;
    child->setRect({ .x = _r.x, .y = y, .w = _r.w, .h = h});
    y += h;
    child = child->_nextChild;
  }
}

ScreenStack::ScreenStack() : View() {
  _currentChild = NULL;
}

void ScreenStack::addChild(View *child) {
  View::addChild(child);
  if (_currentChild == NULL) setCurrentChild(_children);
}

void ScreenStack::draw(Screen *s) {
  s->fillRect(_r, _cs.bg);
}

void ScreenStack::layout() {
  // make sure we have a current screen if there is one
  if (_currentChild == NULL) _currentChild = _children;
  View *child = _children;
  while (child != NULL) {
    child->setVisible(child == _currentChild);
    child->setRect(_r);
    child = child->_nextChild;
  }
}

bool ScreenStack::canGoPrev() {
  return((_currentChild != NULL) && (_currentChild != _children));
}
bool ScreenStack::canGoNext() {
  return((_currentChild != NULL) && (_currentChild->_nextChild != NULL));
}

void ScreenStack::goPrev() {
  View *child = _children;
  while (child != NULL) {
    if (child->_nextChild == _currentChild) {
      setCurrentChild(child);
      return;
    }
  }
  child = child->_nextChild;
}
void ScreenStack::goNext() {
  if (_currentChild != NULL) setCurrentChild(_currentChild->_nextChild);
}

View *ScreenStack::currentChild() { return(_currentChild); }
void ScreenStack::setCurrentChild(View *newChild) {
  if (newChild == NULL) return;
  View *child = _children;
  while (child != NULL) {
    if ((child == newChild) && (newChild != _currentChild)) {
      _currentChild = newChild;
      invalidate();
      if (_parent != NULL) _parent->invalidate();
      return;
    }
    child = child->_nextChild;
  }
}

// ADD LABEL ******************************************************************

const char *ViewWithLabel::label() { return(_label); }
void ViewWithLabel::setLabel(const char *s) {
  if (_label != s) {
    _label = s;
    invalidate();
  }
}

void ViewWithLabel::draw(Screen *s) {
  coord_t y;
  coord_t tb, tx, ty;
  tb = tx = ty = -1;
  if (_label != NULL) {
    // compute the text size
    coord_t tw = strlen(_label) * _ts.font->charWidth;
    coord_t th = _ts.font->charHeight;
    // position the text in the box according to alignment
    tx = (coord_t)((float)(_r.w - tw) * _ts.xalign);
    ty = (coord_t)((float)(_r.h - th) * _ts.yalign);
    tb = ty + th;
  }
  // fill the background
  for (y = 0; y < _r.h; y++) {
    _drawBackScan(s, y, _r.w);
    // draw text when we get to its box
    if ((y >= ty) && (y < tb)) {
      s->scanText(tx, y - ty, _label, _ts.font, _cs.fg);
    }
    s->commitScanBuffer(_r.x, _r.y + y, _r.w);
  }
}
void ViewWithLabel::_drawBackScan(Screen *s, coord_t y, coord_t w) {
  // implement in subclasses
  s->fillScanBuffer(0, w, _cs.bg);
}

// BUTTON VIEW ****************************************************************

Button::Button() : ViewWithLabel() {
  _toggled = _touched = false;
  _enabled = true;
  action = NULL;
}
Button::Button(const char *s) : Button() {
  setLabel(s);
}
Button::Button(const char *s, ButtonType t) : Button(s) {
  type = t;
}

bool Button::toggled() { return(_toggled); }
void Button::setToggled(bool state) {
  if (_toggled != state) {
    _toggled = state;
    invalidate();
  }
}

void Button::touch(Point p) {
  _touched = true;
  if (type == ButtonTypeToggle) {
    setToggled(! toggled());
  }
  invalidate();
  if (action != NULL) action->onButton(this);
}
void Button::release(Point p) {
  _touched = false;
  invalidate();
}

void Button::_drawBackScan(Screen *s, coord_t y, coord_t w) {
  // draw blank edges at top and bottom
  if ((y == 0) || (y == _r.h - 1)) {
    s->fillScanBuffer(0, w, _cs.bg);
    return;
  }
  // draw the button area
  color_t bg = _cs.bg;
  if (_enabled) bg = (_toggled || _touched) ? _cs.accent : _cs.active;
  s->fillScanBuffer(1, w - 2, bg);
  // draw blank edges at left and right
  s->scanBuffer[0] = _cs.bg;
  s->scanBuffer[w - 1] = _cs.bg;
}

// SLIDER VIEW ****************************************************************

Slider::Slider() : ViewWithLabel() {
  _value = 0.5;
  _enabled = true;
  action = NULL;
}
Slider::Slider(const char *s) : Slider() {
  setLabel(s);
}

float Slider::value() { return(_value); }
void Slider::setValue(float v) {
  if (! (v >= 0.0)) v = 0.0;
  if (! (v <= 1.0)) v = 1.0;
  if (_value != v) {
    _value = v;
    invalidate();
    if (action != NULL) action->onSlider(this);
  }
}

void Slider::touch(Point p) { drag(p); }
void Slider::drag(Point p) {
  // detect orientation
  if (_r.w > _r.h) {
    setValue((float)(p.x - (_r.x + SliderMargin)) / (float)(_r.w - (2 * SliderMargin)));
  }
  else {
    setValue(1.0 - ((float)(p.y - (_r.y + SliderMargin)) / (float)(_r.h - (2 * SliderMargin))));
  }
}

void Slider::draw(Screen *s) {
  // detect orientation and compute the size of the filled bar
  if (_r.w > _r.h) { // horizontal
    _valueSize = (coord_t)((float)(_r.w - 2) * _value);
  }
  else { // vertical
    _valueSize = (coord_t)((float)(_r.h - 2) * _value);
  }
  ViewWithLabel::draw(s);
}

void Slider::_drawBackScan(Screen *s, coord_t y, coord_t w) {
  // draw blank edges at top and bottom
  if ((y == 0) || (y == _r.h - 1)) {
    s->fillScanBuffer(0, w, _cs.bg);
    return;
  }
  // fill in the center part
  color_t bg = _enabled ? _cs.active : _cs.bg;
  if (_r.w > _r.h) { // horizontal
    s->fillScanBuffer(1, _valueSize, _cs.accent);
    s->fillScanBuffer(1 + _valueSize, (_r.w - 2) - _valueSize, bg);
  }
  else { // vertical
    s->fillScanBuffer(1, _r.w - 2, 
      ((_r.h - 1) - y <= _valueSize) ? _cs.accent : bg);
  }
  // draw blank edges at left and right
  s->scanBuffer[0] = _cs.bg;
  s->scanBuffer[w - 1] = _cs.bg;
}
