#ifndef _HOODWIND_view_h_
#define _HOODWIND_view_h_

#include "screen.h"
#include "touch.h"
#include "style.h"
#include "geom.h"
#include "fonts.h"

#include "model.h"
class Model;

// VIEW ***********************************************************************

class View {
  public:
    View();
    // get/set the rect to draw the view on
    Rect rect();
    void setRect(Rect r);
    void setWidth(coord_t w);
    void setHeight(coord_t h);
    // get/set the color scheme of the view
    ColorScheme colorScheme();
    void setColorScheme(ColorScheme cs);
    // get/set the text scheme of the view
    TextScheme textScheme();
    void setTextScheme(TextScheme ts);
    // get/set whether the view is enabled for touches
    bool enabled();
    void setEnabled(bool e);
    // get/set whether the view is visible
    bool visible();
    void setVisible(bool v);
    
    // get/set the model the view represents, if any
    Model *model();
    void setModel(Model *m);
    
    // request a redraw of the rect
    void invalidate();
    // redraw the view and subviews to the given screen
    void render(Screen *s);
    // this draws the view itself
    virtual void draw(Screen *s);
    // this lays out child views if needed
    virtual void layout();
    
    // add/remove a child view
    virtual void addChild(View *child);
    virtual void removeChild(View *child);
    
    // a link to the parent view
    View *_parent;
    // a link to the next view in a list of child views
    View *_nextChild;
    // a link to the next view in a model's list of attached views
    View *_nextModelView;
    
    // get this view or the child view being touched
    View *getTouchedView(Point p);
    // respond to a touch
    virtual void touch(Point p);
    // respond to a movement while touching the view
    virtual void drag(Point p);
    // respond to a touch being released
    virtual void release(Point p);
  
  protected:
    // the screen rect to draw to
    Rect _r;
    // the color and text schemes to draw the view with
    ColorScheme _cs;
    TextScheme _ts;
    // whether to draw the view
    bool _visible;
    // whether to accept touches
    bool _enabled;
    // whether the rect needs a redraw
    bool _invalid;
    // the model the view is attached to
    Model *_model;
    // a linked list of views contained by this one
    View *_children;
};

// LAYOUT CONTAINERS **********************************************************

class HStack : public View {
  public:
    virtual void layout();
};

class VStack : public View {
  public:
    virtual void layout();
};

class ScreenStack : public View {
  public:
    ScreenStack();
    virtual void draw(Screen *s);
    virtual void layout();
    virtual void addChild(View *child);
    // navigate through screens
    bool canGoPrev();
    bool canGoNext();
    void goPrev();
    void goNext();
    View *currentChild();
    void setCurrentChild(View *newChild);
  private:
    View *_currentChild;
};

// ADD TEXT *******************************************************************

class ViewWithLabel : public View {
  public:
    ViewWithLabel() : View() {
      _label = NULL;
    }
    // get/set a text label for the view
    const char *label();
    void setLabel(const char *s);
    
    // draw the view
    virtual void draw(Screen *s);
    
  protected:
    // draw a scan line of the background
    virtual void _drawBackScan(Screen *s, coord_t y, coord_t w);
    // text to show on the view
    const char *_label;
};

// BUTTON *********************************************************************

typedef enum {
  ButtonTypeMomentary,
  ButtonTypeToggle
} ButtonType;

class Button;
class ButtonAction {
  public:
    virtual void onButton(Button *) { }
};

class Button : public ViewWithLabel {
  public:
    Button();
    Button(const char *s);
    Button(const char *s, ButtonType t);
    
    // get/set toggle state
    bool toggled();
    void setToggled(bool state);
    
    // the button's behavior
    ButtonType type;
    // the action to take when the button is touched
    ButtonAction *action;
    
    // touch response
    virtual void touch(Point p);
    virtual void release(Point p);
  
  protected:
    // whether the button is toggled
    bool _toggled;
    // whether the button has just been touched or is being touched
    bool _touched;
    // draw the background
    virtual void _drawBackScan(Screen *s, coord_t y, coord_t w);
};

// SLIDER ********************************************************************

class Slider;
class SliderAction {
  public:
    virtual void onSlider(Slider *) { }
};

// define a margin on slider input area so they can go to the edge of the 
//  screen and still receive drags all the way to the edges of the value range
#define SliderMargin 30

class Slider : public ViewWithLabel {
  public:
    Slider();
    Slider(const char *s);
    
    // get/set value (0.0 to 1.0)
    float value();
    void setValue(float value);
    
    // the action to take when the slider value changes
    SliderAction *action;
    
    // touch response
    virtual void touch(Point p);
    virtual void drag(Point p);
    
    virtual void draw(Screen *s);
  
  protected:
    // the value of the slider
    float _value;
    // the number of pixels of fill to map the value to
    coord_t _valueSize;
    // draw the background
    virtual void _drawBackScan(Screen *s, coord_t y, coord_t w);
};

#endif
