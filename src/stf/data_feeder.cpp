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

#include <stf/data_feeder.h>
#include <stf/json_buffer.h>
#include <stf/provider.h>
#include <stf/data_cache.h>

namespace stf {

DataFeeder::DataFeeder(Consumer& consumer_, JsonBuffer& jsonBuffer_) : consumer(consumer_), jsonBuffer(jsonBuffer_) {
}

DataBlock& DataFeeder::nextToWrite(EnumDataField field_, EnumDataType type_, uint8_t typeInfo_, uint8_t extra_) {
  if (validBlock) { // first resolve the block
    jsonBuffer.addDataBlock(block, *cache);
    if (block.isClosedMessage()) consumer.onCloseMessageEvent(jsonBuffer, *cache);
  }

  block.reset();
  block._field = field_;
  block._type = type_;
  block._typeInfo = typeInfo_;
  block._extra = extra_;

  validBlock = true;

  return block;
}

void DataFeeder::consumeGeneratorBlock(DataBlock& block_, DataCache& cache_) {
  cache = &cache_;
  validBlock = false;

  bool feederActive = cache_._flagFeederActive;
  cache_._flagFeederActive = true;
  fnBlockGenerator* fn = (fnBlockGenerator*)block_._value.tPtr[0];
  if (fn != nullptr) (*fn)(*this, block_, cache_);
  if (block_.isClosedMessage()) cache_._flagFeederActive = feederActive; // don't reset the cache at onCloseMessageEvent if the generator is not closedMessage

  if (validBlock) { // resolve the last
    if (block_._closeComplexFlag) block._closeComplexFlag = 1;
    if (block_.isClosedMessage()) block.closeMessage();
    jsonBuffer.addDataBlock(block, *cache);
    if (block.isClosedMessage()) consumer.onCloseMessageEvent(jsonBuffer, cache_);
  } else {
    if (block_.isClosedMessage()) consumer.onCloseMessageEvent(jsonBuffer, cache_);
  }
  cache_._flagFeederActive = feederActive;
}

} // namespace stf
