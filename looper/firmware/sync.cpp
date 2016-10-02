#include "sync.h"

size_t Sync::idealLoopBlocks(Track *track) {
  SyncPoint *p;
  uint8_t ti;
  uint8_t i = track->index;
  // count sync points to the track for each other track
  size_t *counts = new size_t[_trackCount];
  for (ti = 0; ti < _trackCount; ti++) counts[ti] = 0;
  for (p = _head; p; p = p->next) {
    if ((p->target != i) || (p->isProvisional) || 
        (p->source >= _trackCount)) continue;
    counts[p->source]++;
  }
  // for all tracks with more than one reference to this one, 
  //  see if we can loop at an even multiple of the source track's length
  size_t idealBlocks = track->masterBlocks();
  size_t bestLength = 0;
  size_t leastError = 0;
  size_t unit, count, multiple, target, error, maxError;
  for (ti = 0; ti < _trackCount; ti++) {
    count = counts[ti];
    if (count < 2) continue;
    unit = _tracks[ti]->masterBlocks();
    maxError = unit / 4;
    for (multiple = count - 1; multiple <= count + 1; multiple++) {
      target = unit * multiple;
      error = (idealBlocks > target) ? 
                (idealBlocks - target) : (target - idealBlocks);
      if (error > maxError) continue;
      if ((bestLength == 0) || (error < leastError)) {
        leastError = error;
        bestLength = target;
      }
    }
  }
  // clean up dynamically allocated memory
  delete[] counts;
  // if no acceptable match was found, return the track's natural length
  if (bestLength == 0) return(idealBlocks);
  // otherwise return our best match
  return(bestLength);
}

size_t Sync::blocksUntilNextSyncPoint(Track *track, size_t idealBlocks) {
  SyncPoint *p;
  uint8_t i = track->index;
  // examine all sync points where this track could start its next loop
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
  uint8_t si = track->index;
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
  size_t idealBlocks = idealLoopBlocks(track);
  size_t bestLength = blocksUntilNextSyncPoint(track, idealBlocks);
  // if we found no matching sync point, just repeat at the natural length
  if (bestLength == 0) return(idealBlocks);
  // otherwise return our best match
  return(bestLength);
}

void Sync::trackRecording(Track *track) {
  SyncPoint *p;
  uint8_t ri = track->index;
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
  uint8_t i = track->index;
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
  uint8_t i = track->index;
  // remove all sync points involving the erased track
  SyncPoint *p = _head;
  while (p) {
    if ((p->source == i) || (p->target == i)) p = _removePoint(p);
    else p = p->next;
  }
  // save the changed sync points
  save();
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
