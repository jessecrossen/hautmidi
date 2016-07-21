#ifndef LOOPER_TRACK_H
#define LOOPER_TRACK_H

#include <Audio.h>
#include <SD.h>

#include "audio.h"

#define AUDIO_BLOCK_BYTES (AUDIO_BLOCK_SAMPLES * sizeof(int16_t))
#define RECORD_BUFFER_BLOCKS 50
#define PLAY_BUFFER_BLOCKS 4
#define MAX_BUFFER_BLOCKS RECORD_BUFFER_BLOCKS

typedef enum {
  Paused,
  Playing,
  MaybeRecording,
  Recording  
} TrackState;

class FileCache : protected AudioStream {
  public:
    FileCache() : AudioStream(0, NULL) {
      for (size_t i = 0; i < MAX_BUFFER_BLOCKS; i++) {
        _buffer[i] = NULL;
      }
      reset();
    };
    char *path() { return(_path); }
    void setPath(char *newPath) {
      if ((newPath != NULL) && (strcmp(newPath, _path) == 0)) return;
      reset();
      _path = newPath;
    }
    bool isOpen() { return((bool)_file); }
    virtual void update() { }
  protected:
    void reset() {
      if (_file) _file.close();
      _path = NULL;
      _head = _tail = _size = 0;
      for (size_t i = 0; i < MAX_BUFFER_BLOCKS; i++) {
        if (_buffer[i] != NULL) {
          AudioStream::release(_buffer[i]);
          _buffer[i] = NULL;
        }
      }
    }
  protected:
    char *_path;
    File _file;
    size_t _head;
    size_t _tail;
    size_t _size;
    audio_block_t * volatile _buffer[MAX_BUFFER_BLOCKS];
};

class RecordCache : public FileCache {
  public:
    bool open();
    bool writeBlock(audio_block_t *block);
    void flush();
    void emptyBuffer();
  private:
    size_t writeChunk();
};

class PlayCache : public FileCache {
  public:
    bool open();
    audio_block_t *readBlock();
    void fillBuffer();
  private:
    size_t readChunk();
};

class Track : public AudioStream {
  public:
    Track(AudioDevice *audio) 
          : AudioStream(1, _inputQueueArray) {
      _audio = audio;
      _path[0] = _pathA[0] = _pathB[0] = '\0';
      _state = Paused;
      _isActive = false;
      _scratch = new RecordCache();
      _master = new PlayCache();
      // connect to the audio device
      _inputConnection = 
        new AudioConnection(*_audio->inputStream(), 1, *this, 0);
      _outputConnection = 
        new AudioConnection(*this, 0, *_audio->outputStream(), 1);
    }
    // get/set the track's storage path
    char *path() { return(_path); }
    void setPath(char *path);
    // get/set the track's state
    TrackState state() { return(_state); }
    void setState(TrackState newState);
    bool isPlaying();
    bool isRecording();
    bool isOverdubbing();
    // get the number of milliseconds since the last state change
    unsigned int sinceStateChange() { return(_sinceStateChange); }
    // get/set whether the track is active
    bool isActive() { return(_isActive); }
    void setIsActive(bool v) { _isActive = v; }
    
    // update track caches
    void updateCaches();
    
    // handle audio streams into and out of the track
    virtual void update();
  
  private:
    TrackState _state;
    AudioDevice *_audio;
    AudioConnection *_inputConnection;
    AudioConnection *_outputConnection;
    bool _isActive;
    elapsedMillis _sinceStateChange;
    char _path[64];
    char _pathA[64];
    char _pathB[64];
    PlayCache *_master;
    RecordCache *_scratch;
    audio_block_t *_inputQueueArray[1];
};

#endif
