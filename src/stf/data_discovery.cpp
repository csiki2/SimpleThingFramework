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

uint addDiscoveryBlocks(DataBuffer* buffer_, uint8_t topic_, const DiscoveryBlock** list_, EnumTypeInfoGeneric cacheCmd_, const void* cacheValue_, const char* device_name, const char* device_model, const char* device_manufacturer, const char* device_sw) {
  bool bl1 = device_name != nullptr || device_model != nullptr;
  bool bl2 = device_manufacturer != nullptr || device_sw != nullptr;
  uint need = (bl1 ? 1 : 0) + (bl2 ? 1 : 0) + (cacheCmd_ != 0) + 1;

  if (buffer_ == nullptr || buffer_->getFreeBlocks() < need) return need;
  switch (cacheCmd_ & etigCacheMask) {
    case etigCacheDeviceMAC48:
      if (cacheValue_ != nullptr) buffer_->nextToWrite(edf__none, edt_None, cacheCmd_).setMAC48((const uint8_t*)cacheValue_);
      break;
    case etigCacheDeviceMAC64:
      if (cacheValue_ != nullptr) buffer_->nextToWrite(edf__none, edt_None, cacheCmd_).setMAC64((const uint8_t*)cacheValue_);
      break;
    case etigCacheDeviceHost:
      if (cacheValue_ != nullptr) buffer_->nextToWrite(edf__none, edt_None, cacheCmd_).setPtr(cacheValue_);
      break;
  }

  if (bl1) buffer_->nextToWrite(edf__none, edt_None, etigCacheBlock1).setPtr(device_name, device_model);
  if (bl2) buffer_->nextToWrite(edf__none, edt_None, etigCacheBlock2).setPtr(device_manufacturer, device_sw);
  buffer_->nextToWrite(edf__discList, edt_Generator, topic_).setPtr((const void*)&generateDiscoveryBlocks, (void*)list_).closeMessage();
  return 0;
}

void generateDiscoveryBlock(DataFeeder& feeder_, const DataBlock& generatorBlock_, DataCache& cache_, const DiscoveryBlock& discovery_) {
  if (generatorBlock_._typeInfo & etigHost)
    feeder_.nextToWrite(discovery_._field, edt_None, etigCacheDeviceHost).setPtr(hostInfo);
  else
    cache_._block_device._field = discovery_._field;
  feeder_.nextToWrite(edf__topic, edt_Topic, etitConfig + discovery_._component); // device should be cached already
  if (discovery_._name != nullptr)
    feeder_.nextToWrite(edf_name, edt_String, etisSource0Ptr).setPtr(discovery_._name);
  else
    feeder_.nextToWrite(edf_name, edt_String, etisSource0CacheField + etisCaseSmart);
  feeder_.nextToWrite(edf_unit_of_measurement, edt_String, etisSource0Ptr).setPtr(discovery_._measure);
  feeder_.nextToWrite(edf_state_topic, edt_String, etisSource0DeviceId + etisSource1TopicFilter)._value.t32[1] = generatorBlock_._typeInfo & etigTopicMask; // for now...
  feeder_.nextToWrite(edf_unique_id, edt_String, etisSource0DeviceId + etisSource1CacheField);
  if (discovery_._device_class != nullptr) {
    bool useName = strcmp(discovery_._device_class, "_") == 0;
    feeder_.nextToWrite(edf_device_class, edt_String, etisSource0Ptr + (useName ? etisCaseLower : 0)).setPtr((void*)useName ? discovery_._name : discovery_._device_class);
  }
  feeder_.nextToWrite(edf_value_template, edt_String, etisSource0FmtPtr + etisSource1CacheField).setPtr("{{ value_json.%s | is_defined }}");
  feeder_.nextToWrite(edf_device, edt_Device, etidSource0Name + etidSource1Model + etidConnectionsCached + etidIdentifiersCached)._value = cache_._block1._value;
  DataBlock& devCnt = feeder_.nextToWrite(edf_device, edt_Device, etidSource0Manufacturer + etidSource1SWVersion);
  devCnt._value = cache_._block2._value;
  devCnt._closeComplexFlag = 1;
  feeder_.nextToWrite(edf_platform, edt_String, etisSource0Ptr).setPtr("mqtt").closeMessage();
}

void generateDiscoveryBlocks(DataFeeder& feeder_, const DataBlock& generatorBlock_, DataCache& cache_) {
  EnumDataField field = generatorBlock_._field;
  if (field != edf__discElem && field != edf__discList) return;

  if (field == edf__discList) {
    const DiscoveryBlock** list = (const DiscoveryBlock**)generatorBlock_._value.tPtr[1];
    for (int idx = 0; *list != nullptr; list++, idx++) generateDiscoveryBlock(feeder_, generatorBlock_, cache_, **list);
  } else {
    generateDiscoveryBlock(feeder_, generatorBlock_, cache_, *(const DiscoveryBlock*)generatorBlock_._value.tPtr[1]);
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
};

} // namespace Discovery

} // namespace stf
