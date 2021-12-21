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

const char* DiscoveryBlock::getName() const {
  return _name != nullptr ? _name : DataField::_list[_field];
}

uint Discovery::addBlock(DataBuffer* buffer, uint8_t topic, const DiscoveryBlock& block, EnumExtraInfo cacheCmd, const void* cacheValue, const char* device_name, const char* device_model, const char* device_manufacturer, const char* device_sw) {
  uint need = setupGenerator(buffer, topic, cacheCmd, cacheValue, device_name, device_model, device_manufacturer, device_sw);
  if (buffer == nullptr || need != 0) return need;
  buffer->nextToWrite(edf__discElem, edt_Generator, topic).setPtr((const void*)&generateBlocks, (void*)&block).closeMessage();
  return 0;
}

uint Discovery::addBlocks(DataBuffer* buffer, uint8_t topic, const DiscoveryBlock** list, EnumExtraInfo cacheCmd, const void* cacheValue, const char* device_name, const char* device_model, const char* device_manufacturer, const char* device_sw) {
  uint need = setupGenerator(buffer, topic, cacheCmd, cacheValue, device_name, device_model, device_manufacturer, device_sw);
  if (buffer == nullptr || need != 0) return need;
  buffer->nextToWrite(edf__discList, edt_Generator, topic).setPtr((const void*)&generateBlocks, (void*)list).closeMessage();
  return 0;
}

uint Discovery::setupGenerator(DataBuffer* buffer, uint8_t topic, EnumExtraInfo cacheCmd, const void* cacheValue, const char* device_name, const char* device_model, const char* device_manufacturer, const char* device_sw) {
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
  return 0;
}

void Discovery::generateBlock(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache, const DiscoveryBlock& discovery) {
  if ((generatorBlock._extra & eeiCacheMask) == eeiCacheDeviceHost)
    feeder.nextToWrite(discovery._field, edt_None, eeiCacheDeviceHost).setPtr(&Host::_info);
  else
    cache._block_device._field = discovery._field;
  feeder.nextToWrite(edf__topic, edt_Topic, etitConfig + discovery._component); // device should be cached already
  if (discovery._name != nullptr)
    feeder.nextToWrite(edf_name, edt_String, etisSource0Ptr).setPtr(discovery._name);
  else
    feeder.nextToWrite(edf_name, edt_String, etisSource0CacheField + etisCaseSmart);
  feeder.nextToWrite(edf_state_topic, edt_Topic, etitState + generatorBlock._typeInfo);
  if (discovery._component == edcSwitch || discovery._component == edcButton) {
    feeder.nextToWrite(edf_command_topic, edt_Topic, etitCommand + generatorBlock._typeInfo);
  } else {
    feeder.nextToWrite(edf_unit_of_measurement, edt_String, etisSource0Ptr).setPtr(discovery._measure);
  }
  feeder.nextToWrite(edf_unique_id, edt_String, etisSource0MACId + etisSource1CacheField);
  if (discovery._deviceClass != nullptr) {
    bool useName = strcmp(discovery._deviceClass, "_") == 0;
    feeder.nextToWrite(edf_device_class, edt_String, etisSource0Ptr + (useName ? etisCaseLower : 0)).setPtr((void*)useName ? discovery.getName() : discovery._deviceClass);
  }
  if (discovery._category != eecPrimary) feeder.nextToWrite(edf_entity_category, edt_String, etisSource0Ptr).setPtr(_entityCategory[discovery._category]);
  feeder.nextToWrite(edf_value_template, edt_String, etisSource0FmtPtr + etisSource1CacheField).setPtr("{{ value_json.%s | is_defined }}");
  DataBlock& dev1 = feeder.nextToWrite(edf_device, edt_Device, etidSource0Name + etidSource1Model + etidConnectionsCached + etidIdentifiersCached);
  dev1._value = cache._block1._value;
  if (cache._block2._value.tPtr[0] != nullptr || cache._block2._value.tPtr[1] != nullptr) {
    DataBlock& dev2 = feeder.nextToWrite(edf_device, edt_Device, etidSource0Manufacturer + etidSource1SWVersion);
    dev2._value = cache._block2._value;
    dev2._closeComplexFlag = 1;
  } else {
    dev1._closeComplexFlag = 1;
  }
  feeder.nextToWrite(edf_platform, edt_String, etisSource0Ptr).setPtr("mqtt").closeMessage();
}

void Discovery::generateBlocks(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache) {
  EnumDataField field = generatorBlock._field;
  if (field != edf__discElem && field != edf__discList) return;

  if (!cache._flagDevice) cache.addBlock(generatorBlock, eeiCacheDeviceMainHost);
  if (field == edf__discList) {
    const DiscoveryBlock** list = (const DiscoveryBlock**)generatorBlock._value.tPtr[1];
    for (int idx = 0; *list != nullptr; list++, idx++) generateBlock(feeder, generatorBlock, cache, **list);
  } else {
    generateBlock(feeder, generatorBlock, cache, *(const DiscoveryBlock*)generatorBlock._value.tPtr[1]);
  }
}

const DiscoveryBlock Discovery::_Temperature_C = {edf_tempc, edcSensor, eecPrimary, "Temperature", "°C", "_"};
const DiscoveryBlock Discovery::_Humidity = {edf_hum, edcSensor, eecPrimary, "Humidity", "%", "_"};
const DiscoveryBlock Discovery::_Battery = {edf_batt, edcSensor, eecDiagnostic, "Battery", "%", "_"};
const DiscoveryBlock Discovery::_Volt = {edf_volt, edcSensor, eecDiagnostic, nullptr, "V", nullptr};
const DiscoveryBlock Discovery::_Weight = {edf_weight, edcSensor, eecPrimary, nullptr, "kg", nullptr};
const DiscoveryBlock Discovery::_Uptime_S = {edf_uptime_s, edcSensor, eecDiagnostic, "Uptime", "s", nullptr};
const DiscoveryBlock Discovery::_Uptime_D = {edf_uptime_d, edcSensor, eecDiagnostic, "Uptime", "d", nullptr};
const DiscoveryBlock Discovery::_Free_Memory = {edf_free_memory, edcSensor, eecDiagnostic, nullptr, "B", nullptr};
const DiscoveryBlock Discovery::_Device_Reset = {edf_device_reset, edcButton, eecConfig, "Reset Device", nullptr, nullptr};
const DiscoveryBlock Discovery::_Discovery_Reset = {edf_discovery_reset, edcButton, eecConfig, "Reset Discovery", nullptr, nullptr};
const DiscoveryBlock Discovery::_Connectivity = {edf_connectivity, edcBinarySensor, eecDiagnostic, nullptr, nullptr, "_"};

const DiscoveryBlock* Discovery::_listVoltBattHumTempC[] = {&Discovery::_Volt, &Discovery::_Battery, &Discovery::_Humidity, &Discovery::_Temperature_C, nullptr};

const char* Discovery::_topicConfigComponent[] = {
    "sensor", // edcSensor
    "binary_sensor", // edcBinarySensor
    "switch", // edcSwitch
    "button", // edcButton
};

const char* Discovery::_entityCategory[] = {
    "primary", // eecPrimary
    "config", // eecConfig
    "diagnostic", // eecDiagnostic
    "system", // eecSystem
};

} // namespace stf
