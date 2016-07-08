#ifndef _HOODWIND_BMP180_h_
#define _HOODWIND_BMP180_h_

#include "Wire.h"

typedef enum {
  PressureOversampleLowPower = 0x34,
  PressureOversampleStandard = 0x74,
  PressureOversampleHighRes = 0xB4,
  PressureOversampleUltraHighRes = 0xF4
} PressureOversample;

class BMP180 {
  public:
    BMP180() {
      I2C_ADDR = 0x77;
      oversample = PressureOversampleUltraHighRes;
      temperatureInterval = 1000;
      pressureInterval = 100;
      _hasStartedWire = false;
      _hasCalibrated = false;
      _sinceLastTemperature = 2000;
      _temperatureRequested = false;
      _pressureRequested = false;
      _pressure = nan("");
      _temperature = nan("");
      trace = false;
    }
    // calibrate the device and return whether it worked
    bool calibrate();
    // update the current pressure measurement 
    //  (call as often as possible for best performance)
    void update();
    // get the last temperature reading
    double temperature() { return(_temperature); }
    // get the last pressure reading
    double pressure() { return(_pressure); }
    
    // the amount of oversampling to use
    PressureOversample oversample;
    // the rate to sample temperature at
    unsigned int temperatureInterval;
    // the rate to sample pressure at
    unsigned int pressureInterval;
    // the I2C address of the sensor
    char I2C_ADDR;
    // the last error returned by a call
    int lastError;
    // whether to trace actions to the serial console
    bool trace;
    
  protected:
    double readFloat(char addr);
    int readInt(char addr, bool isSigned=true);
    int readUInt(char addr) {
      return(readInt(addr, false));
    }
    void updateTemperature();
    void updatePressure();    
    unsigned int pressureDelay();
    
  private:
    // whether we've started the I2C interface
    bool _hasStartedWire;
    // whether we've successfully fetched calibration data
    bool _hasCalibrated;
    // calibration parameters
    double _c5, _c6, _mc, _md, _x0, _x1, _x2, _y0, _y1, _y2, _p0, _p1, _p2;
    // the setting for pressure oversampling
    PressureOversample _oversample;
    // last temperature reading
    double _temperature;
    // time since the last successful temperature reading
    elapsedMillis _sinceLastTemperature;
    // time since a temperature reading was last requested
    elapsedMillis _sinceLastTemperatureRequest;
    bool _temperatureRequested;
    // last pressure reading
    double _pressure;
    // time since the last successful pressure reading
    elapsedMillis _sinceLastPressure;
    // time since a pressure reading was last requested
    elapsedMillis _sinceLastPressureRequest;
    // the number of milliseconds to wait for the pressure reading
    unsigned int _pressureDelay;
    bool _pressureRequested;
};

#endif
