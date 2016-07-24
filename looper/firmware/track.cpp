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
  // when the track is paused, stop processing audio
  active = (isPlaying() || isRecording());
  // when cancelled recording stops...
  if ((! isRecording()) && (oldState == MaybeRecording)) {
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
}

size_t Track::masterBlocks() {
  if (! _master->isOpen()) return(0);
  return(_master->blocks());
}

void Track::updateCaches() {
  if ((_state != Paused) && (_master->isOpen())) _master->fillBuffer();
  if ((isRecording()) && (_scratch->isOpen())) _scratch->emptyBuffer();
}

void Track::update() {
  if (_state == Paused) return;
  // playback and overdub
  audio_block_t *outBlock = NULL;
  if ((isPlaying()) && (_master->isOpen())) {
    outBlock = _master->readBlock();
  }
  // record
  if ((_state == MaybeRecording) || (_state == Recording)) {
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
  }
  if (! _file) return(false);
  return(true);
}

audio_block_t *PlayCache::readBlock() {
  if (_size == 0) return(NULL);
  audio_block_t *block = _buffer[_head];
  _buffer[_head] = NULL;
  _head++;
  _size--;
  if (_head >= PLAY_BUFFER_BLOCKS) _head = 0;
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
  // read a chunk
  size_t readBytes = _file.read(chunkBuffer, 512);
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
