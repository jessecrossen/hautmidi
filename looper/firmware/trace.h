#ifndef LOOPER_TRACE_H
#define LOOPER_TRACE_H

#define TRACE 1

#define WARN (TRACE && 1)
#define INFO (TRACE && 1)
#define DEBUG (TRACE && 1)

#if WARN
  #define WARN1(a) { Serial.print("!!! "); Serial.println(a); }
  #define WARN2(a,b) { Serial.print("!!! "); Serial.print(a); Serial.print(" "); Serial.println(b); }
  #define WARN3(a,b,c) { Serial.print("!!! "); Serial.print(a); Serial.print(" "); Serial.print(b); Serial.print(" "); Serial.println(c); }
#else
  #define WARN1(a) ()
  #define WARN2(a,b) ()
  #define WARN3(a,b,c) ()
#endif

#if INFO
  #define INFO1(a) { Serial.print("... "); Serial.println(a); }
  #define INFO2(a,b) { Serial.print("... "); Serial.print(a); Serial.print(" "); Serial.println(b); }
  #define INFO3(a,b,c) { Serial.print("... "); Serial.print(a); Serial.print(" "); Serial.print(b); Serial.print(" "); Serial.println(c); }
#else
  #define INFO1(a) ()
  #define INFO2(a,b) ()
  #define INFO3(a,b,c) ()
#endif

#if DEBUG
  #define DBG1(a) { Serial.print("  . "); Serial.println(a); }
  #define DBG2(a,b) { Serial.print("  . "); Serial.print(a); Serial.print(" "); Serial.println(b); }
  #define DBG3(a,b,c) { Serial.print("  . "); Serial.print(a); Serial.print(" "); Serial.print(b); Serial.print(" "); Serial.println(c); }
#else
  #define DBG1(a) ()
  #define DBG2(a,b) ()
  #define DBG3(a,b,c) ()
#endif

#endif
