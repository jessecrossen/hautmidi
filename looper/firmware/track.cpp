#include "track.h"

#include <string.h>
#include <math.h>

#include "audio.h"

#define TRACE 1
#include "trace.h"

// TRACK **********************************************************************

void Track::setPath(char *path) {
  if (path == NULL) return;
  if (strncmp(path, _path, sizeof(_path)) == 0) return;
  strncpy(_path, path, sizeof(_path));
  INFO2("Track::setPath", _path);
  // delete the old scratch path to avoid confusion 
  //  when we load this loop again
  char *oldScratchPath = NULL;
  if (_scratch->isOpen()) {
    oldScratchPath = _scratch->path();
  }
  // close open file entries
  _scratch->setPath(NULL);
  _master->setPath(NULL);
  if (oldScratchPath != NULL) SD.remove(oldScratchPath);
  // make interchangeable paths for the scratch and master audio files
  snprintf(_pathA, sizeof(_pathA), "%s.A", _path);
  snprintf(_pathB, sizeof(_pathB), "%s.B", _path);
  // whichever file exists and has more bytes should be the master
  size_t sizeA = 0, sizeB = 0;
  File f;
  if (SD.exists(_pathA)) {
    f = SD.open(_pathA, O_READ);
    sizeA = f.size();
    f.close();
  }
  if (SD.exists(_pathB)) {
    f = SD.open(_pathB, O_READ);
    sizeB = f.size();
    f.close();
  }
  INFO3("Track::setPath", _pathA, sizeA);
  INFO3("Track::setPath", _pathB, sizeB);
  if (sizeB > sizeA) {
    _master->setPath(_pathB);
    _scratch->setPath(_pathA);
  }
  else {
    _master->setPath(_pathA);
    _scratch->setPath(_pathB);
  }
  // open the files
  _master->open();
  _scratch->open();
  // pause and deactivate the track when its path changes
  setIsActive(false);
  setState(Paused);
}

void Track::setState(TrackState newState) {
  char *temp;
  TrackState oldState = _state;
  if (newState == oldState) return;
  bool wasRecording = isRecording();
  bool willBeRecording = (newState == MaybeRecording) || (newState == Recording);
  // open the scratch track before recording starts
  if ((willBeRecording) && (! wasRecording)) _scratch->open();
  _state = newState;
  sinceStateChange = 0;
  // when cancelled recording stops...
  if ((! isRecording()) && (oldState == MaybeRecording)) {
    _sync->cancelRecording(this);
    // reset the scratch file to the beginning
    _scratch->reset();
	}
  // when true recording stops...
  else if ((! isRecording()) && (oldState == Recording)) {
    // flush recorded audio
    _scratch->flush();
	  // swap the scratch and master files
	  temp = _master->path();
	  _master->setPath(_scratch->path());
	  _scratch->setPath(temp);
	  // remove the new scratch file (old master file)
	  SD.remove(_scratch->path());
	  // commit the sync points for the new recording
	  _sync->commitRecording(this);
	  _master->open();
	  // update the preroll in case the recording should not start immediately
	  updatePreroll();
  }
  // pre-cache the playback track if one exists
  _master->open();
  _master->fillBuffer();
}
bool Track::isPlaying() {
  return(((_state == Playing) || (isRecording())) && (_master));
}
bool Track::isRecording() {
  return((_state == MaybeRecording) || (_state == Recording));
}
bool Track::isOverdubbing() {
  return(isRecording() && isPlaying());
}

void Track::erase() {
  INFO2("Track::erase", _path);
  setState(Paused);
  _master->setPath(NULL);
  _scratch->setPath(NULL);
  if (SD.exists(_pathA)) SD.remove(_pathA);
  if (SD.exists(_pathB)) SD.remove(_pathB);
  _master->setPath(_pathA);
  _scratch->setPath(_pathB);
  _sync->trackErased(this);
}

size_t Track::masterBlocks() { return(_master->blocks()); }
size_t Track::playingBlock() { return(_master->seq()); }
size_t Track::recordingBlock() { return(_scratch->blocks()); }
size_t Track::playBlocks() { return(_master->playBlocks); }

