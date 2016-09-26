#ifndef LOOPER_MODES_H
#define LOOPER_MODES_H

#include <LiquidCrystal.h>
#include <Encoder.h>
#include <Bounce.h>
#include <SD_t3.h>

#include "audio.h"
#include "track.h"
#include "sync.h"

#define TRACK_COUNT 4

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
  public:
    LoopSelectMode(Track **tracks, Sync *sync) : Mode() {
      _tracks = tracks;
      _sync = sync;
      _initTracks(0);
    }
  protected:
    virtual int clamp(int value);
    virtual void display(LiquidCrystal *lcd);
    virtual void onChange();
  private:
    void _initTracks(int loopIndex);
    Track **_tracks;
    Sync *_sync;
};

class SourceMode : public Mode {
  public:
    SourceMode(AudioDevice *audio) : Mode() {
      _audio = audio;
      _audio->setSource(InputSourceLine);
    };
    virtual int read();
    virtual int write(int value);
    virtual void update(LiquidCrystal *lcd);
  protected:
    virtual void display(LiquidCrystal *lcd);
    
    AudioDevice *_audio;
    elapsedMillis _sinceLastUpdate;
};

class LineGainMode : public Mode {
  public:
    LineGainMode(AudioDevice *audio) : Mode() {
      _audio = audio;
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

    AudioDevice *_audio;
    InputSource _oldSource;
    elapsedMillis _sinceLastUpdate;
};

class MicGainMode : public LineGainMode {
  public:
    MicGainMode(AudioDevice *audio) : LineGainMode(audio) { };
    virtual int read();
    virtual int write(int value);
  protected:
    virtual int getMax() { return(63); };
    virtual InputSource getSource() { return(InputSourceMic); }
    virtual const char *getLabel() { return("MIC LEVEL: "); };
};

class Interface {
  public:
    Interface(LiquidCrystal *lcd, 
              Encoder *rotary, Bounce *button, 
              Bounce **switches);
    void update();
    void save();
    void load();
    int setModeIndex(int index);
  private:
    void _createChars();
    void _loadScreen();
    void _failScreen(const char *message);
    LiquidCrystal *_lcd;
    Encoder *_rotary;
    Bounce *_button;
    Bounce **_switches;
    AudioDevice *_audio;
    Track **_tracks;
    Mode **_modes;
    Sync *_sync;
    LoopSelectMode *_mainScreen;
    int _modeCount;
    int _modeIndex;
    bool _needsSave;
    elapsedMillis _sinceLastChange;
};

#endif
