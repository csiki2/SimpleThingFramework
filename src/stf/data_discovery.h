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

#include <stf/os.h>
#include <stf/data_field.h>
#include <stf/data_type.h>

namespace stf {

class DataFeeder;
class DataBuffer;

struct DataBlock;
struct DataCache;

enum EnumDiscoveryComponent {
  edcSensor = 0,
  edcBinarySensor = 1,
};

struct DiscoveryBlock {
  EnumDataField _field : 8;
  EnumDiscoveryComponent _component : 4;
  const char* _name;
  const char* _measure;
  const char* _device_class;
};

namespace Discovery {

extern const char* topicConfigComponent[];

// Device info should be added independently...
uint addDiscoveryBlocks(DataBuffer* buffer_, uint8_t topic_, const DiscoveryBlock** list_, EnumTypeInfoGeneric cacheCmd_ = etigNone, const void* cacheValue_ = nullptr, const char* device_name = nullptr, const char* device_model = nullptr, const char* device_manufacturer = nullptr, const char* device_sw = nullptr);

void generateDiscoveryBlocks(DataFeeder& feeder_, const DataBlock& generatorBlock_, DataCache& cache_);
void generateDiscoveryBlock(DataFeeder& feeder_, const DataBlock& generatorBlock_, DataCache& cache_, const DiscoveryBlock& discovery_);

extern const DiscoveryBlock _Temperature_C;
extern const DiscoveryBlock _Humidity;
extern const DiscoveryBlock _Battery;
extern const DiscoveryBlock _Uptime_S;
extern const DiscoveryBlock _Uptime_D;
extern const DiscoveryBlock _Free_Memory;

extern const DiscoveryBlock* _listBattHumTempC[];

} // namespace Discovery
} // namespace stf