void Track::updateCaches() {
  if (_master->isOpen()) _master->fillBuffer();
  if ((isRecording()) && (_scratch->isOpen())) _scratch->emptyBuffer();
}

bool Track::isPlaybackCacheEmpty() {
  if (! _master->isOpen()) return(false);
  return(_master->isEmpty());
}
bool Track::isPlaybackCacheFull() {
  if (! _master->isOpen()) return(true);
  return(_master->isFull());
}

void Track::updatePreroll() {
  if (_master->isOpen()) {
    _master->preroll = _sync->blocksUntilNextSyncPoint(this, _master->blocks());
  }
}
void Track::setPreroll(size_t newPreroll) {
  if (_master->isOpen()) {
    _master->preroll = newPreroll;
  }
}

void Track::update() {
  // check state
  bool needsRecord = isRecording();
  bool needsPlayback = isPlaying();
  bool needsInput = needsRecord || _isPassthru;
  bool needsOutput = needsRecord || needsPlayback || _isPassthru;
  // get input
  audio_block_t *inBlock = NULL;
  if (needsInput) {
    inBlock = receiveReadOnly();
    if (inBlock == NULL) {
      WARN2("Track::update no input available", index);
    }
  }
  // get output
  size_t beginSeq = _master->seq();
  audio_block_t *outBlock = _master->readBlock();
  if (! needsPlayback) {
    release(outBlock);
    outBlock = NULL;
  }
  // recompute the track's loop length once the first block is played
  if ((beginSeq == 0) && (_master->seq() != beginSeq)) {
    _master->playBlocks = _sync->trackStarting(this);
    INFO3("Track::update starting loop", index, _master->playBlocks);
  }
  // if we have nothing to send to output, we're done
  if (! needsOutput) {
    if (inBlock) release(inBlock);
    if (outBlock) release(outBlock);  
    return;
  }
  // handle the beginning of recording
  if ((needsRecord) && (_scratch->blocks() == 0)) {
    // omit silence when first recording to a track, but not when overdubbing
    if ((inBlock) && (! outBlock)) {
      int16_t *sample = inBlock->data;
      bool isSilent = true;
      for (size_t i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        if (abs(*sample++) > SILENCE_THRESHOLD) {
          isSilent = false;
          break;
        }
      }
      if (isSilent) {
        release(inBlock);
        return;
      }
    }
    // mark when recording actually starts
    _sync->trackRecording(this);
    INFO2("Track::update starting record", index);
  }
  // mix input and output
  if (inBlock) {
	  if (outBlock) {
	    int16_t *inSample = inBlock->data;
	    int16_t *outSample = outBlock->data;
	    for (size_t i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
	      *outSample += *inSample;
	      outSample++; inSample++;
	    }
	    release(inBlock);
	  }
	  else outBlock = inBlock;
  }
  if (outBlock) {
    // record using a copy of the output block
    if (needsRecord) {
      audio_block_t *recordBlock = allocate();
      if (recordBlock == NULL) {
        WARN1("Track::update no block to record to");
      }
      else {
        memcpy(recordBlock->data, outBlock->data, AUDIO_BLOCK_BYTES);
        _scratch->writeBlock(recordBlock);
      }
    }
    // send output to the mixer
    transmit(outBlock, 0);
    release(outBlock);
  }
}

// RECORDING CACHE ************************************************************

void RecordCache::reset() {
  if (_file) _file.seek(0);
  _path = NULL;
  _blocks = 0;
  _head = _tail = _size = 0;
  for (size_t i = 0; i < RECORD_BUFFER_BLOCKS; i++) {
    if (_buffer[i] != NULL) {
      AudioStream::release(_buffer[i]);
      _buffer[i] = NULL;
    }
  }
}

bool RecordCache::open() {
  if (_path == NULL) return(false);
  if (! _file) _file = SD.open(_path, O_WRITE | O_CREAT | O_TRUNC);
  if (! _file) return(false);
  return(true);
}

