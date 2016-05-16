#ifndef _HOODWIND_touch_h_
#define _HOODWIND_touch_h_

#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#include "screen.h"
#include "geom.h"

// PINOUT
#define TS_CS_PIN  8

// CALIBRATION
#define TS_MINX    300
#define TS_MINY    300
#define TS_MAXX    3700
#define TS_MAXY    3700

class Touch : public XPT2046_Touchscreen {
  public:
    Touch();
    
    // get a point calibrated to coordinates on the given screen
    Point getScreenPoint(Screen *s);
};

#endif
