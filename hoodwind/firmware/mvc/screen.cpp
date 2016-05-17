#include "screen.h"

// copied from ILI9341_t3.cpp
#define SPICLOCK 30000000
// macros 
#define BEGIN_TRANSACTION SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0));
#define END_TRANSACTION SPI.endTransaction();

Screen::Screen(ScreenPinout pinout) 
    //!!!
    : ILI9341_t3(21, 20, 255, 7, 14, 12) {
  _activated = false;
  // set up the default pinout
  _brightnessPin = 255;
  _csPin = 10;
  _dcPin = 9;
  _misoPin = 12;
  _mosiPin = 11;
  _sckPin = 13;
  if (pinout == ScreenPinoutWithAudio) {
    _csPin = 21;
    _dcPin = 20;
    _mosiPin = 7;
    _sckPin = 14;
  }
  _dataFlags = _commandFlags = 0;
  // start with max brightness
  _brightness = 255;
  // rotation is indefinite when the screen is initialized
  _rotation = ScreenRotationUnknown;
  // the screen should be awake when it starts up
  _isSleeping = false;
}
void Screen::setCSPin(uint8_t pin) {
  if (_activated) return;
  switch(pin) {
    case 9: case 10: case 15: case 20: case 21:
      _csPin = pin;
  }
}
void Screen::setDCPin(uint8_t pin) {
  if (_activated) return;
  switch(pin) {
    case 9: case 10: case 15: case 20: case 21:
      _dcPin = pin;
  }
}
void Screen::setBrightnessPin(uint8_t pin) {
  _brightnessPin = pin;
  if (_brightnessPin < 255) {
    pinMode(_brightnessPin, OUTPUT);
    analogWrite(_brightnessPin, 0xFF - _brightness);
  }
}


void Screen::begin() {
  // start SPI
  SPI.setMOSI(_mosiPin);
  SPI.setMISO(_misoPin);
  SPI.setSCK(_sckPin);
  SPI.begin();
	// configure the chip select for the screen
  if (SPI.pinIsChipSelect(_csPin, _dcPin)) {
    _dataFlags = SPI.setCS(_csPin);
	  _commandFlags = _dataFlags | SPI.setCS(_dcPin);
	  // !!!
	  pcs_data = _dataFlags; 
	  pcs_command = _commandFlags;
  }
  else {
    //!!!
	  pcs_data = 0;
	  pcs_command = 0;
	  return;
  }
  // send initialization commands
	BEGIN_TRANSACTION
	_command({ .cmd=0xEF, .argc=3, .argv={0x03, 0x80, 0x02}});
	_command({ .cmd=0xCF, .argc=3, .argv={0x00, 0xC1, 0x30}});
	_command({ .cmd=0xED, .argc=4, .argv={0x64, 0x03, 0x12, 0x81}});
	_command({ .cmd=0xE8, .argc=3, .argv={0x85, 0x00, 0x78}});
	_command({ .cmd=0xCB, .argc=5, .argv={0x39, 0x2C, 0x00, 0x34, 0x02}});
	_command({ .cmd=0xF7, .argc=1, .argv={0x20}});
	_command({ .cmd=0xEA, .argc=2, .argv={0x00, 0x00}});
	// power control
	_command({ .cmd=ILI9341_PWCTR1, .argc=1, .argv={0x23}});
	_command({ .cmd=ILI9341_PWCTR2, .argc=1, .argv={0x10}});
	// VCM control
	_command({ .cmd=ILI9341_VMCTR1, .argc=2, .argv={0x3e, 0x28}});
	_command({ .cmd=ILI9341_VMCTR2, .argc=1, .argv={0x86}});
	// memory access control
	_command({ .cmd=ILI9341_MADCTL, .argc=1, .argv={0x48}});
	// pixel format
	_command({ .cmd=ILI9341_PIXFMT, .argc=1, .argv={0x55}});
	// frame rate
	_command({ .cmd=ILI9341_FRMCTR1, .argc=2, .argv={0x00, 0x18}});
	// display function control
	_command({ .cmd=ILI9341_DFUNCTR, .argc=3, .argv={0x08, 0x82, 0x27}});
	// gamma
	_command({ .cmd=0xF2, .argc=1, .argv={0x00}});
	_command({ .cmd=ILI9341_GAMMASET, .argc=1, .argv={0x01}});
	_command({ .cmd=ILI9341_GMCTRP1, .argc=15, 
	  .argv={0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 
	         0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}});
	_command({ .cmd=ILI9341_GMCTRN1, .argc=15, 
	  .argv={0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 
		       0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}});
	// wake the display
	writecommand_cont(ILI9341_DISPON);
	writecommand_last(ILI9341_SLPOUT);
	END_TRANSACTION
}

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

