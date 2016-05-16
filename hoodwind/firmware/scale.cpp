#include "scale.h"
  
ScaleModel::ScaleModel() : Model() {
  _tonic = PitchClassC;
  _type = ScaleTypeMajor;
  _centerOctave = 4;
}

PitchClass ScaleModel::tonic() { return(_tonic); }
void ScaleModel::setTonic(PitchClass t) {
  t = (PitchClass)(t % PitchClassCount);
  if (t != _tonic) {
    _tonic = t;
    invalidate();
  }
}
const char *ScaleModel::nameOfPitchClass(PitchClass pc) {
  switch (pc) {
    case PitchClassA:      return("A");
    case PitchClassBFlat:  return("Bb");
    case PitchClassB:      return("B");
    case PitchClassC:      return("C");
    case PitchClassCSharp: return("C#");
    case PitchClassD:      return("D");
    case PitchClassEFlat:  return("Eb");
    case PitchClassE:      return("E");
    case PitchClassF:      return("F");
    case PitchClassFSharp: return("F#");
    case PitchClassG:      return("G");
    case PitchClassAFlat:  return("Ab");
    default:               return("?");
  }
}

ScaleType ScaleModel::type() { return(_type); }
void ScaleModel::setType(ScaleType t) {
  t = (ScaleType)(t % ScaleTypeCount);
  if (t != _type) {
    _type = t;
    invalidate();
  }
}
const char *ScaleModel::nameOfScaleType(ScaleType t) {
  switch (t) {
    case ScaleTypeMajor:      return("Major");
    case ScaleTypeMinor:      return("Minor");
    case ScaleTypeWholeTone:  return("Whole Tone");
    case ScaleTypeChromatic:  return("Chromatic");
    case ScaleTypePentatonic: return("Pentatonic");
    case ScaleTypeBlues:      return("Blues");
    default:                  return("?");
  }
}
const char *ScaleModel::shortNameOfScaleType(ScaleType t) {
  switch (t) {
    case ScaleTypeMajor:      return("MAJ");
    case ScaleTypeMinor:      return("MIN");
    case ScaleTypeWholeTone:  return("WHT");
    case ScaleTypeChromatic:  return("CHR");
    case ScaleTypePentatonic: return("PNT");
    case ScaleTypeBlues:      return("BLU");
    default:                  return("?");
  }
}

uint8_t ScaleModel::centerOctave() { return(_centerOctave); }
void ScaleModel::setCenterOctave(uint8_t oct) {
  if (oct < 1) oct = 1;
  if (oct > 8) oct = 8;
  if (oct != _centerOctave) {
    _centerOctave = oct;
    invalidate();
  }
}
const char *ScaleModel::nameOfOctave(uint8_t oct) {
  switch (oct) {
    case 0:   return("OCT 0");
    case 1:   return("OCT 1");
    case 2:   return("OCT 2");
    case 3:   return("OCT 3");
    case 4:   return("OCT 4");
    case 5:   return("OCT 5");
    case 6:   return("OCT 6");
    case 7:   return("OCT 7");
    case 8:   return("OCT 8");
    case 9:   return("OCT 9");
    default:  return("?");
  }
}
