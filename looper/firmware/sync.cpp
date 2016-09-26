#include "sync.h"

// get the index of the given track pointer
uint8_t Sync::_getTrackIndex(Track *track) {
  for (uint8_t i = 0; i < _trackCount; i++) {
    if (_tracks[i] == track) return(i);
  }
  return(0);
}

size_t Sync::blocksUntilNextSyncPoint(Track *track) {
  SyncPoint *p;
  uint8_t i = _getTrackIndex(track);
  // examine all sync points where this track could start its next loop
  size_t idealBlocks = track->masterBlocks();
  size_t minBlocks = idealBlocks / 4;
  if (minBlocks < 4) minBlocks = 4;
  size_t blocksUntil, targetBlock, targetRepeat;
  size_t bestLength = 0;
  size_t error = 0;
  size_t leastError = 0;
  for (p = _head; p; p = p->next) {
    if ((p->source != i) || (p->isProvisional)) continue;
    // get the number of blocks until this sync point will arrive
    targetBlock = _tracks[p->target]->playingBlock();
    targetRepeat = _tracks[p->target]->playBlocks();
    if (targetBlock <= (p->time - minBlocks)) blocksUntil = p->time - targetBlock;
    else blocksUntil = (p->time + targetRepeat) - targetBlock;
    // avoid extremely short repeats, which may indicate a timing miss
    //  on the current loop of the track
    if (blocksUntil < minBlocks) continue;
    // find the sync point closest to the natural length of the track
    error = (idealBlocks > blocksUntil) ? 
      (idealBlocks - blocksUntil) : (blocksUntil - idealBlocks);
    if ((bestLength == 0) || (error < leastError)) {
      leastError = error;
      bestLength = blocksUntil;
    }
  }
  return(bestLength);
}

size_t Sync::trackStarting(Track *track) {
  SyncPoint *p;
  uint8_t si = _getTrackIndex(track);
  // if any other track is recording, add a provisional sync point to it
  for (uint8_t i = 0; i < _trackCount; i++) {
    if ((i != si) && (_tracks[i]->isRecording())) {
      p = new SyncPoint;
      p->source = si;
      p->target = i;
      p->time = _tracks[i]->recordingBlock();
      p->isProvisional = true;
      _addPoint(p);
    }
  }
  size_t bestLength = blocksUntilNextSyncPoint(track);
  
  Serial.print(si);
  Serial.print(" ");
  Serial.println(bestLength);
  
  // if we found no matching sync point, just repeat at the natural length
  if (bestLength == 0) return(track->masterBlocks());
  // otherwise return our best match
  return(bestLength);
}

void Sync::trackRecording(Track *track) {
  SyncPoint *p;
  uint8_t ri = _getTrackIndex(track);
  // add a sync point onto any other playing tracks
  for (uint8_t i = 0; i < _trackCount; i++) {
    if ((i != ri) && (_tracks[i]->isPlaying())) {
      p = new SyncPoint;
      p->source = ri;
      p->target = i;
      p->time = _tracks[i]->playingBlock();
      p->isProvisional = true;
      _addPoint(p);
    }
  }
}

void Sync::cancelRecording(Track *track) {
  // remove all provisional sync points
  SyncPoint *p = _head;
  while (p) {
    if (p->isProvisional) p = _removePoint(p);
    else p = p->next;
  }
}

void Sync::commitRecording(Track *track) {
  uint8_t i = _getTrackIndex(track);
  // remove all old sync points involving the given track and 
  //  promote all provisional ones
  SyncPoint *p = _head;
  while (p) {
    // skip sync points not involved with this track
    if ((p->source != i) && (p->target != i)) {
      p = p->next;
    }
    // promote formerly provisional new sync points
    else if (p->isProvisional) {
      p->isProvisional = false;
      p = p->next;
    }
    // remove existing sync points involving this track
    else {
      p = _removePoint(p);
    }
  }
  // save the changed sync points
  save();
}

void Sync::trackErased(Track *track) {
  uint8_t i = _getTrackIndex(track);
  // remove all sync points involving the erased track
  SyncPoint *p = _head;
  while (p) {
    if ((p->source == i) || (p->target == i)) p = _removePoint(p);
    else p = p->next;
  }
}

// DOUBLY-LINKED LIST *********************************************************

void Sync::_addPoint(SyncPoint *p) {
  if (_tail == NULL) {
    _head = _tail = p;
    p->next = p->prev = NULL;
  }
  else {
    _tail->next = p;
    p->prev = _tail;
    _tail = p;
    p->next = NULL;
  }
}

SyncPoint *Sync::_removePoint(SyncPoint *p) {
  SyncPoint *next = p->next;
  if (p->prev) p->prev->next = p->next;
  else _head = p->next;
  if (p->next) p->next->prev = p->prev;
  else _tail = p->prev;
  delete p;
  return(next);
}

void Sync::_removeAllPoints() {
  SyncPoint *p = _head;
  SyncPoint *next;
  while (p) {
    next = p->next;
    delete p;
    p = next;
  }
  _head = _tail = NULL;
}

// PERSISTENCE ****************************************************************

void Sync::setPath(char *path) {
  if (path == NULL) return;
  if (strncmp(path, _path, sizeof(_path)) == 0) return;
  strncpy(_path, path, sizeof(_path));
  // remove existing sync points
  _removeAllPoints();
  // load sync points from the path
  if ((_path[0] == '\0') || (! SD.exists(_path))) return;
  File f = SD.open(_path, O_READ);
  if (! f) return;
  byte buffer[2 + sizeof(size_t)];
  size_t *time = (size_t *)(buffer + 2);
  size_t bytesRead;
  SyncPoint *p;
  while (true) {
    bytesRead = f.read(buffer, sizeof(buffer));
    if (! (bytesRead >= sizeof(buffer))) return;
    p = new SyncPoint;
    p->source = buffer[0] % _trackCount;
    p->target = buffer[1] % _trackCount;
    p->time = *time;
    p->isProvisional = false;
    _addPoint(p);
  }
  f.close();
}

void Sync::save() {
  // save sync points to the current path
  if (_path[0] == '\0') return;
  File f = SD.open(_path, O_WRITE | O_CREAT | O_TRUNC);
  if (! f) return;
  byte buffer[2 + sizeof(size_t)];
  size_t *time = (size_t *)(buffer + 2);
  for (SyncPoint *p = _head; p; p = p->next) {
    buffer[0] = p->source;
    buffer[1] = p->target;
    *time = p->time;
    f.write(buffer, sizeof(buffer));
  }
  f.close();
}
