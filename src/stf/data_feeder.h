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

class DataCache;
class Consumer;
class JsonBuffer;

// Behaves like a DataBuffer, but instead of collecting all the data it just feeds instantly to the jsonBuffer and the consumer
class DataFeeder {
public:
  DataFeeder(Consumer& consumer, JsonBuffer& jsonBuffer);

  DataBlock& nextToWrite(EnumDataField field, EnumDataType type, uint8_t typeInfo, uint8_t extra = 0);

protected:
  Consumer& _consumer;
  JsonBuffer& _jsonBuffer;

  DataBlock _block;
  DataCache* _cache;
  bool _validBlock;

  friend class Consumer;
  void consumeGeneratorBlock(DataBlock& block, DataCache& cache);
};

// Callback function to generate the DataBlocks
typedef void fnBlockGenerator(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache);

} // namespace stf
