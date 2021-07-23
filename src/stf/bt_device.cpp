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
#include <stf/data_discovery.h>
#include <stf/data_buffer.h>
#include <stf/provider_bt.h>

namespace stf {

int32_t GetBufferValue(const uint8_t* buff_, uint8_t size_) {
  int32_t value = size_ == 1 ? (int32_t) * (int8_t*)buff_ : (size_ == 2 ? (int32_t) * (int16_t*)buff_ : (size_ == 4 ? *(int32_t*)buff_ : 0));
  return value;
}

EnumBTResult BTServiceMiBacon(const uint8_t* mac_, uint8_t macType_, uint32_t uuid_, const uint8_t* serviceData_, uint serviceDataLength_, DataBuffer& buffer_) {
  if (serviceDataLength_ < 14 || serviceDataLength_ < 14 + serviceData_[13]) return ebtrWrongType;
  uint16_t type = serviceData_[2] + (serviceData_[3] << 8);

  const char* model = "Unknown";
  BTDevice* dev = nullptr;
  switch (type) {
    case 0x01aa:
      model = "LYWSDCGQ";
      dev = BTProvider::_discoveryList.findOrCreateDevice(mac_);
      if (dev != nullptr && !dev->_discovery) {
        if (Discovery::addDiscoveryBlocks(&buffer_, etitBTtoMQTT, Discovery::_listBattHumTempC, etigCacheDeviceMAC48, mac_, "MiJia ", model, "Xiaomi, Qingping", nullptr) == 0)
          dev->_discovery = true;
        else
          return ebtrSmallBuffer;
      }
      break;
    default:
      break;
  }
  uint8_t vlen = serviceData_[13];
  switch (serviceData_[11]) {
    case 4:
      if (buffer_.getFreeBlocks() < 3) return ebtrSmallBuffer;
      buffer_.nextToWrite(edf_tempc, edt_Float, 1).setFloat(GetBufferValue(serviceData_ + 14, vlen) / 10.f);
      break;
    case 6:
      if (buffer_.getFreeBlocks() < 3) return ebtrSmallBuffer;
      buffer_.nextToWrite(edf_hum, edt_Float, 1).setFloat(GetBufferValue(serviceData_ + 14, vlen) / 10.f);
      break;
    case 10:
      if (buffer_.getFreeBlocks() < 3) return ebtrSmallBuffer;
      buffer_.nextToWrite(edf_batt, edt_32, 0).set32(GetBufferValue(serviceData_ + 14, vlen));
      break;
    case 13:
      if (vlen != 4) return ebtrWrongType;
      if (buffer_.getFreeBlocks() < 3) return ebtrSmallBuffer;
      buffer_.nextToWrite(edf_tempc, edt_Float, 1 + etigDoubleField).setFloat(GetBufferValue(serviceData_ + 14, 2) / 10.f, GetBufferValue(serviceData_ + 16, 2) / 10.f)._extra = edf_hum;
      //buffer_.nextToWrite(edf_hum, edt_Float, 1).setFloat(GetBufferValue(serviceData_ + 16, 2) / 10.f);
      break;

    default:
      return ebtrWrongType;
      break;
  }
  buffer_.nextToWrite(edf__topic, edt_Topic, etitBTtoMQTT + etigCacheDeviceMAC48).setMAC48(mac_);
  buffer_.nextToWrite(edf_model, edt_String, 0).setPtr((const char*)model);

  return ebtrResolved;
}

namespace BT {
const BTServiceType _serviceFunction[] = {
    {0xfe95, &BTServiceMiBacon}};
const uint _serviceFunctionLength = sizeof(_serviceFunction) / sizeof(BTServiceType);
} // namespace BT

// BTDeviceGroup

#define STF_BTDEVICE_SIZE 16

BTDeviceGroup::BTDeviceGroup() {
  _connectionTime = _discoveryTime = 0;
}

BTDevice* BTDeviceGroup::findDevice(const uint8_t* mac_) {
  uint32_t v4 = *(const uint32_t*)mac_;
  uint16_t v2 = *(const uint16_t*)(mac_ + 4);

  for (BTDevice* arr : _devices) {
    for (int idx = 0; idx < STF_BTDEVICE_SIZE; idx++) {
      BTDevice& dev = arr[idx];
      if (v4 == dev._mac32[0] && v2 == dev._mac16[2]) return &dev;
    }
  }
  return nullptr;
}

BTDevice* BTDeviceGroup::findOrCreateDevice(const uint8_t* mac_) {
  BTDevice* dev = findDevice(mac_);
  if (dev != nullptr) return dev;
  for (BTDevice* arr : _devices) {
    for (int idx = 0; idx < STF_BTDEVICE_SIZE; idx++) {
      BTDevice& dev = arr[idx];
      if (dev._mac[0] == 0xff) {
        memcpy(dev._mac, mac_, 6);
        return &dev;
      }
    }
  }
  BTDevice* arr = new BTDevice[STF_BTDEVICE_SIZE];
  _devices.push_back(arr);
  memcpy(arr[0]._mac, mac_, 6);
  return arr;
}

void BTDeviceGroup::updateDevices() {
  if (_connectionTime != _discoveryTime) {
    _discoveryTime = _connectionTime;
    for (BTDevice* arr : _devices)
      for (int idx = 0; idx < STF_BTDEVICE_SIZE; idx++) arr[idx]._discovery = false;
  }
}

} // namespace stf
