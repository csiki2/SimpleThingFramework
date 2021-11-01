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

#pragma once

#include <stf/data_block.h>

namespace stf {

enum EnumDataBlockElemSeparator {
  edsNone = 0,
  edsComma,
  edsDot,
  edsColon,
  edsSlash,
};

// Every buffer has exactly one consumer and multiple provider
class Provider;
class Consumer;

class DataBuffer : public Object {
public:
  DataBuffer(TaskRoot* task, DataBlock* buffer, uint size);

  virtual void init() override;
  virtual int initPriority() override;
  virtual Object** getObjectHead();

  void setupProviders();
  uint loopProviders();

  uint getUsedBlocks();
  uint getFreeBlocks();
  bool hasClosedMessage();

  DataBlock& getReadBlock();
  DataBlock& getWriteBlock();
  void IncrementReadIndex();
  void IncrementWriteIndex();

  DataBlock& nextToWrite(EnumDataField field, EnumDataType type, uint8_t typeInfo, uint8_t extra = 0);
  void closeMessage();

  inline Consumer* getConsumer() {
    return _parentConsumer;
  }

protected:
  // #include <memory> and #include <stdatomic.h> have conflicts...
  struct {
    int volatile __val;
  } writeIdx; // atomic_int writeIdx
  struct {
    int volatile __val;
  } readIdx; // atomic_int readIdx

  int _head, _tail;
  DataBlock* _buffer;
  uint _size;

  friend class TaskRoot;
  TaskRoot* _parentTask;

  // List of buffers for the consumer
  DataBuffer* _consumerBufferNext;
  Consumer* _parentConsumer;
  friend class Consumer;
};

template <uint SIZE>
class StaticDataBuffer : public DataBuffer {
public:
  StaticDataBuffer(TaskRoot* task) : DataBuffer(task, localBuffer, SIZE){};

protected:
  DataBlock localBuffer[SIZE];
};

#define STF_BUFFER0(name, size, task) STF_BUFFER_DECLARE(name, size, task)
#define STF_BUFFER1(name, size, task, provider) \
  STF_BUFFER_DECLARE(name, size, task)          \
  STF_BUFFER_PROVIDER(name, provider)
#define STF_BUFFER2(name, size, task, provider1, provider2) \
  STF_BUFFER_DECLARE(name, size, task)                      \
  STF_BUFFER_PROVIDER(name, provider1)                      \
  STF_BUFFER_PROVIDER(name, provider2)
#define STF_BUFFER_DECLARE(name, size, task) extern StaticDataBuffer<size> g_##name;
#define STF_BUFFER_PROVIDER(name, provider)  constexpr DataBuffer* g_buffer##provider = &g_##name;
STFBUFFERS;

} // namespace stf
