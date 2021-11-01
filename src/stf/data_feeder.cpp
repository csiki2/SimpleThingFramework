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

DataFeeder::DataFeeder(Consumer& consumer, JsonBuffer& jsonBuffer) : _consumer(consumer), _jsonBuffer(jsonBuffer) {
}

DataBlock& DataFeeder::nextToWrite(EnumDataField field, EnumDataType type, uint8_t typeInfo, uint8_t extra) {
  if (_validBlock) { // first resolve the block
    _jsonBuffer.addDataBlock(_block, *_cache);
    if (_block.isClosedMessage()) _consumer.onCloseMessageEvent(_jsonBuffer, *_cache);
  }

  _block.reset();
  _block._field = field;
  _block._type = type;
  _block._typeInfo = typeInfo;
  _block._extra = extra;

  _validBlock = true;

  return _block;
}

void DataFeeder::consumeGeneratorBlock(DataBlock& block, DataCache& cache) {
  _cache = &cache;
  _validBlock = false;

  bool feederActive = cache._flagFeederActive;
  cache._flagFeederActive = true;
  fnBlockGenerator* fn = (fnBlockGenerator*)block._value.tPtr[0];
  if (fn != nullptr) (*fn)(*this, block, cache);
  if (block.isClosedMessage()) cache._flagFeederActive = feederActive; // don't reset the cache at onCloseMessageEvent if the generator is not closedMessage

  if (_validBlock) { // resolve the last
    if (block._closeComplexFlag) _block._closeComplexFlag = 1;
    if (block.isClosedMessage()) _block.closeMessage();
    _jsonBuffer.addDataBlock(_block, *_cache);
    if (_block.isClosedMessage()) _consumer.onCloseMessageEvent(_jsonBuffer, cache);
  } else {
    if (block.isClosedMessage()) _consumer.onCloseMessageEvent(_jsonBuffer, cache);
  }
  cache._flagFeederActive = feederActive;
}

} // namespace stf
