#ifndef LOOPER_MODES_H
#define LOOPER_MODES_H

#include <LiquidCrystal.h>
#include <Encoder.h>
#include <Bounce.h>

#include "audio.h"

class Mode {
  public:
    Mode() {
      _value = 0;
      _valid = false;
    };
    // generic methods
    virtual int read();
    virtual int write(int value);
    void invalidate();
    void setActive(bool active);
    bool isActive();
    virtual void update(LiquidCrystal *lcd);
  protected:
    virtual int clamp(int value);
    virtual void display(LiquidCrystal *lcd);
    virtual void onChange();
    virtual void onActivate();
    virtual void onDeactivate();
  protected:
    bool _valid;
  private:
    int _value;
    bool _active;
};

class LoopSelectMode : public Mode {
  protected:
    virtual int clamp(int value);
    virtual void display(LiquidCrystal *lcd);
    virtual void onChange();
};

class SourceMode : public Mode {
  public:
    SourceMode(Input *input) : Mode() {
      _input = input;
      _input->setSource(InputSourceLine);
    };
    virtual int read();
    virtual int write(int value);
    virtual void update(LiquidCrystal *lcd);
  protected:
    virtual void display(LiquidCrystal *lcd);
    
    Input *_input;
    elapsedMillis _sinceLastUpdate;
};

class LineGainMode : public Mode {
  public:
    LineGainMode(Input *input) : Mode() {
      _input = input;
    };
    virtual int read();
    virtual int write(int value);
    virtual void update(LiquidCrystal *lcd);
  protected:
    virtual void display(LiquidCrystal *lcd);
    virtual void onActivate();
    virtual void onDeactivate();
    virtual int getMax() { return(15); };
    virtual InputSource getSource() { return(InputSourceLine); }
    virtual const char *getLabel() { return("LINE LEVEL: "); };

    Input *_input;
    InputSource _oldSource;
    elapsedMillis _sinceLastUpdate;
};

class MicGainMode : public LineGainMode {
  public:
    MicGainMode(Input *input) : LineGainMode(input) { };
    virtual int read();
    virtual int write(int value);
  protected:
    virtual int getMax() { return(63); };
    virtual InputSource getSource() { return(InputSourceMic); }
    virtual const char *getLabel() { return("MIC LEVEL: "); };
};

class Interface {
  public:
    Interface(LiquidCrystal *lcd, Encoder *rotary, Bounce *button);
    void update();
    int setModeIndex(int index);
  private:
    void _createChars();
    void _loadScreen();
    LiquidCrystal *_lcd;
    Encoder *_rotary;
    Bounce *_button;
    Input *_input;
    Mode **_modes;
    int _modeCount;
    int _modeIndex;
};

#endif
