#include "touch.h"

Touch::Touch() : XPT2046_Touchscreen(TS_CS_PIN) { }

Point Touch::getScreenPoint(Screen *s) {
  // map the point to screen coordinates
  TS_Point tp = getPoint();
  // get a rotation-invariant screen size
  int16_t temp;
  int16_t screenWidth = s->width();
  int16_t screenHeight = s->height();
  int16_t sizeLong = screenWidth;
  int16_t sizeShort = screenHeight;
  if (sizeShort > sizeLong) {
    temp = sizeLong; sizeLong = sizeShort; sizeShort = temp;
  }
  Point p = { .x = (int16_t)map(tp.x, TS_MINX, TS_MAXX, 0, sizeLong),
              .y = (int16_t)map(tp.y, TS_MINY, TS_MAXY, 0, sizeShort) };
  // compensate for rotation
  uint8_t rotation = s->rotation();
  if ((rotation == 0) || (rotation == 2)) {
    temp = p.x; p.x = p.y; p.y = temp;
  }
  if ((rotation == 0) || (rotation == 3)) {
    p.x = screenWidth - p.x;
  }
  if ((rotation == 2) || (rotation == 3)) {
    p.y = screenHeight - p.y;
  }
  return(p);
}
