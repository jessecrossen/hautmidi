#include "synth.h"


#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define SMAX 0x3FFF

void WaveformSynthStrategy::fillBlock(audio_block_t *block, ScaleModel *scale) {
  short *bp = block->data;
  
  uint32_t step = (scale->frequency() / AUDIO_SAMPLE_RATE_EXACT) * (float)0xFFFFFFFF;
  //uint32_t pulsewidth = ((1.0 - scale->modulation()) * (float)0xFFFFFFFF) / 2.0;
  short amplitude = SMAX * scale->modulation(); //!!! scale->amplitude();
  
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    if (phase < 0x80000000) *bp++ = -amplitude;
    else *bp++ = amplitude;
    phase += step;
  }
  
  //filterBlock(block, (1.0 - scale->modulation()), 1.0);
}

void WaveformSynthStrategy::filterBlock(audio_block_t *block, float cutoff, float Q) {
  short *bp = block->data;
  
  // precompute factors
  int f = (cutoff * 1.16) * SMAX;
  int f2 = (f * f) >> 15;
  int f4 = (0x1668 * ((f2 * f2) >> 15)) >> 15;
  int fi = SMAX - f;
  int fb = ((int)(Q * SMAX) * (SMAX - ((0x999 * f2) >> 15))) >> 15;
  int v;
  
  // modify samples
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    v = *bp;
    v -= (out4 * fb) / 0x3FFF;
    v = (v * f4) / 0x3FFF;
    // pole 1
    out1 = v + ((0x1333 * in1) / 0x3FFF) + ((fi * out1) / 0x3FFF);
    in1 = v;
    // pole 2
    out2 = out1 + ((0x1333 * in2) / 0x3FFF) + ((fi * out2) / 0x3FFF);
    in2 = out1;
    // pole 3
    out3 = out2 + ((0x1333 * in3) / 0x3FFF) + ((fi * out3) / 0x3FFF);
    in3 = out2;
    // pole 4
    out4 = out3 + ((0x1333 * in4) / 0x3FFF) + ((fi * out4) / 0x3FFF);
    in4 = out3;
    // clip with a band-limited sigmoid
    out4 -= (((((out4 * out4) / 0x3FFF) * out4) / 0x3FFF) * 0xAAB) / 0x3FFF;
    *bp++ = (short)out4;
  }
}

SynthModel::SynthModel(ScaleModel *scale) : Model(), AudioStream(0, NULL) {
  _scale = scale;
  // start up audio control
  AudioMemory(60);
  _control = new AudioControlSGTL5000();
  _output = new AudioOutputI2S();
  _control->enable();
  _control->volume(0.8);
  _synthToLeft = new AudioConnection(*this, 0, *_output, 0);
  _synthToRight = new AudioConnection(*this, 0, *_output, 1);
  _strategy = new WaveformSynthStrategy();
}

void SynthModel::update() {
  audio_block_t *block = allocate();
  if (block) {
    if (_strategy) _strategy->fillBlock(block, _scale);
    transmit(block, 0);
    release(block);
  }
}
