#ifndef LOOPER_SYNC_H
#define LOOPER_SYNC_H

#include "track.h"

// forward-declare Track because of circular references
class Track;



// define a doubly-linked list entry for syncing one track with another
typedef struct SyncPointStruct {
  // the index of the track which should just be starting playback at this point
  uint8_t source;
  // the index of the track that is already playing/recording at this point
  uint8_t target;
  // the block index of the current timepoint in the playing track
  size_t time;
  // whether the timepoint is not yet committed
  bool isProvisional;
  // doubly-linked list
  SyncPointStruct *prev;
  SyncPointStruct *next;
} SyncPoint;

class Sync {
  public:
    Sync(Track **tracks, int trackCount) {
      _head = _tail = NULL;
      _path[0] = '\0';
      _tracks = tracks;
      _trackCount = trackCount;
    }
    // get the ideal number of blocks that should be played for a track
    size_t idealLoopBlocks(Track *track);
    // get the number of blocks until the track's next sync point, 
    //  or zero if no other track provides one within its next loop
    size_t blocksUntilNextSyncPoint(Track *track, size_t idealBlocks);
    // register that a track is beginning playback and return the number of 
    //  blocks it should advance before looping
    size_t trackStarting(Track *track);
    // register that a track is beginning recording
    void trackRecording(Track *track);
    // cancel or commit changes from recording a track
    void cancelRecording(Track *track);
    void commitRecording(Track *track);
    // erase sync points for a track
    void trackErased(Track *track);
    
    // set the path to persist sync points to
    char *path() { return(_path); }
    void setPath(char *newPath);
    void save();
    
  private:
    SyncPoint *_head;
    SyncPoint *_tail;
    Track **_tracks;
    int _trackCount;
    char _path[64];

    uint8_t _getTrackIndex(Track *track);
    
    void _addPoint(SyncPoint *p);
    SyncPoint *_removePoint(SyncPoint *p);
    void _removeAllPoints();

};

#endif
