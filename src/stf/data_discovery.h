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
  edcSwitch = 2,
};

enum EnumEntityCategory {
  eecPrimary = 0,
  eecConfig = 1,
  eecDiagnostic = 2,
  eecSystem = 3
};

struct DiscoveryBlock {
  EnumDataField _field : 8;
  EnumDiscoveryComponent _component : 4;
  EnumEntityCategory _category : 4;
  const char* _name;
  const char* _measure;
  const char* _device_class;
};

class Discovery {
public:
  // Device info should be added independently...
  static uint addBlocks(DataBuffer* buffer, uint8_t topic, const DiscoveryBlock** list, EnumExtraInfo cacheCmd = eeiNone, const void* cacheValue = nullptr, const char* device_name = nullptr, const char* device_model = nullptr, const char* device_manufacturer = nullptr, const char* device_sw = nullptr);

  static void generateBlocks(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache);
  static void generateBlock(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache, const DiscoveryBlock& discovery);

  static const DiscoveryBlock _Temperature_C;
  static const DiscoveryBlock _Humidity;
  static const DiscoveryBlock _Battery;
  static const DiscoveryBlock _Volt;
  static const DiscoveryBlock _Uptime_S;
  static const DiscoveryBlock _Uptime_D;
  static const DiscoveryBlock _Free_Memory;

  static const DiscoveryBlock* _listVoltBattHumTempC[];

  static const char* _topicConfigComponent[];
  static const char* _entityCategory[];
};

} // namespace stf