ScreenRotation Screen::rotation() { return(_rotation); }
void Screen::setRotation(ScreenRotation r) {
  if (r == ScreenRotationUnknown) return;
  if (r != _rotation) {
    _rotation = r;
	  BEGIN_TRANSACTION
	  _command({ .cmd=ILI9341_MADCTL, .argc=0 });
	  switch (_rotation) {
	    case ScreenRotationUnknown:
	    case ScreenRotationCableDown:
		    _send((uint8_t)(MADCTL_MX | MADCTL_BGR), false, true);
		    _width  = ILI9341_TFTWIDTH;
		    _height = ILI9341_TFTHEIGHT;
		    break;
	    case ScreenRotationCableRight:
		    _send((uint8_t)(MADCTL_MV | MADCTL_BGR), false, true);
		    _width  = ILI9341_TFTHEIGHT;
		    _height = ILI9341_TFTWIDTH;
		    break;
	    case ScreenRotationCableUp:
		    _send((uint8_t)(MADCTL_MY | MADCTL_BGR), false, true);
		    _width  = ILI9341_TFTWIDTH;
		    _height = ILI9341_TFTHEIGHT;
		    break;
	    case ScreenRotationCableLeft:
		    _send((uint8_t)(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR), false, true);
		    _width  = ILI9341_TFTHEIGHT;
		    _height = ILI9341_TFTWIDTH;
		    break;
	  }
	  END_TRANSACTION
	
	  //!!!
	  cursor_x = 0;
	  cursor_y = 0;
  }
}

coord_t Screen::width() { return(_width); }
coord_t Screen::height() { return(_height); }

bool Screen::isSleeping() { return(_isSleeping); }
void Screen::setSleeping(bool v) {
  if (v != _isSleeping) {
    // guard against too-frequent changes per datasheet
    if ((v) && (sinceSleepChange <= 120)) return;
    if ((! v) && (sinceSleepChange <= 5)) return;
    sinceSleepChange = 0;
    _isSleeping = v;
    // turn the display off when sleeping
    SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0));
    if (_isSleeping) {
		  writecommand_cont(ILI9341_DISPOFF);
		  writecommand_last(ILI9341_SLPIN);
	  } else {
		  writecommand_cont(ILI9341_DISPON);
		  writecommand_last(ILI9341_SLPOUT);
	  }
    SPI.endTransaction();
  }
}

uint8_t Screen::brightness() { return(_brightness); }
void Screen::setBrightness(uint8_t v) {
  if (v != _brightness) {
    _brightness = v;
    if (_brightnessPin < 255) {
      analogWrite(_brightnessPin, 0xFF - _brightness);
    }
  }
}

// DRAWING *******************************************************************

void Screen::fillRect(Rect r, uint16_t color) {
  uint16_t x, y;
	// clip the rectangle to the screen
	if ((r.x >= _width) || (r.y >= _height)) return;
	if ((r.x + r.w - 1) >= _width)  r.w = _width  - r.x;
	if ((r.y + r.h - 1) >= _height) r.h = _height - r.y;
	BEGIN_TRANSACTION
	setRegion(r);
	_command({ .cmd=ILI9341_RAMWR, .argc=0 });
	for (y = r.h; y > 0; y--) {
	  // send each scanline as a separate transaction
		for (x = r.w; x > 1; x--) _send(color);
		_send(color, false, true);
		// start a new transaction if we have more scanlines
		if ((y > 1) && (y & 1)) {
			END_TRANSACTION
			BEGIN_TRANSACTION
		}
	}
	END_TRANSACTION
}

void Screen::drawTextInRect(const char *string, Rect r, TextScheme ts) {
  // roughly compute the text size
  int16_t tw = (strlen(string) * ts.size * 5) / 6;
  int16_t th = ts.size;
  // position the text in the box
  setCursor(r.x + (int16_t)((float)(r.w - tw) * ts.xalign),
            r.y + (int16_t)((float)(r.h - th) * ts.yalign));
  // draw the text
  setFont(*ts.font);
  print(string);
}
