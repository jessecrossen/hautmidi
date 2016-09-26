#include "track.h"

#include <string.h>

// TRACK **********************************************************************

void Track::setPath(char *path) {
  if (path == NULL) return;
  if (strncmp(path, _path, sizeof(_path)) == 0) return;
  strncpy(_path, path, sizeof(_path));
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
  // whichever file exists should be the master
  if (SD.exists(_pathB)) {
    _master->setPath(_pathB);
    _scratch->setPath(_pathA);
  }
  else {
    _master->setPath(_pathA);
    _scratch->setPath(_pathB);
  }
  // open the master file
  _master->open();
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
  if ((willBeRecording) && (! wasRecording)) {
    _scratch->open();
    _master->fillBuffer();
  }
  _state = newState;
  sinceStateChange = 0;
  // when cancelled recording stops...
  if ((! isRecording()) && (oldState == MaybeRecording)) {
    _sync->cancelRecording(this);
    // remove the scratch file when recording is cancelled
    temp = _scratch->path();
    _scratch->setPath(NULL);
	  SD.remove(_scratch->path());
	  _scratch->setPath(temp);
	}
  // when true recording stops...
  else if ((! isRecording()) && (oldState == Recording)) {
    // flush recorded audio
    _scratch->flush();
    //!!! // TODO
    //!!! size_t bytes;
    //!!! do {
    //!!!   bytes = _master.read(_writeBuffer, 512);
    //!!!   if (bytes > 0) _scratch.write(_writeBuffer, bytes);
    //!!! } while (bytes > 0);
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
  setState(Paused);
  _master->setPath(NULL);
  _scratch->setPath(NULL);
  if (SD.exists(_pathA)) SD.remove(_pathA);
  if (SD.exists(_pathB)) SD.remove(_pathB);
  _master->setPath(_pathA);
  _scratch->setPath(_pathB);
  _sync->trackErased(this);
}

size_t Track::masterBlocks() {
  if (! _master->isOpen()) return(0);
  return(_master->blocks());
}
size_t Track::recordingBlock() {
  if (! _scratch) return(0);
  return(_scratch->block());
}
size_t Track::playingBlock() {
  if (! _master) return(0);
  return(_master->block());
}
size_t Track::playBlocks() {
  if (! _master) return(0);
  return(_master->playBlocks);
}

void Track::updateCaches() {
  if (_master->isOpen()) _master->fillBuffer();
  if ((isRecording()) && (_scratch->isOpen())) _scratch->emptyBuffer();
}

void Track::updatePreroll() {
  if (_master->isOpen()) {
    _master->preroll = _sync->blocksUntilNextSyncPoint(this);
  }
}

void Track::update() {
  // playback and overdub
  audio_block_t *outBlock = NULL;
  if (_master->isOpen()) {
    size_t beginBlock = _master->block();
    outBlock = _master->readBlock();
    // recompute the track's loop length once the first block is played
    if ((beginBlock == 0) && (_master->block() != beginBlock)) {
      _master->playBlocks = _sync->trackStarting(this);
    }
  }
  // if we're not playing or recording, don't route anything to output
  if ((! isPlaying()) && (! isRecording())) {
    if (outBlock) release(outBlock);
    return;
  }
  // record
  if ((_state == MaybeRecording) || (_state == Recording)) {
    // mark when recording starts
    if (_scratch->block() == 0) _sync->trackRecording(this);
    // get a block of input
    audio_block_t *inBlock = receiveReadOnly();
	  // if we're overdubbing, mix the output block with the input
	  audio_block_t *recordBlock = inBlock;
	  if (outBlock) {
	    recordBlock = allocate();
	    int16_t *inSample = inBlock->data;
	    int16_t *outSample = outBlock->data;
	    int16_t *recordSample = recordBlock->data;
	    for (size_t i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
	      *recordSample++ = *inSample++ + *outSample++;
	    }
	    release(inBlock);
	  }
	  if (! _scratch->writeBlock(recordBlock)) {
	    release(recordBlock);
	  }
  }
  // send the output block if there is one
  if (outBlock != NULL) {
    transmit(outBlock, 0);
    release(outBlock);
  }
}

// RECORDING CACHE ************************************************************

bool RecordCache::open() {
  if (_path == NULL) return(false);
  if (! _file) _file = SD.open(_path, O_WRITE | O_CREAT | O_TRUNC);
  if (! _file) return(false);
  return(true);
}

bool RecordCache::writeBlock(audio_block_t *block) {
  // if the buffer is full, we have to drop the block
  if (_size >= RECORD_BUFFER_BLOCKS) return(false);
  _buffer[_tail] = block;
  _tail++;
  _size++;
  if (_tail >= RECORD_BUFFER_BLOCKS) _tail = 0;
  _blocks++;
  _block++;
  return(true);
}

void RecordCache::flush() {
  while (_size > 0) {
    if (writeChunk() == 0) break;
  }
}

void RecordCache::emptyBuffer() {
  if (_size < 2) return;
  writeChunk();
}

size_t RecordCache::writeChunk() {
  static byte chunkBuffer[512];
  if (_size == 0) return(0);
  if (! open()) return(0);
  size_t bytes = 0;
  audio_block_t *block;
  for (size_t i = 0; (i < 2) && (_size > 0); i++) {
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

bool PlayCache::open() {
  if (_path == NULL) return(false);
  if (! _file) {
    _file = SD.open(_path, O_READ);
    size_t bytes = _file ? _file.size() : 0;
    // guard against small files, which could cause a tight loop
    if (bytes < AUDIO_BLOCK_BYTES) {
      _file.close();
      SD.remove(_path);
      return(false);
    }
    _blocks = bytes / AUDIO_BLOCK_BYTES;
    playBlocks = _blocks;
    preroll = 0;
  }
  if (! _file) return(false);
  return(true);
}

audio_block_t *PlayCache::readBlock() {
  // if we have no blocks, there's nothing to play
  if (_size == 0) return(NULL);
  // if we're still in the preroll, there's nothing to play
  if (preroll > 0) {
    preroll--;
    return(NULL);
  }
  // if we're within the number of blocks available, we have something to play
  audio_block_t *block = NULL;
  if (_block < _blocks) {
    block = _buffer[_head];
    _buffer[_head] = NULL;
    _head++;
    _size--;
    if (_head >= PLAY_BUFFER_BLOCKS) _head = 0;
  }
  // advance the block counter
  _block = (_block + 1) % playBlocks;
  return(block);
}

void PlayCache::fillBuffer() {
  if (_size + 1 >= PLAY_BUFFER_BLOCKS) return;
  readChunk();
}

size_t PlayCache::readChunk() {
  static byte chunkBuffer[512];
  if (! open()) return(0);
  if (_size + 1 >= PLAY_BUFFER_BLOCKS) return(0);
  // see if we need to re-seek to zero to sync to the current playback length
  size_t blocksLeft = playBlocks - (_file.position() / AUDIO_BLOCK_BYTES);
  size_t bytesToRead = 512;
  if (blocksLeft <= 0) {
    _file.seek(0);
  }
  else if (blocksLeft < (512 / AUDIO_BLOCK_BYTES)) {
    bytesToRead = blocksLeft * AUDIO_BLOCK_BYTES;
  }
  // read a chunk
  size_t readBytes = _file.read(chunkBuffer, bytesToRead);
  // if we reach the end of the loop, start at the beginning
  if (readBytes < 512) {
    _file.seek(0);
    readBytes += _file.read(chunkBuffer + readBytes, 512 - readBytes);
  }
  audio_block_t *block;
  for (size_t offset = 0; offset < readBytes; offset += AUDIO_BLOCK_BYTES) {
    block = allocate();
    memcpy(block->data, chunkBuffer + offset, AUDIO_BLOCK_BYTES);
    _buffer[_tail] = block;
    _tail++;
    _size++;
    if (_tail >= PLAY_BUFFER_BLOCKS) _tail = 0;
  }
  return(readBytes);
}
