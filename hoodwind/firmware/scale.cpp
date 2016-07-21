#include "scale.h"
  
ScaleModel::ScaleModel(InputModel *input) : Model() {
  _input = input;
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
    case PitchClassC:      return("C");
    case PitchClassCSharp: return("C#");
    case PitchClassD:      return("D");
    case PitchClassEFlat:  return("Eb");
    case PitchClassE:      return("E");
    case PitchClassF:      return("F");
    case PitchClassFSharp: return("F#");
    case PitchClassG:      return("G");
    case PitchClassAFlat:  return("Ab");
    case PitchClassA:      return("A");
    case PitchClassBFlat:  return("Bb");
    case PitchClassB:      return("B");
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
    case ScaleTypeMajor:       return("MAJ");
    case ScaleTypeMinor:       return("MIN");
    case ScaleTypePentatonic:  return("PNT");
    case ScaleTypeBlues:       return("BLU");
    case ScaleTypeChromatic:   return("CHR");
    case ScaleTypeWholeTone:   return("WHT");
    case ScaleTypeDiminished:  return("DIM");
    case ScaleTypeOctatonic:   return("OCT");
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

const ScaleProperties *ScaleModel::propertiesOfScaleType(ScaleType t) {
  switch (t) {
    // diatonic major scale
    case ScaleTypeMajor:
      static ScaleProperties major = {
          .intervals={ 2, 2, 1, 2, 2, 2, 1 },
          .repeat=7, .octave=7
        };
      return(&major);
    // aeolian mode minor scale
    case ScaleTypeMinor:
      static ScaleProperties minor = {
          .intervals={ 2, 1, 2, 2, 1, 2, 2 },
          .repeat=7, .octave=7
        };
      return(&minor);
    // plain western pentatonic scale
    case ScaleTypePentatonic:
      static ScaleProperties pentatonic = {
          .intervals={ 2, 2, 3, 2, 3 },
          .repeat=5, .octave=5
        };
      return(&pentatonic);
    // minor pentatonic blues scale
    case ScaleTypeBlues:
      static ScaleProperties blues = {
          .intervals={ 3, 2, 1, 1, 3, 2 },
          .repeat=6, .octave=6
        };
      return(&blues);
    // whole tone scale
    case ScaleTypeWholeTone:
      static ScaleProperties whole = {
          .intervals={ 2 },
          .repeat=1, .octave=6
        };
      return(&whole);
    // diminished scale
    case ScaleTypeDiminished:
      static ScaleProperties diminished = {
          .intervals={ 2, 1 },
          .repeat=2, .octave=8
        };
      return(&diminished);
    case ScaleTypeOctatonic:
      static ScaleProperties octatonic = {
          .intervals={ 1, 2 },
          .repeat=2, .octave=8
        };
      return(&octatonic);
    // default to a chromatic scale
    default:
      static ScaleProperties chromatic = {
          .intervals={ 1 },
          .repeat=1, .octave=12
        };
      return(&chromatic);
  }
}

void ScaleModel::interpret() {
  int i, key;
  // get the properties of the current scale
  const ScaleProperties *scale = propertiesOfScaleType(_type);
  // use the high and low register inputs to transpose the note and shift
  //  the keys to create "virtual keys" in the high and low registers
  int baseKey = 0;
  float baseOffset = 0.0;
  if (_input->highRegister() != _input->lowRegister()) {
    // for asymetrical scales, we want each key to have the same pitch class
    //  in all registers, so we always shift by one octave
    float magnitude = 12.0;
    // for symmetrical scales with small repeats, we want the registers to 
    //  be contiguous with eachother
    if (scale->repeat <= 2) {
      magnitude = (float)(InputKeyCount + 1) * 
        (float)(12 / (scale->octave / scale->repeat));
    }
    // apply the offset using the correct sign
    if (_input->highRegister()) {
      baseKey += InputKeyCount + 1;
      baseOffset += magnitude;
    }
    else {
      baseKey -= InputKeyCount + 1;
      baseOffset -= magnitude;
    }
    while (baseKey < 0) baseKey += scale->octave;
  }
  // use the scale to make intervals from the tonic for the no-keys-pressed 
  //  state and each input key
  static float offsets[InputKeyCount + 1];
  float offset = baseOffset;
  key = baseKey;
  for (i = 0; i < InputKeyCount + 1; i++) {
    offsets[i] = offset;
    offset += scale->intervals[key % scale->repeat];
    key++;
  }
  // make a copy of key depressions, with the first entry being the 
  //  "open key" representing the no-keys-pressed state
  static float keys[InputKeyCount + 1];
  keys[0] = 0.0;
  float maxKey = 0.0;
  int keysDown = 0;
  float v;
  for (i = 0; i < InputKeyCount; i++) {
    v = _input->key(i);
    keys[i + 1] = v;
    if (v > 0.0) keysDown++;
    if (v > maxKey) maxKey = v;
  }
  // if only one key is down, scale the "open key" to be the inverse of it
  if (keysDown <= 1) {
    keys[0] = 1.0 - maxKey;
  }
  // use the key depressions as weights to compute a weighted average of 
  //  the offsets
  float offsetTotal = 0.0;
  float keyTotal = 0.0;
  for (i = 0; i < InputKeyCount + 1; i++) {
    v = keys[i];
    if (v > 0.0) offsetTotal += v * offsets[i];
    keyTotal += v;
  }
  float note = offsetTotal / keyTotal;
  // transpose to the selected key and center octave to get a MIDI note number
  note += 12.0 + ((float)_centerOctave * 12.0) + (float)_tonic;
  // convert to frequency in Hz
  _frequency = 440.0 * powf(2.0, (note - 69.0) / 12.0);
  // read amplitude from the breath input
  _amplitude = pow(_input->breath(), 1.5);
  // read modulation from the bite input
  _modulation = _input->bite();
}

float ScaleModel::frequency() { return(_frequency); }
float ScaleModel::amplitude() { return(_amplitude); }
float ScaleModel::modulation() { return(_modulation); }

