#ifndef _HOODWIND_screen_h_
#define _HOODWIND_screen_h_

#include <ILI9341_t3.h>
#include <SPI.h>

#include "geom.h"
#include "style.h"

// DISPLAY PINOUT
#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12

#define TFT_LED      3

class Screen : public ILI9341_t3 {
  public:
    Screen();
    
    // store rotation when setting it
    uint8_t rotation();
    void setRotation(uint8_t r);
    
    // draw a string aligned inside the given rectangle
    void drawTextInRect(const char *string, Rect r, TextScheme ts);
    
    // control sleep state
    bool isSleeping();
    void setSleeping(bool v);
    
    // control display brightness
    uint8_t brightness();
    void setBrightness(uint8_t v);
    
  private:
    uint8_t _rotation;
    bool _isSleeping;
    uint8_t _brightness;
    elapsedMillis sinceSleepChange;
};

#endif
