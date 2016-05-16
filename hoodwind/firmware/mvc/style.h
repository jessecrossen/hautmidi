#ifndef _HOODWIND_style_h_
#define _HOODWIND_style_h_

typedef ILI9341_t3_font_t Font;

// define a standardized color scheme format
typedef struct {
  uint16_t bg; // background
  uint16_t fg; // foreground
  uint16_t active; // used to indicate that an area accepts touches
  uint16_t accent; // a color that pops against the others
} ColorScheme;

// define a standardized text scheme format
typedef struct {
  const Font *font;
  uint8_t size;
  float xalign; // 0.0 = left, 0.5 = center, 1.0 = right
  float yalign; // 0.0 = top, 0.5 = middle, 1.0 = bottom
} TextScheme;

#endif
