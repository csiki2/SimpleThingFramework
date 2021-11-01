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

#include <stf/data_discovery.h>
#include <stf/data_feeder.h>
#include <stf/data_cache.h>
#include <stf/data_buffer.h>

namespace stf {

namespace Discovery {

uint addDiscoveryBlocks(DataBuffer* buffer, uint8_t topic, const DiscoveryBlock** list, EnumExtraInfo cacheCmd, const void* cacheValue, const char* device_name, const char* device_model, const char* device_manufacturer, const char* device_sw) {
  bool bl1 = device_name != nullptr || device_model != nullptr;
  bool bl2 = device_manufacturer != nullptr || device_sw != nullptr;
  uint need = (bl1 ? 1 : 0) + (bl2 ? 1 : 0) + (cacheCmd != 0 ? 1 : 0) + 1;

  if (buffer == nullptr || buffer->getFreeBlocks() < need) return need;
  switch (cacheCmd & eeiCacheMask) {
    case eeiCacheDeviceMAC48:
      if (cacheValue != nullptr) buffer->nextToWrite(edf__none, edt_None, 0, cacheCmd).setMAC48((const uint8_t*)cacheValue);
      break;
    case eeiCacheDeviceMAC64:
      if (cacheValue != nullptr) buffer->nextToWrite(edf__none, edt_None, 0, cacheCmd).setMAC64((const uint8_t*)cacheValue);
      break;
    case eeiCacheDeviceHost:
      if (cacheValue != nullptr) buffer->nextToWrite(edf__none, edt_None, 0, cacheCmd).setPtr(cacheValue);
      break;
  }

  if (bl1) buffer->nextToWrite(edf__none, edt_None, 0, eeiCacheBlock1).setPtr(device_name, device_model);
  if (bl2) buffer->nextToWrite(edf__none, edt_None, 0, eeiCacheBlock2).setPtr(device_manufacturer, device_sw);
  buffer->nextToWrite(edf__discList, edt_Generator, topic).setPtr((const void*)&generateDiscoveryBlocks, (void*)list).closeMessage();
  return 0;
}

void generateDiscoveryBlock(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache, const DiscoveryBlock& discovery) {
  if ((generatorBlock._extra & eeiCacheMask) == eeiCacheDeviceHost)
    feeder.nextToWrite(discovery._field, edt_None, eeiCacheDeviceHost).setPtr(&Host::_info);
  else
    cache._block_device._field = discovery._field;
  feeder.nextToWrite(edf__topic, edt_Topic, etitConfig + discovery._component); // device should be cached already
  if (discovery._name != nullptr)
    feeder.nextToWrite(edf_name, edt_String, etisSource0Ptr).setPtr(discovery._name);
  else
    feeder.nextToWrite(edf_name, edt_String, etisSource0CacheField + etisCaseSmart);
  if (discovery._component != edcSwitch) // Home Assistant doesn't accept a switch with unit_of_measurement
    feeder.nextToWrite(edf_unit_of_measurement, edt_String, etisSource0Ptr).setPtr(discovery._measure);
  feeder.nextToWrite(edf_state_topic, edt_Topic, etitState + generatorBlock._typeInfo);
  if (discovery._component == edcSwitch)
    feeder.nextToWrite(edf_command_topic, edt_Topic, etitCommand + generatorBlock._typeInfo);
  feeder.nextToWrite(edf_unique_id, edt_String, etisSource0MACId + etisSource1CacheField);
  if (discovery._device_class != nullptr) {
    bool useName = strcmp(discovery._device_class, "_") == 0;
    feeder.nextToWrite(edf_device_class, edt_String, etisSource0Ptr + (useName ? etisCaseLower : 0)).setPtr((void*)useName ? discovery._name : discovery._device_class);
  }
  feeder.nextToWrite(edf_value_template, edt_String, etisSource0FmtPtr + etisSource1CacheField).setPtr("{{ value_json.%s | is_defined }}");
  feeder.nextToWrite(edf_device, edt_Device, etidSource0Name + etidSource1Model + etidConnectionsCached + etidIdentifiersCached)._value = cache._block1._value;
  DataBlock& devCnt = feeder.nextToWrite(edf_device, edt_Device, etidSource0Manufacturer + etidSource1SWVersion);
  devCnt._value = cache._block2._value;
  devCnt._closeComplexFlag = 1;
  feeder.nextToWrite(edf_platform, edt_String, etisSource0Ptr).setPtr("mqtt").closeMessage();
}

void generateDiscoveryBlocks(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache) {
  EnumDataField field = generatorBlock._field;
  if (field != edf__discElem && field != edf__discList) return;

  if (!cache._flagDevice) cache.addBlock(generatorBlock, eeiCacheDeviceMainHost);
  if (field == edf__discList) {
    const DiscoveryBlock** list = (const DiscoveryBlock**)generatorBlock._value.tPtr[1];
    for (int idx = 0; *list != nullptr; list++, idx++) generateDiscoveryBlock(feeder, generatorBlock, cache, **list);
  } else {
    generateDiscoveryBlock(feeder, generatorBlock, cache, *(const DiscoveryBlock*)generatorBlock._value.tPtr[1]);
  }
}

const DiscoveryBlock _Temperature_C = {edf_tempc, edcSensor, "Temperature", "°C", "_"};
const DiscoveryBlock _Humidity = {edf_hum, edcSensor, "Humidity", "%", "_"};
const DiscoveryBlock _Battery = {edf_batt, edcSensor, "Battery", "%", "_"};
const DiscoveryBlock _Volt = {edf_volt, edcSensor, nullptr, "V", nullptr};
const DiscoveryBlock _Uptime_S = {edf_uptime_s, edcSensor, "Uptime", "s", nullptr};
const DiscoveryBlock _Uptime_D = {edf_uptime_d, edcSensor, "Uptime", "d", nullptr};
const DiscoveryBlock _Free_Memory = {edf_free_memory, edcSensor, nullptr, "B", nullptr};

const DiscoveryBlock* _listVoltBattHumTempC[] = {&Discovery::_Volt, &Discovery::_Battery, &Discovery::_Humidity, &Discovery::_Temperature_C, nullptr};

const char* topicConfigComponent[] = {
    "sensor", // edcSensor
    "binary_sensor", // edcBinarySensor
    "switch", // edcSwitch
};

} // namespace Discovery

} // namespace stf
