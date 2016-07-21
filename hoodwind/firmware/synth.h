#ifndef _HOODWIND_synth_h_
#define _HOODWIND_synth_h_

#include <Audio.h>

#include "mvc/model.h"
#include "scale.h"

class ISynthStrategy {
  public:
    virtual void fillBlock(audio_block_t *block, ScaleModel *scale) = 0;
};

class WaveformSynthStrategy : public ISynthStrategy {
  public:
    WaveformSynthStrategy() {
      phase = 0;
      in1 = in2 = in3 = in4 = 0;
      out1 = out2 = out3 = out4 = 0;
    }
    virtual void fillBlock(audio_block_t *block, ScaleModel *scale);
  protected:
    void filterBlock(audio_block_t *block, float cutoff, float Q);
  private:
    uint32_t phase;
    int in1, in2, in3, in4;
    int out1, out2, out3, out4;
};

class SynthModel : public Model, public AudioStream {
  public:
    SynthModel(ScaleModel *scale);
    
    // act as an audio stream
    virtual void update(void);
    
  private:
    ScaleModel *_scale;
    AudioControlSGTL5000 *_control;
    AudioOutputI2S *_output;
    AudioConnection *_synthToLeft;
    AudioConnection *_synthToRight;
    ISynthStrategy *_strategy;
};

#endif
