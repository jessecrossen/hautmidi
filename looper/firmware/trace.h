#ifndef TRACE
  #define TRACE 1
#endif

#ifndef WARN
  #define WARN (TRACE && 1)
#endif

#ifndef INFO
  #define INFO (TRACE && 1)
#endif

#ifndef DEBUG
  #define DEBUG (TRACE && 1)
#endif

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
  #define DBG4(a,b,c,d) { Serial.print("  . "); Serial.print(a); Serial.print(" "); Serial.print(b); Serial.print(" "); Serial.println(c); Serial.print(" "); Serial.println(d); }
#else
  #define DBG1(a) ()
  #define DBG2(a,b) ()
  #define DBG3(a,b,c) ()
  #define DBG4(a,b,c,d) ()
#endif
