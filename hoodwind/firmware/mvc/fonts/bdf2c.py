#!/usr/bin/env python

import sys
import os
import os.path
import glob
import re

def add_font(path):
  h = ''
  c = ''
  # get an identifier-friendly name for the font
  name = re.sub(r'\W+', '', os.path.basename(path)[0:-4])
  # declare the font in the header
  h += '// '+os.path.basename(path)+'\n'
  h += 'extern const Font %s;\n' % name
  # parse the file into a data structure
  charWidth = charHeight = leading = 0
  asciiMin = asciiMax = charCode = None
  inBitmap = False
  table = dict()
  with open(path, 'r') as f:
    for line in f:
      line = line.strip()
      if (line.startswith('FONTBOUNDINGBOX')):
        parts = line.split(' ')
        charWidth = int(parts[1])
        charHeight = int(parts[2])
        leading = abs(int(parts[4]))
        charHeight -= leading
      elif (line.startswith('ENCODING')):
        parts = line.split(' ')
        charCode = int(parts[1])
        if ((charCode >= 32) and (charCode <= 126)):
          table[charCode] = list()
          inBitmap = False
        else:
          charCode = None
      elif (line.startswith('BITMAP')):
        inBitmap = True
      elif (line.startswith('ENDCHAR')):
        inBitmap = False
      elif ((inBitmap) and (charCode != None)):
        while (len(line) < 4):
          line += '0'
        table[charCode].append(int(line, 16))
  asciiMin = min(table.keys())
  asciiMax = max(table.keys())
  
  c += 'static const uint16_t %s_data[] = {\n' % name
  for charCode in range(asciiMin, asciiMax + 1):
    c += '  '
    lines = table[charCode]
    # strip off the leading from the top, shifting ascending characters 
    #  like quotes down to fit in the X height
    for i in range(0, leading):
      if (lines[0] == 0):
        lines = lines[1:]
      else:
        lines = lines[0:-1]
    # reverse line bits so the leftmost pixel is the LSB
    for line in lines:
      reversedBits = 0
      for b in range(0, 16):
        reversedBits |= ((line >> b) & 0x01)
        reversedBits <<= 1
      c += '0x%04x, ' % reversedBits
    c += '\n'
  c += '};\n'
  c += '\n'
  c += 'const Font %s = {\n' % name
  c += '  %i, // charWidth\n' % charWidth
  c += '  %i, // charHeight\n' % charHeight
  c += '  %i, // asciiMin\n' % asciiMin
  c += '  %i, // asciiMax\n' % asciiMax
  c += '  %s_data // data\n' % name
  c += '};\n'
  
  return((h, c, charHeight, name))

h = '''#ifndef _HOODWIND_fonts_h_
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

'''
c = '#include "fonts.h"\n\n'
heightAndName = list()
for path in sorted(glob.glob('./bdf/*.bdf')):
  (fh, fc, height, name) = add_font(path)
  h += fh + '\n'
  c += fc + '\n'
  heightAndName.append((height, name))

h += 'const Font *fontWithHeight(uint8_t h);\n'
h += '''

#ifdef __cplusplus
} // extern "C"
#endif

#endif
'''

c += 'const Font *fontWithHeight(uint8_t h) {\n'
for item in heightAndName:
  c += '  if (h <= %i) return(&%s);\n' % item
c += '  return(&%s);\n' % heightAndName[-1][1];
c += '}\n'

with open('../fonts.h', 'w') as f:
  f.write(h)
with open('../fonts.c', 'w') as f:
  f.write(c)
