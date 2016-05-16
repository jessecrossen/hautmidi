#ifndef _HOODWIND_debug_h_
#define _HOODWIND_debug_h_

#define DEBUG 1

#if DEBUG
  #define LOG(msg) Serial.println(msg)
#else
  #define LOG(msg)
#endif

#endif
