#ifndef LOOPER_TRACK_H
#define LOOPER_TRACK_H

#include <Audio.h>
#include <SD.h>

#include "audio.h"
#include "sync.h"
#include "trace.h"

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
    FileCache() : AudioStream(0, NULL) { }
    virtual void reset() { }
    char *path() { return(_path); }
    void setPath(char *newPath) {
      if ((newPath != NULL) && (strcmp(newPath, _path) == 0)) {
        WARN2("FileCache::setPath path is NULL or unchanged", newPath);
        return;
      }
      if (_file) _file.close();
      reset();
      _path = newPath;
      DBG2("FileCache::setPath", _path);
    }
    bool isOpen() { return((bool)_file); }
    virtual void update() { }
  protected:
    char *_path;
    File _file;
};

class RecordCache : public FileCache {
  public:
    RecordCache() : FileCache() {
      for (size_t i = 0; i < RECORD_BUFFER_BLOCKS; i++) _buffer[i] = NULL;
      reset();
    };
    virtual void reset();
    bool open();
    bool writeBlock(audio_block_t *block);
    void flush();
    void emptyBuffer();
    size_t blocks() { return(_blocks); }
  protected:
    size_t _head, _tail, _size, _blocks;
    audio_block_t * volatile _buffer[RECORD_BUFFER_BLOCKS];
    size_t writeChunk();
};

typedef struct {
  size_t seq;
  audio_block_t * volatile block;  
} PlayBlock;

class PlayCache : public FileCache {
  public:
    PlayCache() : FileCache() {
      for (size_t i = 0; i < PLAY_BUFFER_BLOCKS; i++) _buffer[i].block = NULL;
      reset();
    };
    virtual void reset();
    bool open();
    audio_block_t *readBlock();
    void fillBuffer();
    size_t playBlocks;
    size_t preroll;
    bool isEmpty() { return((_blocks > 0) && (_size == 0)); }
    bool isFull() { return(_size >= PLAY_BUFFER_BLOCKS); }
    size_t blocks() { return(_file ? _blocks : 0); }
    size_t seq() { return(_seq); }
  protected:
    size_t _head, _tail, _size, _blocks, _seq;
    PlayBlock _buffer[PLAY_BUFFER_BLOCKS];
    void readChunk();
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
      _isPassthru = false;
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
    // get/set whether the track should pass its input through
    bool isPassthru() { return(_isPassthru); }
    void setIsPassthru(bool v) { _isPassthru = v; }
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
    // return whether the track cache is empty/full
    bool isPlaybackCacheEmpty();
    bool isPlaybackCacheFull();
    // set the track preroll before playback starts
    void updatePreroll();
    
    // handle audio streams into and out of the track
    virtual void update();
    
    // the index of the track in the list
    size_t index;
  
  private:
    TrackState _state;
    AudioDevice *_audio;
    AudioConnection *_inputConnection;
    bool _isActive;
    bool _isPassthru;
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
