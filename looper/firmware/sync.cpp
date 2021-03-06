#include "sync.h"

#define TRACE 0
#include "trace.h"

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
  size_t blocksUntil, targetBlock, targetRepeat, time;
  size_t bestLength = 0;
  size_t error = 0;
  size_t leastError = 0;
  for (p = _head; p; p = p->next) {
    if ((p->source != i) || (p->isProvisional)) continue;
    // get the number of blocks until this sync point will arrive
    targetBlock = _tracks[p->target]->playingBlock();
    targetRepeat = _tracks[p->target]->playBlocks();
    if (targetRepeat == 0) {
      WARN1("Sync::blocksUntilNextSyncPoint repeat is zero");
      continue;
    }
    time = p->time;
    if (time >= targetRepeat) {
      // if the target timepoint will not be played in this cycle, 
      //  try to sync up with the loop after it's been restarted
      size_t untilOriginalLoop = _tracks[p->target]->masterBlocks() - time;
      if (untilOriginalLoop < targetRepeat) {
        time = targetRepeat - untilOriginalLoop;
      }
    }
    time += targetRepeat;
    if (targetBlock >= time) {
      WARN3("Sync::blocksUntilNextSyncPoint time overflow", targetBlock, time);
      continue;
    }
    blocksUntil = (time - targetBlock) % targetRepeat;
    // find the sync point closest to the natural length of the track
    while (blocksUntil <= idealBlocks + targetRepeat) {
      error = (idealBlocks > blocksUntil) ? 
        (idealBlocks - blocksUntil) : (blocksUntil - idealBlocks);
      if ((bestLength == 0) || (error < leastError)) {
        leastError = error;
        bestLength = blocksUntil;
      }
      blocksUntil += targetRepeat;
    }
  }
  return(bestLength > minBlocks ? bestLength : idealBlocks);
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

void Sync::setInitialPreroll(Track *track) {
  track->setPreroll(_prerolls[track->index]);
}

void Sync::_computePrerolls(size_t startTimes[MAX_TRACKS]) {
  int i;
  // get the length of the longest track in blocks
  size_t maxBlocks = 0;
  for (i = 0; i < _trackCount; i++) {
    if (_tracks[i]->masterBlocks() > maxBlocks) maxBlocks = _tracks[i]->masterBlocks();
  }
  // offset start times to get a set of prerolls
  for (i = 0; i < _trackCount; i++) {
    if (startTimes[i] > maxBlocks) _prerolls[i] = 0;
    else _prerolls[i] = maxBlocks - startTimes[i];
  }
  // reduce such that the minimum preroll is zero
  size_t minBlocks = maxBlocks;
  for (i = 0; i < _trackCount; i++) {
    if ((_tracks[i]->masterBlocks() > 0) && (_prerolls[i] < minBlocks))
      minBlocks = _prerolls[i];
  }
  for (i = 0; i < _trackCount; i++) {
    if (_prerolls[i] >= minBlocks) _prerolls[i] -= minBlocks;
  }
}

// DOUBLY-LINKED LIST ********************************************************

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
  // reset block starts
  for (int i = 0; i < _trackCount; i++) {
    _prerolls[i] = 0;
  }
  // load sync points from the path
  if ((_path[0] == '\0') || (! SD.exists(_path))) return;
  INFO2("Sync::setPath", _path);
  File f = SD.open(_path, O_READ);
  if (! f) {
    WARN2("Sync::setPath unable to open", _path);
    return;
  }
  byte buffer[2 + sizeof(size_t)];
  size_t *time = (size_t *)buffer;
  size_t bytesRead;
  // read track starting times
  size_t startTimes[MAX_TRACKS];
  for (int i = 0; i < _trackCount; i++) {
    bytesRead = f.read(buffer, sizeof(size_t));
    if (! (bytesRead >= sizeof(size_t))) return;
    startTimes[i] = *time;
  }
  // read sync points
  time = (size_t *)(buffer + 2);
  SyncPoint *p;
  while (true) {
    bytesRead = f.read(buffer, sizeof(buffer));
    if (! (bytesRead >= sizeof(buffer))) return;
    p = new SyncPoint;
    p->source = buffer[0] % _trackCount;
    p->target = buffer[1] % _trackCount;
    p->time = *time;
    p->isProvisional = false;
    DBG4("Sync::setPath point", p->source, p->target, p->time);
    _addPoint(p);
  }
  f.close();
  // calculate the preroll for all tracks
  _computePrerolls(startTimes);
}

void Sync::save() {
  // save sync points to the current path
  if (_path[0] == '\0') return;
  File f = SD.open(_path, O_WRITE | O_CREAT | O_TRUNC);
  if (! f) {
    WARN2("Sync::save unable to open", _path);
    return;
  }
  // write track starting times
  byte buffer[2 + sizeof(size_t)];
  size_t *time = (size_t *)buffer;
  for (int i = 0; i < _trackCount; i++) {
    *time = _tracks[i]->playingBlock();
    f.write(buffer, sizeof(size_t));
  }
  // write sync points
  time = (size_t *)(buffer + 2);
  for (SyncPoint *p = _head; p; p = p->next) {
    buffer[0] = p->source;
    buffer[1] = p->target;
    *time = p->time;
    DBG4("Sync::save point", p->source, p->target, p->time);
    f.write(buffer, sizeof(buffer));
  }
  f.close();
}
