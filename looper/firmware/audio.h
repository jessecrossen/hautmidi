#ifndef LOOPER_AUDIO_H
#define LOOPER_AUDIO_H

#include <Audio.h>
#include <Wire.h>

typedef enum {
  InputSourceMic = 0,
  InputSourceLine,
  InputSourceUndefined
} InputSource;

class Input {
  public:
    Input() {
      AudioMemory(60);
      _source = InputSourceUndefined;
      _audioControl = new AudioControlSGTL5000;
      _i2s = new AudioInputI2S;
      _peakAnalyzer = new AudioAnalyzePeak;
      _peakPatch = new AudioConnection(*_i2s, 1, *_peakAnalyzer, 0);
      _audioControl->enable();
      _micLevel = 32;
      _lineLevel = 5;
      _peak = 0.0;
      _integratedPeak = nan("");
    }
    // get/set the source for audio data
    InputSource source();
    void setSource(InputSource s);
    // get/set mic and line levels
    int micLevel();
    void setMicLevel(int level);
    int lineLevel();
    void setLineLevel(int level);
    // update anything that needs to be checked periodically
    void update();
    // get the current peak level
    float peak();
  private:
    AudioControlSGTL5000 *_audioControl;
    AudioInputI2S *_i2s;
    AudioAnalyzePeak *_peakAnalyzer;
    AudioConnection *_peakPatch;    
    InputSource _source;
    int _micLevel;
    int _lineLevel;
    float _peak;
    float _integratedPeak;
};

#endif
