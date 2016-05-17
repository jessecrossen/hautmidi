#ifndef _HOODWIND_geom_h_
#define _HOODWIND_geom_h_

typedef uint16_t coord_t;

// define a struct to describe a screen point
typedef struct {
  coord_t x;
  coord_t y;
} Point;

// define a struct to describe a rectangular screen region
typedef struct {
  coord_t x;
  coord_t y;
  coord_t w;
  coord_t h;
} Rect;

#define insetRect(r, inset) { .x = (coord_t)(r.x + (inset)), \
                              .y = (coord_t)(r.y + (inset)), \
                              .w = (coord_t)(r.w - ((inset) * 2)), \
                              .h = (coord_t)(r.h - ((inset) * 2)) }
                              
#define trimRect(r, top, right, bottom, left) { \
                              .x = (coord_t)(r.x + (left)), \
                              .y = (coord_t)(r.y + (top)), \
                              .w = (coord_t)(r.w - ((left) + (right))), \
                              .h = (coord_t)(r.h - ((top) + (bottom))) }

#endif
