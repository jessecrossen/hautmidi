#ifndef LOOPER_TRACK_H
#define LOOPER_TRACK_H

#include <Audio.h>
#include <SD.h>

#include "audio.h"
#include "sync.h"

#define AUDIO_BLOCK_BYTES (AUDIO_BLOCK_SAMPLES * sizeof(int16_t))
#define RECORD_BUFFER_BLOCKS 50
#define PLAY_BUFFER_BLOCKS 4
#define MAX_BUFFER_BLOCKS RECORD_BUFFER_BLOCKS

// forward declaration for circular references
class Sync;

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
    size_t blocks() { return(_blocks); }
    size_t block() { return(_block); }
    virtual void update() { }
  protected:
    void reset() {
      if (_file) _file.close();
      _path = NULL;
      _blocks = 0;
      _block = 0;
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
    size_t _blocks;
    size_t _block;
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
    size_t playBlocks;
    size_t preroll;
  private:
    size_t readChunk();
};

class Track : public AudioStream {
  public:
    Track(AudioDevice *audio, Sync *sync) 
          : AudioStream(1, _inputQueueArray) {
      _audio = audio;
      _sync = sync;
      _path[0] = _pathA[0] = _pathB[0] = '\0';
      _state = Paused;
      _isActive = false;
      _scratch = new RecordCache();
      _master = new PlayCache();
      // set the time since last tap to a high value so the first tap
      //  won't trigger a spurious erasure
      sinceLastTap = 1000;
      // connect to the audio device
      _inputConnection = 
        new AudioConnection(*_audio->inputStream(), 1, *this, 0);
      audio->mix(this, 0);
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
    // the number of milliseconds since the last state change
    elapsedMillis sinceStateChange;
    // the number of milliseconds since the track's pedal was last tapped
    elapsedMillis sinceLastTap;
    // get/set whether the track is active
    bool isActive() { return(_isActive); }
    void setIsActive(bool v) { _isActive = v; }
    // erase the content on the track
    void erase();
    // return the length of the master track in blocks
    size_t masterBlocks();
    // return the indices of the current record/playback blocks
    size_t playingBlock();
    size_t recordingBlock();
    // return the number of blocks to play in the current loop
    size_t playBlocks();
    
    // update track caches
    void updateCaches();
    // set the track preroll before playback starts
    void updatePreroll();
    
    // handle audio streams into and out of the track
    virtual void update();
  
  private:
    TrackState _state;
    AudioDevice *_audio;
    AudioConnection *_inputConnection;
    bool _isActive;
    char _path[64];
    char _pathA[64];
    char _pathB[64];
    PlayCache *_master;
    RecordCache *_scratch;
    audio_block_t *_inputQueueArray[1];
    // a synchronizer for keeping tracks in sync
    Sync *_sync;
};

#endif
