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

class DataBuffer;

enum EnumBTResult {
  ebtrResolved = 0,
  ebtrWrongType = 1,
  ebtrSmallBuffer = 2
};

typedef EnumBTResult (*BTServiceFunction)(const uint8_t* mac_, uint8_t macType_, uint32_t uuid_, const uint8_t* serviceData_, uint serviceDataLength_, DataBuffer& buffer_);

struct BTServiceType {
  uint32_t _uuid;
  BTServiceFunction _func;
};

struct __attribute__((packed)) BTDevice {
  BTDevice() {
    _whiteList = _blackList = _discovery = false;
    _mac[0] = 0xff;
  }
  union __attribute__((packed)) {
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
  BTDevice* findDevice(const uint8_t* mac_);
  BTDevice* findOrCreateDevice(const uint8_t* mac_);

  uint32_t _connectionTime;
  uint32_t _discoveryTime;

  std::vector<BTDevice*> _devices;
};

namespace BT {
extern const BTServiceType _serviceFunction[];
extern const uint _serviceFunctionLength;
} // namespace BT

} // namespace stf
