#ifndef _HOODWIND_scale_h_
#define _HOODWIND_scale_h_

#include "mvc/model.h"
#include "input.h"

typedef enum {
  PitchClassStart = 0,
  PitchClassC = 0,
  PitchClassCSharp,
  PitchClassD,
  PitchClassEFlat,
  PitchClassE,
  PitchClassF,
  PitchClassFSharp,
  PitchClassG,
  PitchClassAFlat,
  PitchClassA,
  PitchClassBFlat,
  PitchClassB,
  PitchClassCount
} PitchClass;

typedef enum {
  ScaleTypeStart = 0,
  ScaleTypeMajor = 0,
  ScaleTypeMinor,
  ScaleTypePentatonic,
  ScaleTypeBlues,
  ScaleTypeChromatic,
  ScaleTypeWholeTone,
  ScaleTypeDiminished,
  ScaleTypeOctatonic,
  ScaleTypeCount
} ScaleType;

typedef struct {
  // a series of intervals in semitones making up the scale
  float intervals[12];
  // the number of steps after which the interval series repeats
  int repeat;
  // the number of scale steps after which the interval series makes an octave
  int octave;
} ScaleProperties;

#define CenterOctaveMin 1
#define CenterOctaveMax 8
#define CenterOctaveCount ((CenterOctaveMax - CenterOctaveMin) + 1)

class ScaleModel : public Model {
  public:
    ScaleModel(InputModel *input);
    
    // get/set the tonic note of the scale
    PitchClass tonic();
    void setTonic(PitchClass t);
    // get/set the type of the scale
    ScaleType type();
    void setType(ScaleType t);
    // get set the center octave
    uint8_t centerOctave();
    void setCenterOctave(uint8_t oct);
    
    // interpret input parameters
    void interpret();
    
    // get the currently selected frequency in Hz
    float frequency();
    // get the current amplitude from 0.0 to 1.0
    float amplitude();
    // get the current modulation amount from 0.0 to 1.0
    float modulation();
    
    // get scale definitions
    static const ScaleProperties *propertiesOfScaleType(ScaleType t);
    
    // get friendly strings
    static const char *nameOfPitchClass(PitchClass pc);
    static const char *nameOfScaleType(ScaleType t);
    static const char *shortNameOfScaleType(ScaleType t);
    static const char *nameOfOctave(uint8_t oct);
  
  private:
    InputModel *_input;
    PitchClass _tonic;
    ScaleType _type;
    uint8_t _centerOctave;
    float _frequency, _amplitude, _modulation;
};

#endif
