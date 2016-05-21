#ifndef _HOODWIND_screen_h_
#define _HOODWIND_screen_h_

#include <SPI.h>

#include "geom.h"
#include "style.h"
#include "fonts.h"

// the size of the screen
#define TFT_HEIGHT 320
#define TFT_WIDTH 240
// the number of pixels to allocate for the scan line buffer
#define SCAN_BUFFER_SIZE TFT_HEIGHT

// commands for the screen
#define ILI9341_NOP         0x00
#define ILI9341_SWRESET     0x01
#define ILI9341_RDDID       0x04
#define ILI9341_RDDST       0x09
#define ILI9341_SLPIN       0x10
#define ILI9341_SLPOUT      0x11
#define ILI9341_PTLON       0x12
#define ILI9341_NORON       0x13
#define ILI9341_RDMODE      0x0A
#define ILI9341_RDMADCTL    0x0B
#define ILI9341_RDPIXFMT    0x0C
#define ILI9341_RDIMGFMT    0x0D
#define ILI9341_RDSELFDIAG  0x0F
#define ILI9341_INVOFF      0x20
#define ILI9341_INVON       0x21
#define ILI9341_GAMMASET    0x26
#define ILI9341_DISPOFF     0x28
#define ILI9341_DISPON      0x29
#define ILI9341_CASET       0x2A
#define ILI9341_PASET       0x2B
#define ILI9341_RAMWR       0x2C
#define ILI9341_RAMRD       0x2E
#define ILI9341_PTLAR       0x30
#define ILI9341_MADCTL      0x36
#define ILI9341_VSCRSADD    0x37
#define ILI9341_PIXFMT      0x3A
#define ILI9341_FRMCTR1     0xB1
#define ILI9341_FRMCTR2     0xB2
#define ILI9341_FRMCTR3     0xB3
#define ILI9341_INVCTR      0xB4
#define ILI9341_DFUNCTR     0xB6
#define ILI9341_PWCTR1      0xC0
#define ILI9341_PWCTR2      0xC1
#define ILI9341_PWCTR3      0xC2
#define ILI9341_PWCTR4      0xC3
#define ILI9341_PWCTR5      0xC4
#define ILI9341_VMCTR1      0xC5
#define ILI9341_VMCTR2      0xC7
#define ILI9341_RDID1       0xDA
#define ILI9341_RDID2       0xDB
#define ILI9341_RDID3       0xDC
#define ILI9341_RDID4       0xDD
#define ILI9341_GMCTRP1     0xE0
#define ILI9341_GMCTRN1     0xE1

// color construction
#define TFT_RGB(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

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

class Screen {
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
