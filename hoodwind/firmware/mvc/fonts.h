#ifndef _HOODWIND_fonts_h_
#define _HOODWIND_fonts_h_

#include <stdint.h>

typedef struct {
  uint8_t charWidth;
  uint8_t charHeight;
  uint8_t asciiMin;
  uint8_t asciiMax;
  const uint16_t *data;
} Font;

#ifdef __cplusplus
extern "C" {
#endif

// ter-u12b.bdf
extern const Font teru12b;

// ter-u14b.bdf
extern const Font teru14b;

// ter-u16b.bdf
extern const Font teru16b;

// ter-u18b.bdf
extern const Font teru18b;

// ter-u20b.bdf
extern const Font teru20b;

// ter-u22b.bdf
extern const Font teru22b;

// ter-u24b.bdf
extern const Font teru24b;

// ter-u28b.bdf
extern const Font teru28b;

// ter-u32b.bdf
extern const Font teru32b;

const Font *fontWithHeight(uint8_t h);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
