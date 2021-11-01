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

void DataCache::addBlock(const DataBlock& block, uint8_t cmd) {
  uint8_t chcmd = cmd & eeiCacheMask;
  switch (chcmd) {
    case eeiCacheBlock1:
      setBlock1(block);
      break;
    case eeiCacheBlock2:
      setBlock2(block);
      break;
    case eeiCacheDeviceMAC48:
    case eeiCacheDeviceMAC64:
      setBlockDevice(block);
      _device.create(block._value.t8, chcmd == eeiCacheDeviceMAC48 ? 6 : 8);
      break;
    case eeiCacheDeviceHost:
      setBlockDevice(block);
      _device.info = *(const DeviceInfo*)block._value.tPtr[0];
      _device.createMissing();
      break;
    case eeiCacheDeviceMainHost:
      setBlockDevice(block);
      _device.info = Host::_info;
      _device.createMissing();
      break;
    default:
      break;
  }
}

} // namespace stf
