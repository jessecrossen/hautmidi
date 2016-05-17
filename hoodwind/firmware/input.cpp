#include "input.h"

// an index into the array of calibrations
typedef enum {
  R1 = 0,
  R2,
  R3,
  R4,
  L1,
  L2,
  L3,
  L4,
  Bite,
  HighRegister,
  LowRegister
} CalibrationIndex;

InputModel::InputModel() : Model() {
  int i;
  _calibrating = false;
  _highRegister = false;
  _lowRegister = false;
  _bite = 0.0;
  for (i = 0; i < InputKeyCount; i++) {
    _keys[i] = 0.0;
  }
  // reset the calibrations
  for (i = 0; i < InputCount; i++) {
    _calibrations[i].pin = 255;
    _calibrations[i].isTouch = false;
    _calibrations[i].min = _calibrations[i].first = _calibrations[i].max = 0;
    _calibrations[i].raw = 0;
    _calibrations[i].value = 0.0;
  }
  // configure the pinout
  _calibrations[R1].pin = 37;
  _calibrations[R2].pin = 31;
  _calibrations[R3].pin = 30;
  _calibrations[R4].pin = 29;
  _calibrations[L1].pin = 36;
  _calibrations[L2].pin = 26;
  _calibrations[L3].pin = 27;
  _calibrations[L4].pin = 28;
  _calibrations[HighRegister].isTouch = true;
  _calibrations[LowRegister].isTouch = true;
  // set all valid pins to input
  for (i = 0; i < InputCount; i++) {
    if (_calibrations[i].pin < 255) {
      pinMode(_calibrations[i].pin, INPUT);
    }
  }
  // configure analog input
  analogReadResolution(12);
  analogReference(EXTERNAL);
}

bool InputModel::highRegister() { return(_highRegister); }
void InputModel::setHighRegister(bool v) {
  if (v != _highRegister) {
    _highRegister = v;
    invalidate();
  }
}

bool InputModel::lowRegister() { return(_lowRegister); }
void InputModel::setLowRegister(bool v) {
  if (v != _lowRegister) {
    _lowRegister = v;
    invalidate();
  }
}

float InputModel::bite() { return(_bite); }
void InputModel::setBite(float v) {
  if (v != _bite) {
    _bite = v;
    invalidate();
  }
}

float InputModel::key(uint8_t i) {
  if (i < InputKeyCount) return(_keys[i]);
  return(0.0);
}
void InputModel::setKey(uint8_t i, float v) {
  if (i >= InputKeyCount) return;
  if (! (v >= 0.0)) v = 0.0;
  if (! (v <= 1.0)) v = 1.0;
  if (v != _keys[i]) {
    _keys[i] = v;
    invalidate();
  }
}

const char *InputModel::nameOfKey(uint8_t i) {
  switch (i) {
    case R1:  return("R1");
    case R2:  return("R2");
    case R3:  return("R3");
    case R4:  return("R4");
    case L1:  return("L1");
    case L2:  return("L2");
    case L3:  return("L3");
    case L4:  return("L4");
    default: return("?");
  }
}

bool InputModel::calibrating() { return(_calibrating); }
void InputModel::setCalibrating(bool v) {
  int i;
  if (v != _calibrating) {
    _calibrating = v;
    // mark all current calibrations as invalid
    if (_calibrating) {
      for (i = 0; i < InputCount; i++) {
        _calibrations[i].valid = false;
      }
    }
    // save settings when calibration is done
    else {
      if (onStorageChanged != NULL) onStorageChanged();
    }
    invalidate();
  }
}

void InputModel::read() {
  int i;
  float mapMin, mapMax;
  // read all inputs
  Calibration *cal;
  for (i = 0; i < InputCount; i++) {
    cal = &(_calibrations[i]);
    if (cal->pin == 255) continue;
    if (cal->isTouch) cal->raw = touchRead(cal->pin);
    else cal->raw = analogRead(cal->pin);
    // calibrate if needed
    if (_calibrating) {
      if (! cal->valid) {
        cal->min = cal->max = cal->first = cal->raw;
        cal->valid = true;
      }
      else {
        if (cal->raw < cal->min) cal->min = cal->raw;
        if (cal->raw > cal->max) cal->max = cal->raw;
        if ((cal->first == cal->min) ||
            ((cal->raw > cal->min) && (cal->raw < cal->first))) {
          cal->first = cal->raw;
        }
      }
    }
    // map to a float
    mapMin = (float)(cal->isTouch ? cal->min : cal->first);
    mapMax = (float)cal->max;
    if (mapMax == mapMin) cal->value = 0.0; // divide-by-zero protection
    else cal->value = ((float)cal->raw - mapMin) / (mapMax - mapMin);
    if (! (cal->value >= 0.0)) cal->value = 0.0;
    if (! (cal->value <= 1.0)) cal->value = 1.0;
  }
  // transfer to properties, invalidating when we're done
  for (i = 0; i < InputKeyCount; i++) {
    _keys[i] = _calibrations[i].value;
  }
  _bite = _calibrations[Bite].value;
  _highRegister = (_calibrations[HighRegister].value > 0.5);
  _lowRegister = (_calibrations[LowRegister].value > 0.5);
  invalidate();
}

uint8_t InputModel::storageBytes() {
  return(InputCount * 6);
}
void InputModel::store(uint8_t *buffer) {
  int i;
  int j = 0;
  uint16_t v;
  for (i = 0; i < InputCount; i++) {
    v = (uint16_t)_calibrations[i].min;
    buffer[j++] = (v >> 8) & 0xFF;
    buffer[j++] = v & 0xFF;
    v = (uint16_t)_calibrations[i].first;
    buffer[j++] = (v >> 8) & 0xFF;
    buffer[j++] = v & 0xFF;
    v = (uint16_t)_calibrations[i].max;
    buffer[j++] = (v >> 8) & 0xFF;
    buffer[j++] = v & 0xFF;
  }
}
void InputModel::recall(uint8_t *buffer) {
  int i;
  int j = 0;
  for (i = 0; i < InputCount; i++) {
    _calibrations[i].min = (buffer[j++] << 8);
    _calibrations[i].min |= buffer[j++];
    _calibrations[i].first = (buffer[j++] << 8);
    _calibrations[i].first |= buffer[j++];
    _calibrations[i].max = (buffer[j++] << 8);
    _calibrations[i].max |= buffer[j++];
    _calibrations[i].valid = true;
  }
}

