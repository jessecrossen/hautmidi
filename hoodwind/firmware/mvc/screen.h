#ifndef _HOODWIND_screen_h_
#define _HOODWIND_screen_h_

#include <ILI9341_t3.h>
#include <SPI.h>

#include "geom.h"
#include "style.h"
#include "fonts.h"

// the number of pixels to allocate for the scan line buffer
#define SCAN_BUFFER_SIZE ILI9341_TFTHEIGHT

typedef enum {
  ScreenPinoutDefault,
  ScreenPinoutWithAudio
} ScreenPinout;

typedef enum {
  // the location of the TFT cable with respect to the display
  //  (on the display sold at http://pjrc.com)
  ScreenRotationCableDown,
  ScreenRotationCableRight,
  ScreenRotationCableUp,
  ScreenRotationCableLeft,
  ScreenRotationUnknown
} ScreenRotation;

typedef struct {
  uint8_t cmd;
  uint8_t argc;
  uint8_t argv[15];
} DisplayCommand;

class Screen : public ILI9341_t3 {
  public:
    Screen(ScreenPinout pinout);
    // change pins
    void setCSPin(uint8_t pin);
    void setDCPin(uint8_t pin);
    void setBrightnessPin(uint8_t pin);
    
    // start the screen driver
    void begin();
    
    // store rotation when setting it
    ScreenRotation rotation();
    void setRotation(ScreenRotation r);
    
    // screen size
    coord_t width();
    coord_t height();
    
    // control sleep state
    bool isSleeping();
    void setSleeping(bool v);
    
    // control display brightness
    uint8_t brightness();
    void setBrightness(uint8_t v);
    
    // a buffer to hold a scan line in preparation for drawing
    color_t scanBuffer[SCAN_BUFFER_SIZE];
    // fill the scan buffer with a color
    void fillScanBuffer(coord_t x, coord_t w, color_t c);
    // commit a portion of the scan buffer to the device
    void commitScanBuffer(coord_t x, coord_t y, coord_t w);
    // fill a rectangle on the screen with the given color
    void fillRect(Rect r, color_t color);
    // draw text to a scan line
    void scanText(coord_t tx, coord_t ty, const char *s, const Font *font, color_t color);
    
  private:
    // the size of the screen
    coord_t _width, _height;
    // whether the begin method has been called
    bool _activated;
    // the current rotation constant (0-3)
    ScreenRotation _rotation;
    // whether the display is sleeping
    bool _isSleeping;
    // the current backlight brightness (0-255)
    uint8_t _brightness;
    // the time since the sleep state last changed
    elapsedMillis sinceSleepChange;
    // pinout
    uint8_t _csPin, _dcPin, _mosiPin, _sckPin, _misoPin, _brightnessPin;
    // flags for selecting whether an SPI instruction is for data or a command
    uint8_t _commandFlags, _dataFlags;
    
    // SPI UTILS **************************************************************
    
    // send a display command
    void _command(DisplayCommand c, bool isLast=false) {
      _send(c.cmd, true, isLast && (c.argc == 0));
      uint8_t lastIndex = isLast ? c.argc - 1 : 255;
      for (uint8_t i = 0; i < c.argc; i++) {
        _send(c.argv[i], false, i == lastIndex);
      }
    }
    // send an 8-bit or 16-bit value via SPI
    template <typename T>
    void _send(T value, bool isCommand=false, bool isLast=false) {
      static uint32_t mcr;
      static uint32_t sr;
      static uint32_t tmp __attribute__((unused));
      if (isLast) mcr = SPI0_MCR;
      // push data
      KINETISK_SPI0.PUSHR = value | 
        ((isCommand ? _commandFlags : _dataFlags) << 16) | 
        SPI_PUSHR_CTAS(sizeof(T) - 1) | 
        (isLast ? SPI_PUSHR_EOQ : SPI_PUSHR_CONT);
      // wait for queues to empty
      if (isLast) {
        while (1) {
	        sr = KINETISK_SPI0.SR;
	        if (sr & SPI_SR_EOQF) break;  // wait for last transmit
	        if (sr &  0xF0) tmp = KINETISK_SPI0.POPR;
        }
        KINETISK_SPI0.SR = SPI_SR_EOQF;
        SPI0_MCR = mcr;
        while (KINETISK_SPI0.SR & 0xF0) {
	        tmp = KINETISK_SPI0.POPR;
        }
      }
      else {
        do {
	        sr = KINETISK_SPI0.SR;
	        if (sr & 0xF0) tmp = KINETISK_SPI0.POPR;  // drain RX FIFO
        } while ((sr & (15 << 12)) > (3 << 12));
      }
    }
    
    void setRegion(Rect r) __attribute__((always_inline)) {
      _command({ .cmd=ILI9341_CASET, .argc=0 });
      _send(r.x);
      _send((uint16_t)(r.x + r.w - 1));
      _command({ .cmd=ILI9341_PASET, .argc=0 });
      _send(r.y);
      _send((uint16_t)(r.y + r.h - 1));
    }
};

#endif