bool RecordCache::writeBlock(audio_block_t *block) {
  // if the buffer is full, we have to drop the block
  if (_size >= RECORD_BUFFER_BLOCKS) {
    WARN1("RecordCache::writeBlock buffer overflow");
    release(block);
    return(false);
  }
  _buffer[_tail] = block;
  _tail++; _size++;
  if (_tail >= RECORD_BUFFER_BLOCKS) _tail = 0;
  _blocks++;
  return(true);
}

void RecordCache::flush() {
  while (writeChunk() > 0) { }
}

void RecordCache::emptyBuffer() {
  if (_size < BLOCKS_PER_CHUNK) return;
  writeChunk();
}

size_t RecordCache::writeChunk() {
  static byte chunkBuffer[512];
  if ((! open()) || (_size == 0)) return(0);
  size_t bytes = 0;
  audio_block_t *block;
  for (size_t i = 0; (i < BLOCKS_PER_CHUNK) && (_size > 0); i++) {
    block = _buffer[_head];
    memcpy(chunkBuffer + (i * AUDIO_BLOCK_BYTES), block->data, AUDIO_BLOCK_BYTES);
    bytes += AUDIO_BLOCK_BYTES;
    release(block);
    _buffer[_head] = NULL;
    _head++;
    _size--;
    if (_head >= RECORD_BUFFER_BLOCKS) _head = 0;
  }
  return(_file.write(chunkBuffer, bytes));
}

// PLAYBACK CACHE *************************************************************

void PlayCache::reset() {
  if (_file) _file.seek(0);
  _path = NULL;
  _head = _tail = _size = _blocks = _seq = 0;
  for (size_t i = 0; i < PLAY_BUFFER_BLOCKS; i++) {
    if (_buffer[i].block != NULL) {
      AudioStream::release(_buffer[i].block);
      _buffer[i].block = NULL;
    }
  }
}

bool PlayCache::open() {
  if (_path == NULL) {
    WARN1("PlayCache::open has no path");
    return(false);
  }
  if (! _file) {
    _file = SD.open(_path, O_READ);
    size_t bytes = _file ? _file.size() : 0;
    INFO3("PlayCache::open", _path, bytes);
    // guard against small files, which could cause a tight loop
    if (bytes < AUDIO_BLOCK_BYTES) {
      WARN3("PlayCache::open has no data", _path, bytes);
      _file.close();
      SD.remove(_path);
      return(false);
    }
    _blocks = bytes / AUDIO_BLOCK_BYTES;
    playBlocks = _blocks;
    preroll = 0;
    DBG3("PlayCache::open", _blocks, "blocks");
  }
  return(true);
}

audio_block_t *PlayCache::readBlock() {
  // if the file isn't open, we can't be caching anything
  if (! _file) return(NULL);
  // if we're still in the preroll, there's nothing to play
  if (preroll > 0) {
    preroll--;
    return(NULL);
  }
  // see if we need a block from the range in the loop
  audio_block_t *block = NULL;
  if (_seq < _blocks) {
    while ((_size > 0) && (block == NULL)) {
      if (_buffer[_head].seq == _seq) {
        block = _buffer[_head].block;
      }
      else {
        release(_buffer[_head].block);
      }
      _buffer[_head].block = NULL;
      _head = (_head + 1) % PLAY_BUFFER_BLOCKS;
      _size--;
    }
  }
  // advance the sequence number
  _seq = (_seq + 1) % playBlocks;
  return(block);
}

void PlayCache::fillBuffer() {
  // only read a chunk if we have enough space to use all of it
  if (_size + BLOCKS_PER_CHUNK > PLAY_BUFFER_BLOCKS) return;
  readChunk();
}

