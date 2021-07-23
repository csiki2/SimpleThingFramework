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

#include <stf/data_cache.h>

namespace stf {

void DataCache::addBlock(const DataBlock& block_, uint8_t cmd_) {
  uint8_t chcmd = cmd_ & etigCacheMask;
  switch (chcmd) {
    case etigCacheBlock1:
      setBlock1(block_);
      break;
    case etigCacheBlock2:
      setBlock2(block_);
      break;
    case etigCacheDeviceMAC48:
    case etigCacheDeviceMAC64:
      setBlockDevice(block_);
      _device.create(block_._value.t8, chcmd == etigCacheDeviceMAC48 ? 6 : 8);
      break;
    case etigCacheDeviceHost:
      setBlockDevice(block_);
      _device.info = *(const DeviceInfo*)block_._value.tPtr[0];
      _device.createMissing();
      break;
    default:
      break;
  }
}

} // namespace stf
