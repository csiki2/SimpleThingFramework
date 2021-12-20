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
#include <stf/data_discovery.h>

namespace stf {

class DataBuffer;

enum class EnumBTResult {
  Resolved = 0,
  Unknown = 1,
  SmallBuffer = 2,
  Disabled = 3,
};

class BTPacket;

class BTResolver {
public:
  static EnumBTResult resolve(DataBuffer* buffer, const BTPacket& packet);

protected:
  static int32_t getBufferValueLE(const uint8_t* buffer, uint8_t size); // little endian
  static int32_t getBufferValueBE(const uint8_t* buffer, uint8_t size); // big endian
  static uint addDiscoveryBlock(DataBuffer& buffer, const DiscoveryBlock& block, const uint8_t* mac, const char* deviceName, const char* deviceModel, const char* deviceManufacturer, const char* deviceSW);
  static uint addDiscoveryBlocks(DataBuffer& buffer, const DiscoveryBlock** list, const uint8_t* mac, const char* deviceName, const char* deviceModel, const char* deviceManufacturer, const char* deviceSW);

  typedef EnumBTResult (*ServiceFunction)(DataBuffer* buffer, const BTPacket& packet);
  static const ServiceFunction _serviceFunctions[];

  static EnumBTResult serviceMiBacon(DataBuffer* buffer, const BTPacket& packet);
  static EnumBTResult serviceTelinkLYWSD03MMC_atc1441_pvvx(DataBuffer* buffer, const BTPacket& packet);
  static EnumBTResult serviceLaica_700234(DataBuffer* buffer, const BTPacket& packet);
};

class STFATTR_PACKED BTDevice {
public:
  BTDevice() {
    _whiteList = _blackList = _discovery = false;
    _mac[0] = 0xff;
  }
  union STFATTR_PACKED {
    uint8_t _mac[6];
    uint16_t _mac16[3];
    uint32_t _mac32[1];
  };
  uint8_t _reserved;
  bool _whiteList : 1;
  bool _blackList : 1;
  bool _discovery : 1;
};

class BTDeviceGroup {
public:
  BTDeviceGroup();

  void updateDevices();
  BTDevice* findDevice(const uint8_t* mac);
  BTDevice* findOrCreateDevice(const uint8_t* mac);

  ElapsedTime _lastReadyTime;
  ElapsedTime _discoveryTime;

  std::vector<BTDevice*> _devices;
};

} // namespace stf