void PlayCache::readChunk() {
  size_t i;
  static byte chunkBuffer[512];
  static bool isOffsetCached[PLAY_BUFFER_BLOCKS];
  if (! open()) return;
  if (_size >= PLAY_BUFFER_BLOCKS) return;
  // get the maximum number of blocks to be played from the loop
  size_t loopBlocks = _blocks;
  if (playBlocks < loopBlocks) loopBlocks = playBlocks;
  // the sequence position we should be caching
  size_t seqNeeded = _seq;
  // if the required sequence is past the section to play, 
  //  we'll need blocks starting at the beginning when the track loops
  if (seqNeeded >= loopBlocks) seqNeeded = 0;
  // check whether the next buffer-full of blocks is cached
  size_t seqAvailable;
  for (i = 0; i < PLAY_BUFFER_BLOCKS; i++) isOffsetCached[i] = false;
  for (i = 0; i < PLAY_BUFFER_BLOCKS; i++) {
    if (_buffer[i].block == NULL) continue;
    seqAvailable = _buffer[i].seq;
    if (seqAvailable >= loopBlocks) continue;
    if (seqAvailable < seqNeeded) seqAvailable += loopBlocks;
    if ((seqAvailable >= seqNeeded) && 
        (seqAvailable < seqNeeded + PLAY_BUFFER_BLOCKS)) {
      isOffsetCached[seqAvailable - seqNeeded] = true;
    }
  }
  // get the first block we'll need in the future that is not yet cached
  for (i = 0; i < PLAY_BUFFER_BLOCKS; i++) {
    if (! isOffsetCached[i]) {
      seqNeeded = (seqNeeded + i) % loopBlocks;
      break;
    }
  }
  // get our current sequence position in the file
  size_t seqInFile = _file.position() / AUDIO_BLOCK_BYTES;
  // if we can't get our desired chunk from this position, we need to seek
  if (seqNeeded - seqInFile >= BLOCKS_PER_CHUNK) {
    DBG3("PlayCache::readChunk seek miss", seqNeeded, seqInFile);
    // always read in aligned 512-byte blocks for speed
    size_t seekPos = seqNeeded * AUDIO_BLOCK_BYTES;
    seekPos -= (seekPos % 512);
    _file.seek(seekPos);
    seqInFile = _file.position() / AUDIO_BLOCK_BYTES;
    DBG3("PlayCache::readChunk did seek", seqNeeded, seqInFile);
  }
  // read a chunk from the file
  size_t readBytes = _file.read(chunkBuffer, 512);
  if (readBytes < 512) {
    DBG2("PlayCache::readChunk underflow", readBytes);
  }
  // get the needed block from the read buffer
  audio_block_t *block;
  size_t maxOffset = readBytes - AUDIO_BLOCK_BYTES;
  size_t offset = (seqNeeded - seqInFile) * AUDIO_BLOCK_BYTES;
  for (; offset <= maxOffset; offset += AUDIO_BLOCK_BYTES) {
    if ((_size >= PLAY_BUFFER_BLOCKS) || (seqNeeded >= loopBlocks)) break;
    block = allocate();
    memcpy(block->data, chunkBuffer + offset, AUDIO_BLOCK_BYTES);
    // fade the head/tail of the first/last blocks to avoid a click
    if ((seqNeeded == 0) || (seqNeeded == loopBlocks - 1)) {
      int16_t *sample = (seqNeeded == 0) ? 
        block->data : (block->data + AUDIO_BLOCK_SAMPLES - 1);
      int step = (seqNeeded == 0) ? 1 : -1;
      int32_t count = 16;
      for (int32_t i = 0; i < count; i++) {
        *sample = (int16_t)(((int32_t)(*sample) * i) / count);
        sample += step;
      }
    }
    _buffer[_tail].block = block;
    _buffer[_tail].seq = seqNeeded++;
    _tail++; _size++;
    if (_tail >= PLAY_BUFFER_BLOCKS) _tail = 0;
  }
}

void FileCache::setPath(char *newPath) {
  if ((newPath != NULL) && (strcmp(newPath, _path) == 0)) {
    WARN2("FileCache::setPath path is NULL or unchanged", newPath);
    return;
  }
  if (_file) _file.close();
  reset();
  _path = newPath;
  DBG2("FileCache::setPath", _path);
}
