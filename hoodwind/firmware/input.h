#ifndef _HOODWIND_input_h_
#define _HOODWIND_input_h_

#include "mvc/model.h"
#include "lib/BMP180.h"

#define InputKeyCount 8
#define InputCount (InputKeyCount + 4)

#define InputCalibrationBytes (InputCount * 6)

// a calibration for a specific input pin
typedef struct {
  uint8_t pin; // the pin to read input from
  int raw; // the raw value last read from the pin
  int min; // the calibrated minimum value
  int first; // the next up value from the min, deals with the "jump" in pots
  int max; // the calibrated maximum value
  bool valid; // whether the initial calibration values have been set
  bool isTouch; // whether the input should be read as a capacitive touch
  float value; // the calibrated and scaled value of the input
} Calibration;

class InputModel : public Model {
  public:
    InputModel();
    
    // get/set register switches
    bool highRegister();
    void setHighRegister(bool v);
    bool lowRegister();
    void setLowRegister(bool v);
    // get/set breath pressure
    float breath();
    void setBreath(float v);
    // get/set bite pressure
    float bite();
    void setBite(float v);
    // get/set key depressions
    float key(uint8_t i);
    void setKey(uint8_t i, float v);
    static const char *nameOfKey(uint8_t i);
    // enable/disable calibration mode
    bool calibrating();
    void setCalibrating(bool v);
    // read input from the device
    void read();
    // persist calibration
    virtual uint8_t storageBytes();
    virtual void store(uint8_t *buffer);
    virtual void recall(uint8_t *buffer);
    
  private:
    // whether the register toggles are on
    bool _highRegister;
    bool _lowRegister;
    // the baseline atmospheric pressure
    float _atmosphericPressure;
    // the current breath pressure (0.0 to 1.0)
    float _breath;
    // the current bite pressure (0.0 to 1.0)
    float _bite;
    // the current depression of each key
    float _keys[InputKeyCount];
    // whether inputs are being calibrated
    bool _calibrating;
    // calibrations for each input
    Calibration _calibrations[InputCount];
    // breath sensor
    BMP180 *_breathSensor;
};

#endif
