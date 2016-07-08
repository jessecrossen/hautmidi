#ifndef _HOODWIND_interface_h_
#define _HOODWIND_interface_h_

#include "mvc/view.h"
#include "mvc/controller.h"
#include "mvc/fonts.h"
#include "instrument.h"

class ScaleMenu : public View, public ButtonAction {
  public:
    ScaleMenu();
    virtual void onButton(Button *target);
    virtual void layout();
  private:
    ScaleModel *_scale;
    Button *_tonicButtons[PitchClassCount];
    Button *_typeButtons[ScaleTypeCount];
};

class OctaveMenu : public View, public ButtonAction {
  public:
    OctaveMenu();
    virtual void onButton(Button *target);
    virtual void layout();
  private:
    ScaleModel *_scale;
    Button *_octaveButtons[CenterOctaveCount];
};

class ScaleNavigation : public Navigation {
  public:
    ScaleNavigation();
  private:
    ScaleMenu *_scaleMenu;
    OctaveMenu *_octaveMenu;
};

class InputDisplay : public View, public ButtonAction {
  public:
    InputDisplay();
    virtual void onButton(Button *target);
    virtual void layout();
  private:
    InputModel *_input;
    VStack *_sliders;
    Button *_calibrateButton;
    Button *_highRegisterButton;
    Button *_lowRegisterButton;
    Slider *_breathSlider;
    Slider *_biteSlider;
    Slider *_keySliders[InputKeyCount];
};

class InputNavigation : public Navigation {
  public:
    InputNavigation();
  private:
    InputDisplay *_inputDisplay;
};

class MainMenu : public VStack, public ButtonAction {
  public:
    MainMenu(Controller *c);
    virtual void onButton(Button *target);
    Controller *controller;
  private:
    // menu buttons
    Button *_scaleButton;
    Button *_synthButton;
    Button *_inputButton;
    // target views
    ScaleNavigation *_scaleNavigation;
    InputNavigation *_inputNavigation;
};

class Interface : public Controller {
  public:
    Interface();
    
  private:
    // menus
    MainMenu *_mainMenu;
};

#endif
