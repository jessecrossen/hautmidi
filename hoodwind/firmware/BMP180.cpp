#include "BMP180.h"

bool BMP180::calibrate() {
  // start the I2C interface if we haven't already
  if (! _hasStartedWire) {
    if (trace) Serial.println("BMP180: starting Wire");
    Wire.begin();
    _hasStartedWire = true;
  }
  // exit if we've already calibrated
  if (_hasCalibrated) return(true);
  if (trace) Serial.print("BMP180: calibrating...");
  lastError = 0;
  int16_t AC1, AC2, AC3, VB1, VB2, MC, MD;
  uint16_t AC4, AC5, AC6;
  // read calibration registers
  AC1 = readInt(0xAA);
  AC2 = readInt(0xAC);
  AC3 = readInt(0xAE);
  AC4 = readUInt(0xB0);
  AC5 = readUInt(0xB2);
  AC6 = readUInt(0xB4);
  VB1 = readInt(0xB6);
  VB2 = readInt(0xB8);
  MC = readInt(0xBC);
  MD = readInt(0xBE);
  // make sure we got good data
  if (lastError != 0) {
    if (trace) {
      Serial.print("ERROR ");
      Serial.println(lastError);
    }
    return(false);
  }
  // precompute calibration parameters
  double c3 = 160.0 * pow(2, -15) * AC3;
	double c4 = pow(10, -3) * pow(2, -15) * AC4;
	double b1 = pow(160, 2) * pow(2, -30) * VB1;
	_c5 = (pow(2, -15) / 160) * AC5;
	_c6 = AC6;
	_mc = (pow(2, 11) / pow(160, 2)) * MC;
	_md = MD / 160.0;
	_x0 = AC1;
	_x1 = 160.0 * pow(2, -13) * AC2;
	_x2 = pow(160, 2) * pow(2, -25) * VB2;
	_y0 = c4 * pow(2, 15);
	_y1 = c4 * c3;
	_y2 = c4 * b1;
	_p0 = (3791.0 - 8.0) / 1600.0;
	_p1 = 1.0 - 7357.0 * pow(2, -20);
	_p2 = 3038.0 * 100.0 * pow(2, -36);
	_hasCalibrated = true;
	if (trace) Serial.println("DONE");
  return(true);
}

void BMP180::update() {
  // make sure we've calibrated
  if (! calibrate()) return;
  // make sure we have a valid temperature at least once per second
  if ((_sinceLastTemperature > temperatureInterval) && 
      (! _pressureRequested)) {
    updateTemperature();
  }
  // don't update pressure when we're waiting for a temperature request
  if (_temperatureRequested) return;
  // update the pressure at the requested interval
  if ((_sinceLastPressure > pressureInterval) || (_pressureRequested)) {
    updatePressure();
  }
}

void BMP180::updateTemperature() {
  lastError = 0;
  if (! _temperatureRequested) {
    if (trace) Serial.print("BMP180: requesting temperature...");
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0xF4);
    Wire.write(0x2E);
    lastError = Wire.endTransmission();
    if (lastError != 0) {
      if (trace) {
        Serial.print("ERROR ");
        Serial.println(lastError);
      }
      return;
    }
    _sinceLastTemperatureRequest = 0;
    _temperatureRequested = true;
    if (trace) Serial.println("DONE, waiting 5ms");
    return;
  }
  if (_sinceLastTemperatureRequest >= 5) {
    if (trace) Serial.print("BMP180: reading temperature...");
    _temperatureRequested = false;
    double ut = (double)readUInt(0xF6);
    if (lastError != 0) {
      Serial.print("ERROR ");
      Serial.println(lastError);
      return;
    }
    double a = _c5 * (ut - _c6);
		_temperature = a + (_mc / (a + _md));
		_sinceLastTemperature = 0;
		if (trace) {
		  Serial.print("DONE (");
		  Serial.print(ut); Serial.print(" => "); Serial.print(_temperature);
		  Serial.println(")");
		}
  }
}

void BMP180::updatePressure() {
  lastError = 0;
  // request a pressure reading
  if (! _pressureRequested) {
    if (trace) Serial.print("BMP180: requesting pressure...");
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0xF4);
    Wire.write(oversample);
    lastError = Wire.endTransmission();
    if (lastError != 0) {
      if (trace) {
        Serial.print("ERROR ");
        Serial.println(lastError);
      }
      return;
    }
    // see how long we need to wait for the result based on oversampling
    _pressureDelay = pressureDelay();
    _sinceLastPressureRequest = 0;
    _pressureRequested = true;
    if (trace) {
      Serial.print("DONE, waiting ");
      Serial.print(_pressureDelay); Serial.println("ms");
    }
    return;
  }
  // see if the last requested reading is ready
  if (_sinceLastPressureRequest >= _pressureDelay) {
    if (trace) Serial.print("BMP180: reading pressure...");
    _pressureRequested = false;
    double up = readFloat(0xF6);
    if (lastError != 0) {
      if (trace) {
        Serial.print("ERROR ");
        Serial.println(lastError);
      }
      return;
    }
    // apply calibration parameters
		double s = _temperature - 25.0;
		double x = (_x2 * pow(s, 2)) + (_x1 * s) + _x0;
		double y = (_y2 * pow(s, 2)) + (_y1 * s) + _y0;
		double z = (up - x) / y;
		_pressure = (_p2 * pow(z, 2)) + (_p1 * z) + _p0;
		_sinceLastPressure = 0;
		if (trace) {
		  Serial.print("DONE (");
		  Serial.print(up); Serial.print(" => "); Serial.print(_pressure);
		  Serial.println(")");
		}
  }
}

unsigned int BMP180::pressureDelay() {
  switch (oversample) {
    case PressureOversampleLowPower:
      return(5);
    case PressureOversampleStandard:
      return(8);
    case PressureOversampleHighRes:
      return(14);
    case PressureOversampleUltraHighRes:
      return(26);
    default:
      return(5);
  }
}

int BMP180::readInt(char addr, bool isSigned) {
  Wire.beginTransmission(I2C_ADDR);
	Wire.write(addr);
	lastError |= Wire.endTransmission();
  if (lastError != 0) return(0);
  Wire.requestFrom(I2C_ADDR, 2);
  while (Wire.available() < 2) { };
  if (isSigned) {
    return((int16_t)(Wire.read() << 8) | Wire.read());
  }
  else {
    return((uint16_t)(Wire.read() << 8) | Wire.read());
  }
}

double BMP180::readFloat(char addr) {
  Wire.beginTransmission(I2C_ADDR);
	Wire.write(addr);
	lastError |= Wire.endTransmission();
  if (lastError != 0) return(0);
  Wire.requestFrom(I2C_ADDR, 3);
  while (Wire.available() < 3) { };
  double v = 0.0;
  v += Wire.read() * 256.0;
  v += (double)Wire.read();
  v += Wire.read() / 256.0;
  return(v);
}
