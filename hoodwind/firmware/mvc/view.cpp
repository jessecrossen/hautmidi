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
      .bg = ILI9341_BLACK,
      .fg = ILI9341_WHITE,
      .active = ILI9341_DARKGREY,
      .accent = ILI9341_RED
    };
  _ts = {
      .font = &LiberationMono_18_Bold,
      .size = 18,
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
  if ((_ts.font != ts.font) || (_ts.size != ts.size) || 
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

// BUTTON VIEW ****************************************************************

Button::Button() : View() {
  _label = NULL;
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

const char *Button::label() { return(_label); }
void Button::setLabel(const char *s) {
  if (_label != s) {
    _label = s;
    invalidate();
  }
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

void Button::draw(Screen *s) {
  // select a color for the button background
  color_t bg = _cs.bg;
  if (_enabled) bg = (_toggled || _touched) ? _cs.accent : _cs.active;
  // draw the background
  s->fillRect(insetRect(_r, 1), bg);
  // draw the label, if any
  if (_label != NULL) {
    s->setTextColor(_cs.fg, bg);
    s->drawTextInRect(_label, _r, _ts);
  }
}

// SLIDER VIEW ****************************************************************

Slider::Slider() : View() {
  _label = NULL;
  _value = 0.5;
  _enabled = true;
  action = NULL;
}
Slider::Slider(const char *s) : Slider() {
  setLabel(s);
}

const char *Slider::label() { return(_label); }
void Slider::setLabel(const char *s) {
  if (_label != s) {
    _label = s;
    invalidate();
  }
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
  // select a color for the slider background
  color_t bg = _enabled ? _cs.active : _cs.bg;
  Rect ra = insetRect(_r, 1);
  Rect rb = ra;
  coord_t v;
  // detect orientation
  if (_r.w > _r.h) {
    v = (coord_t)((float)(ra.w) * _value);
    rb = trimRect(ra, 0, 0, 0, v);
    ra = trimRect(ra, 0, rb.w, 0, 0);
    if (ra.w > 0) s->fillRect(ra, _cs.accent);
    if (rb.w > 0) s->fillRect(rb, bg);
  }
  else {
    v = (coord_t)((float)(ra.h) * _value);
    rb = trimRect(ra, v, 0, 0, 0);
    ra = trimRect(ra, 0, 0, rb.h, 0);
    if (ra.h > 0) s->fillRect(ra, _cs.accent);
    if (rb.h > 0) s->fillRect(rb, bg);
  }
  // draw the label, if any
  if (_label != NULL) {
    s->setTextColor(_cs.fg, bg);
    s->drawTextInRect(_label, _r, _ts);
  }
}
