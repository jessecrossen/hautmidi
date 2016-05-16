#include "screen.h"

// copied from ILI9341_t3.cpp
#define SPICLOCK 30000000

Screen::Screen() : 
    ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO) {
  // configure the display brightness pin for PWM
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW);
  _brightness = 255;
  // rotation is indefinite when the screen is initialized
  _rotation = -1;
  // the screen should be awake when it starts up
  _isSleeping = false;
}
 
uint8_t Screen::rotation() { return(_rotation); }
void Screen::setRotation(uint8_t r) {
  _rotation = r;
  ILI9341_t3::setRotation(r);
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
    analogWrite(TFT_LED, 0xFF - _brightness);
  }
}
