#include "audio.h"

InputSource AudioDevice::source() { return(_source); }
void AudioDevice::setSource(InputSource s) {
  // never allow a transition to an undefined state
  if (s == InputSourceUndefined) return;
  if (s != _source) {
    _source = s;
    if (_audioControl) {
      if (_source == InputSourceMic) {
         _audioControl->inputSelect(AUDIO_INPUT_MIC);
         _audioControl->micGain(_micLevel);
      }
      else if (_source == InputSourceLine) {
        _audioControl->inputSelect(AUDIO_INPUT_LINEIN);
        _audioControl->lineInLevel(_lineLevel);
      }
    }
  }
}

void AudioDevice::mix(AudioStream *stream, unsigned char channel) {
  for (int i = 0; i < 4; i++) {
    if (_mixerInputConnection[i] == NULL) {
      _mixerInputConnection[i] = 
        new AudioConnection(*stream, channel, *_mixer, i);
      return;
    }
  }
}

int AudioDevice::micLevel() { return(_micLevel); }
void AudioDevice::setMicLevel(int level) {
  if (level < 0) level = 0;
  if (level > 63) level = 63;
  if (level != _micLevel) {
    _micLevel = level;
    _audioControl->micGain(_micLevel);
  }
}

int AudioDevice::lineLevel() { return(_lineLevel); }
void AudioDevice::setLineLevel(int level) {
  if (level < 0) level = 0;
  if (level > 15) level = 15;
  if (level != _lineLevel) {
    _lineLevel = level;
    _audioControl->lineInLevel(_lineLevel);
  }
}

void AudioDevice::update() {
  if (_peakAnalyzer->available()) {
    if (isnan(_integratedPeak)) _integratedPeak = 0.0;
    float newPeak = _peakAnalyzer->read();
    if (newPeak > _integratedPeak) _integratedPeak = newPeak;
  }
}

float AudioDevice::peak() {
  update();
  if (! isnan(_integratedPeak)) {
    _peak = _integratedPeak;
    _integratedPeak = nan("");
  }
  return(_peak);
}
