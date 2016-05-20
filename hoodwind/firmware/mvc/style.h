#ifndef _HOODWIND_style_h_
#define _HOODWIND_style_h_

#include "fonts.h"

typedef uint16_t color_t;

// define a standardized color scheme format
typedef struct {
  color_t bg; // background
  color_t fg; // foreground
  color_t active; // used to indicate that an area accepts touches
  color_t accent; // a color that pops against the others
} ColorScheme;

// define a standardized text scheme format
typedef struct {
  const Font *font;
  float xalign; // 0.0 = left, 0.5 = center, 1.0 = right
  float yalign; // 0.0 = top, 0.5 = middle, 1.0 = bottom
} TextScheme;

#endif
