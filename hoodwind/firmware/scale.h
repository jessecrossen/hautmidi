#ifndef _HOODWIND_scale_h_
#define _HOODWIND_scale_h_

#include "mvc/model.h"

typedef enum {
  PitchClassStart = 0,
  PitchClassA = 0,
  PitchClassBFlat,
  PitchClassB,
  PitchClassC,
  PitchClassCSharp,
  PitchClassD,
  PitchClassEFlat,
  PitchClassE,
  PitchClassF,
  PitchClassFSharp,
  PitchClassG,
  PitchClassAFlat,
  PitchClassCount
} PitchClass;

typedef enum {
  ScaleTypeStart = 0,
  ScaleTypeMajor = 0,
  ScaleTypeMinor,
  ScaleTypeWholeTone,
  ScaleTypeChromatic,
  ScaleTypePentatonic,
  ScaleTypeBlues,
  ScaleTypeCount
} ScaleType;

#define CenterOctaveMin 1
#define CenterOctaveMax 8
#define CenterOctaveCount ((CenterOctaveMax - CenterOctaveMin) + 1)

class ScaleModel : public Model {
  public:
    ScaleModel();
    
    // get/set the tonic note of the scale
    PitchClass tonic();
    void setTonic(PitchClass t);
    // get/set the type of the scale
    ScaleType type();
    void setType(ScaleType t);
    // get set the center octave
    uint8_t centerOctave();
    void setCenterOctave(uint8_t oct);
    
    // get friendly strings
    static const char *nameOfPitchClass(PitchClass pc);
    static const char *nameOfScaleType(ScaleType t);
    static const char *shortNameOfScaleType(ScaleType t);
    static const char *nameOfOctave(uint8_t oct);
  
  private:
    PitchClass _tonic;
    ScaleType _type;
    uint8_t _centerOctave;
};

#endif
