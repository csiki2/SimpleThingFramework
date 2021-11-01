/*
  SimpleThingFramework

  Copyright 2021 Andras Csikvari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <stf/data_block.h>
#include <stf/data_buffer.h>
#include <stf/provider.h>

#include <stdatomic.h>

namespace stf {

// Buffer connections with the provider objects
#undef STF_BUFFER_DECLARE
#define STF_BUFFER_DECLARE(name, size, task) StaticDataBuffer<size> g_##name(&SimpleTask<EnumSimpleTask::task>::_obj);
#undef STF_BUFFER_PROVIDER
#define STF_BUFFER_PROVIDER(name, provider)

STFBUFFERS;

#define getReadIdx()       atomic_load_explicit(&readIdx, ::memory_order_seq_cst)
#define setReadIdx(value)  atomic_store_explicit(&readIdx, (value) % (2 * _size), ::memory_order_seq_cst)
#define getWriteIdx()      atomic_load_explicit(&writeIdx, ::memory_order_seq_cst)
#define setWriteIdx(value) atomic_store_explicit(&writeIdx, (value) % (2 * _size), ::memory_order_seq_cst)

DataBuffer::DataBuffer(TaskRoot* task, DataBlock* buffer, uint size) : _buffer(buffer), _size(size) {
  setReadIdx(0);
  setWriteIdx(0);

  _parentTask = task;

  _consumerBufferNext = nullptr;
}

void DataBuffer::init() {
}

int DataBuffer::initPriority() {
  return 10;
}

Object** DataBuffer::getObjectHead() {
  return (Object**)(_parentTask != nullptr ? &_parentTask->_bufferHead : nullptr);
}

void DataBuffer::setupProviders() {
  for (Provider* pr = Provider::getNext(nullptr, this); pr != nullptr; pr = Provider::getNext(pr, this))
    pr->setup();
}

uint DataBuffer::loopProviders() {
  uint wait = 100;
  for (Provider* pr = Provider::getNext(nullptr, this); pr != nullptr; pr = Provider::getNext(pr, this)) {
    uint res = pr->loop();
    if (res < wait) wait = res;
  }
  return wait;
}

bool DataBuffer::hasClosedMessage() {
  int ridx = getReadIdx();
  int widx = getWriteIdx();

  if (ridx == widx) return false;
  if (widx < ridx) widx += 2 * _size;
  for (; ridx < widx; ridx++)
    if (_buffer[ridx % _size].isClosedMessage()) return true;
  return false;
}

uint DataBuffer::getUsedBlocks() {
  int ridx = getReadIdx();
  int widx = getWriteIdx();
  uint written = widx >= ridx ? widx - ridx : 2 * _size + widx - ridx;
  return written;
}

uint DataBuffer::getFreeBlocks() {
  int ridx = getReadIdx();
  int widx = getWriteIdx();
  uint written = widx >= ridx ? ridx - widx + _size : ridx - widx - _size;
  return written;
}

DataBlock& DataBuffer::getReadBlock() {
  int ridx = getReadIdx();
  return _buffer[ridx % _size];
}

DataBlock& DataBuffer::getWriteBlock() {
  int widx = getWriteIdx();
  return _buffer[widx % _size];
}

DataBlock& DataBuffer::nextToWrite(EnumDataField field, EnumDataType type, uint8_t typeInfo, uint8_t extra) {
  int widx = getWriteIdx();
  DataBlock& block = _buffer[widx % _size];

  block.reset();
  block._field = field;
  block._type = type;
  block._typeInfo = typeInfo;
  block._extra = extra;

  // This is ok: although it can be read, shouldn't be used till a "closeMessageFlag" block is not in the queue
  setWriteIdx(widx + 1);

  return block;
}

// The function assumes that the previous message wasn't closed
void DataBuffer::closeMessage() {
  _buffer[(getWriteIdx() - 1) % _size].closeMessage();
}

void DataBuffer::IncrementReadIndex() {
  int ridx = getReadIdx();
  _buffer[ridx % _size].reset();
  setReadIdx(ridx + 1);
}

void DataBuffer::IncrementWriteIndex() {
  setWriteIdx(getWriteIdx() + 1);
}

} // namespace stf
