#ifndef _HOODWIND_geom_h_
#define _HOODWIND_geom_h_

// define a struct to describe a screen point
typedef struct {
  int16_t x;
  int16_t y;
} Point;

// define a struct to describe a rectangular screen region
typedef struct {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} Rect;

#endif
