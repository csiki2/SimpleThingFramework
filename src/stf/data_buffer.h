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

class DataBuffer {
public:
  DataBuffer(DataBlock* buffer_, uint size);

  void addProvider(Provider* provider_);

  uint getUsedBlocks();
  uint getFreeBlocks();
  bool hasClosedMessage();

  DataBlock& getReadBlock();
  DataBlock& getWriteBlock();
  void IncrementReadIndex();
  void IncrementWriteIndex();

  DataBlock& nextToWrite(EnumDataField field_, EnumDataType type_, uint8_t typeInfo_, uint8_t extra_ = 0);
  void closeMessage();

  inline Consumer* getConsumer() {
    return parentConsumer;
  }

protected:
  // #include <memory> and #include <stdatomic.h> have conflicts...
  struct {
    int volatile __val;
  } writeIdx; // atomic_int writeIdx
  struct {
    int volatile __val;
  } readIdx; // atomic_int readIdx

  int head, tail;
  DataBlock* buffer;
  uint size;

  // List of providers
  Provider* providerHead;
  friend class Provider;

  // List of buffers for the consumer
  DataBuffer* bufferNext;
  Consumer* parentConsumer;
  friend class Consumer;
};

template <uint size_>
class StaticDataBuffer : public DataBuffer {
public:
  StaticDataBuffer() : DataBuffer(localBuffer, size_){};

protected:
  DataBlock localBuffer[size_];
};

#define STF_BUFFER0(name, size) STF_BUFFER_DECLARE(name, size)
#define STF_BUFFER1(name, size, provider) \
  STF_BUFFER_DECLARE(name, size)          \
  STF_BUFFER_PROVIDER(name, provider)
#define STF_BUFFER2(name, size, provider1, provider2) \
  STF_BUFFER_DECLARE(name, size)                      \
  STF_BUFFER_PROVIDER(name, provider1)                \
  STF_BUFFER_PROVIDER(name, provider2)
#define STF_BUFFER_DECLARE(name, size)      extern StaticDataBuffer<size> name;
#define STF_BUFFER_PROVIDER(name, provider) extern DataBuffer& buffer##provider;
STFBUFFERS;

} // namespace stf
