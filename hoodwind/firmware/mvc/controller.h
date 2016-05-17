#ifndef _HOODWIND_controller_h_
#define _HOODWIND_controller_h_

#include "view.h"

// UPDATE INTERVAL
#define FRAME_INTERVAL 50 // milliseconds

// CONTROLLER ****************************************************************

class Navigation;

class Controller : public View {
  public:
    Controller();
    
    // set screen rotation
    void setRotation(ScreenRotation r);
    
    // initialize resources
    void begin();
    // update input and redraw invalid regions
    void update();
    // redraw
    virtual void draw(Screen *s);
    virtual void layout();
    
    // show a view as the root view
    void showView(View *view);
    void showNavigation(Navigation *nav);
    
  private:
    Touch *_touch;
    Screen *_screen;
    
    // the current root view in the interface
    View *_root;
    
    void _setTouchedView(View *tv);
    View *_touchedView;
    Point _touchPoint;
    
    elapsedMillis sinceLastUpdate;
    elapsedMillis sinceLastTouch;
    bool _touchReleasedSinceWake;
};

// NAVIGATION ****************************************************************

class Navigation : public View, public ButtonAction {
  public:
    Navigation();
    
    virtual void layout();
    virtual void onButton(Button *target);
  
    // the controller that presented the navigation view
    Controller *controller;
    // the previous view that was shown in the controller
    View *lastView;
    // screens to be navigated through
    ScreenStack *screens;
  
  protected:
    // buttons to navigate through screens
    Button *_prevButton;
    Button *_nextButton;
    Button *_exitButton;
    HStack *_topBar;
};

#endif
