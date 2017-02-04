#ifndef LOOPER_AUDIO_H
#define LOOPER_AUDIO_H

#include <Audio.h>
#include <Wire.h>

// the number of bytes in one unit of audio
#define AUDIO_BLOCK_BYTES (AUDIO_BLOCK_SAMPLES * sizeof(int16_t))
// the ideal number of blocks to write to a file at one time
#define BLOCKS_PER_CHUNK (512 / AUDIO_BLOCK_BYTES)
// the number of blocks to allocate to buffer recording
#define RECORD_BUFFER_BLOCKS 96
// the number of blocks to allocate per track to buffer playback
#define PLAY_BUFFER_BLOCKS 8
// the total number of blocks to allocate
#define TOTAL_BLOCKS (RECORD_BUFFER_BLOCKS + (PLAY_BUFFER_BLOCKS * 4) + 16)
// the approximate number of blocks that plays in one second
#define BLOCKS_PER_SECOND (AUDIO_SAMPLE_RATE / AUDIO_BLOCK_SAMPLES)
// the threshold below which to consider the input silent
#define SILENCE_THRESHOLD 2000

typedef enum {
  InputSourceMic = 0,
  InputSourceLine,
  InputSourceUndefined
} InputSource;

class AudioDevice {
  public:
    AudioDevice() {
      AudioMemory(TOTAL_BLOCKS);
      _source = InputSourceUndefined;
      _audioControl = new AudioControlSGTL5000();
      _input = new AudioInputI2S();
      _output = new AudioOutputI2S();
      _mixer = new AudioMixer4();
      _mixerOutputConnection = new AudioConnection(*_mixer, 0, *_output, 1);
      for (int i = 0; i < 4; i++) _mixerInputConnection[i] = NULL;
      _peakAnalyzer = new AudioAnalyzePeak();
      _peakPatch = new AudioConnection(*_input, 1, *_peakAnalyzer, 0);
      _audioControl->enable();
      _micLevel = 32;
      _lineLevel = 5;
      _outputLevel = 2;
      _peak = 0.0;
      _integratedPeak = nan("");
    }
    // get/set the source for audio data
    InputSource source();
    void setSource(InputSource s);
    // get/set mic and line levels
    int micLevel();
    void setMicLevel(int level);
    int lineLevel();
    void setLineLevel(int level);
    int outputLevel();
    void setOutputLevel(int level);
    // update anything that needs to be checked periodically
    void update();
    // get the current peak level
    float peak();
    // connect a stream's output to the mixer
    void mix(AudioStream *stream, unsigned char channel);
    // get the input and output streams
    AudioStream *inputStream() { return(_input); }
    AudioStream *outputStream() { return(_output); }
  private:
    AudioControlSGTL5000 *_audioControl;
    AudioInputI2S *_input;
    AudioOutputI2S *_output;
    AudioAnalyzePeak *_peakAnalyzer;
    AudioConnection *_peakPatch; 
    InputSource _source;
    AudioMixer4 *_mixer;
    AudioConnection *_mixerInputConnection[4];
    AudioConnection *_mixerOutputConnection;
    int _micLevel;
    int _lineLevel;
    int _outputLevel;
    float _peak;
    float _integratedPeak;
};

#endif
