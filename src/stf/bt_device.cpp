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

#include <stf/bt_device.h>
#include <stf/data_buffer.h>
#include <stf/provider_bt.h>

namespace stf {

const BTResolver::ServiceFunction BTResolver::_serviceFunctions[] = {
    &serviceMiBacon,
    &serviceTelinkLYWSD03MMC_atc1441_pvvx,
};

EnumBTResult BTResolver::resolve(DataBuffer* buffer, const BTPacket& packet) {
  for (const ServiceFunction& sf : _serviceFunctions) {
    EnumBTResult res = sf(buffer, packet);
    if (res != EnumBTResult::Unknown) return res;
  }
  return EnumBTResult::Unknown;
}

int32_t BTResolver::getBufferValue(const uint8_t* buffer, uint8_t size) {
  int32_t value = size == 1 ? (int32_t) * (int8_t*)buffer : (size == 2 ? (int32_t) * (int16_t*)buffer : (size == 4 ? *(int32_t*)buffer : 0));
  return value;
}

uint BTResolver::addDiscoveryBlocks(DataBuffer& buffer, const DiscoveryBlock** list, const uint8_t* mac, const char* deviceName, const char* deviceModel, const char* deviceManufacturer, const char* deviceSW) {
  BTDevice* dev = BTProvider::_discoveryList.findOrCreateDevice(mac);
  if (dev == nullptr || dev->_discovery) return 0;
  uint res = Discovery::addBlocks(&buffer, etitBT, list, eeiCacheDeviceMAC48, mac, deviceName, deviceModel, deviceManufacturer, deviceSW);
  if (res == 0) dev->_discovery = true;
  return res;
}

// Service functions
EnumBTResult BTResolver::serviceMiBacon(DataBuffer* buffer, const BTPacket& packet) {
  uint serviceDataLength = 14; // expected minimum length
  const uint8_t* serviceData = packet.getServiceDataByUUID(0xfe95, serviceDataLength);
  if (serviceData == nullptr || serviceDataLength < 14 + serviceData[13]) return EnumBTResult::Unknown;
  if (buffer == nullptr) return EnumBTResult::Resolved;
  uint16_t type = serviceData[2] + (serviceData[3] << 8);

  const char* model = "Unknown";
  switch (type) {
    case 0x01aa:
      model = "LYWSDCGQ";
      if (addDiscoveryBlocks(*buffer, Discovery::_listVoltBattHumTempC + 1, packet._mac, "MiJia ", model, "Xiaomi, Qingping", nullptr) != 0) return EnumBTResult::SmallBuffer;
      break;
    default:
      break;
  }
  uint8_t vlen = serviceData[13];
  switch (serviceData[11]) {
    case 4:
      if (buffer->getFreeBlocks() < 3) return EnumBTResult::SmallBuffer;
      buffer->nextToWrite(edf_tempc, edt_Float, 1).setFloat(getBufferValue(serviceData + 14, vlen) / 10.f);
      break;
    case 6:
      if (buffer->getFreeBlocks() < 3) return EnumBTResult::SmallBuffer;
      buffer->nextToWrite(edf_hum, edt_Float, 1).setFloat(getBufferValue(serviceData + 14, vlen) / 10.f);
      break;
    case 10:
      if (buffer->getFreeBlocks() < 3) return EnumBTResult::SmallBuffer;
      buffer->nextToWrite(edf_batt, edt_32, 0).set32(getBufferValue(serviceData + 14, vlen));
      break;
    case 13:
      if (vlen != 4) return EnumBTResult::Unknown;
      if (buffer->getFreeBlocks() < 3) return EnumBTResult::SmallBuffer;
      buffer->nextToWrite(edf_tempc, edt_Float, 1 + etiDoubleField, edf_hum).setFloat(getBufferValue(serviceData + 14, 2) / 10.f, getBufferValue(serviceData + 16, 2) / 10.f);
      break;

    default:
      return EnumBTResult::Unknown;
      break;
  }
  buffer->nextToWrite(edf__topic, edt_Topic, etitBT, eeiCacheDeviceMAC48).setMAC48(packet._mac);
  buffer->nextToWrite(edf_model, edt_String, 0).setPtr((const char*)model);

  return EnumBTResult::Resolved;
}

// Special firmware for Xiaomi LYWSD03MMC from Telink
// See more at https://github.com/pvvx/ATC_MiThermometer (forked from https://github.com/atc1441/ATC_MiThermometer)
EnumBTResult BTResolver::serviceTelinkLYWSD03MMC_atc1441_pvvx(DataBuffer* buffer, const BTPacket& packet) {
  if (!packet.checkMAC(0, 0xA4, 0xC1, 0x38)) return EnumBTResult::Unknown;
  uint serviceDataLength = 15; // expected minimum length
  const uint8_t* serviceData = packet.getServiceDataByUUID(0x181a, serviceDataLength);
  if (serviceData == nullptr) return EnumBTResult::Unknown;
  if (buffer == nullptr) return EnumBTResult::Resolved;

  const char* model = "LYWSD03MMC";
  if (addDiscoveryBlocks(*buffer, Discovery::_listVoltBattHumTempC, packet._mac, "MiJia ", model, "Xiaomi, Telink", "pvvx") != 0) return EnumBTResult::SmallBuffer;

  if (buffer->getFreeBlocks() < 5) return EnumBTResult::SmallBuffer;
  buffer->nextToWrite(edf__topic, edt_Topic, etitBT, eeiCacheDeviceMAC48).setMAC48(packet._mac);
  buffer->nextToWrite(edf_tempc, edt_Float, 2 + etiDoubleField, edf_hum).setFloat(getBufferValue(serviceData + 6, 2) / 100.f, getBufferValue(serviceData + 8, 2) / 100.f);
  buffer->nextToWrite(edf_batt, edt_32, 0).set32(getBufferValue(serviceData + 12, 1));
  buffer->nextToWrite(edf_volt, edt_Float, 3).setFloat(getBufferValue(serviceData + 10, 2) / 1000.f);
  buffer->nextToWrite(edf_model, edt_String, 0).setPtr((const char*)model);
  return EnumBTResult::Resolved;
}

// BTDeviceGroup

#define STF_BTDEVICE_SIZE 16

BTDeviceGroup::BTDeviceGroup() {}

BTDevice* BTDeviceGroup::findDevice(const uint8_t* mac) {
  uint32_t v4 = *(const uint32_t*)mac;
  uint16_t v2 = *(const uint16_t*)(mac + 4);

  for (BTDevice* arr : _devices) {
    for (int idx = 0; idx < STF_BTDEVICE_SIZE; idx++) {
      BTDevice& dev = arr[idx];
      if (v4 == dev._mac32[0] && v2 == dev._mac16[2]) return &dev;
    }
  }
  return nullptr;
}

BTDevice* BTDeviceGroup::findOrCreateDevice(const uint8_t* mac) {
  BTDevice* dev = findDevice(mac);
  if (dev != nullptr) return dev;
  for (BTDevice* arr : _devices) {
    for (int idx = 0; idx < STF_BTDEVICE_SIZE; idx++) {
      BTDevice& dev = arr[idx];
      if (dev._mac[0] == 0xff) {
        memcpy(dev._mac, mac, 6);
        return &dev;
      }
    }
  }
  BTDevice* arr = new BTDevice[STF_BTDEVICE_SIZE];
  _devices.push_back(arr);
  memcpy(arr[0]._mac, mac, 6);
  return arr;
}

void BTDeviceGroup::updateDevices() {
  if (_lastReadyTime != _discoveryTime) {
    _discoveryTime = _lastReadyTime;
    for (BTDevice* arr : _devices)
      for (int idx = 0; idx < STF_BTDEVICE_SIZE; idx++) arr[idx]._discovery = false;
  }
}

} // namespace stf
