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

#include <stf/provider_bt.h>
#include <stf/data_block.h>
#include <stf/data_discovery.h>
#include <stf/data_feeder.h>
#include <stf/data_cache.h>
#include <stf/bt_device.h>

#include <NimBLEDevice.h>
#include <unordered_set>

namespace stf {

DEFINE_PROVIDERTASK(BTProvider, 4, 0, 0);

BTDeviceGroup BTProvider::_discoveryList;

BTProvider::BTProvider() : Provider(g_bufferBTProvider) {
}

// Coefficients from android-beacon-library
double BTProvider::beaconDistance(int rssi, int txPower) {
  if (txPower >= 0)
    txPower = -59;
  double ratio = rssi * 1.0 / txPower;
  double distance;
  if (ratio < 1.0) {
    distance = pow(ratio, 10);
  } else {
    distance = (0.42093) * pow(ratio, 6.9476) + 0.54992;
  }
  return distance;
}

void BTProvider::generateBTBlocks(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache) {
  feeder.nextToWrite(edf_id, edt_Raw, etirFormatHexUpper + etirSeparatorColon + 6).setRaw(cache._device.info.mac, cache._device.info.macLen);

  int8_t txpw = (int8_t)generatorBlock._extra;
  int8_t rssi = (int8_t)generatorBlock._typeInfo;
  if (txpw != 127) feeder.nextToWrite(edf_txpower, edt_32, 1).set32(txpw);
  if (rssi != 127) {
    feeder.nextToWrite(edf_rssi, edt_32, 1).set32(rssi);
    feeder.nextToWrite(edf_distance, edt_Float, 2).setFloat(beaconDistance(rssi, txpw));
  }

  uint32_t uuid32 = generatorBlock._value.t32[1];
  feeder.nextToWrite(edf_servicedatauuid, edt_Hex32, etihPrefix + (uuid32 <= 0xffff ? 4 : 8)).set32(uuid32);
}

class BTProviderDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void toBuffer(char* buffer_, uint size_, const std::string& str_) {
    uint len = 0;
    for (int jdx = 0, jen = str_.length(); jdx < jen; jdx++) len += snprintf(buffer_ + len, size_ - len, "%02x", (unsigned char)str_[jdx]);
    buffer_[len < size_ ? len : size_ - 1] = 0;
  }

  void onResult(NimBLEAdvertisedDevice* ble_device_) {
    STFLED_COMMAND(STFLEDEVENT_BLE_RECEIVE);

    BTProvider::_discoveryList.updateDevices();
    g_BTProviderObj._packetsScanned++;

    // The NimBLE interface seems to like copy and alloc as much as possible...
    NimBLEAddress addr = ble_device_->getAddress();
    const uint8_t* macNat = addr.getNative();
    uint8_t mac[6];
    uint8_t macType = addr.getType();
    for (int idx = 0; idx < 6; idx++) mac[idx] = macNat[5 - idx]; // Reverse it to the proper order..

      // For test - filter
#ifdef STFBLE_TEST_MAC
    if (mac[5] != (STFBLE_TEST_MAC)) return;
#endif
#ifdef STFBLE_TEST_XOR
    // Not perfect as service data may contain the mac and we don't change that
    mac[5] ^= STFBLE_TEST_XOR;
#endif

    int statFreeBlocksBegin = g_bufferBTProvider->getFreeBlocks();
    uint sdCount = ble_device_->getServiceDataCount();
    for (uint sdidx = 0; sdidx < sdCount; sdidx++) {
      std::string data = ble_device_->getServiceData(sdidx);
      NimBLEUUID uuid = ble_device_->getServiceDataUUID(sdidx);

      uint8_t bs = uuid.bitSize();
      EnumBTResult res = EnumBTResult::Unknown;
      if (bs == 16 || bs == 32) { // We support only 16 or 32 bit uuid
        uint32_t uuid32 = bs == 16 ? (uint32_t)uuid.getNative()->u16.value : uuid.getNative()->u32.value;
        const uint8_t* serviceDataBuffer = (const uint8_t*)data.data();
        uint serviceDataLength = data.length();
        res = BTResolver::resolve(mac, macType, uuid32, serviceDataBuffer, serviceDataLength, *g_bufferBTProvider);
        if (res != EnumBTResult::Resolved) {
          const char* msg = res == EnumBTResult::Unknown ? "Unknown " : "No buffer for the";
          STFLOG_WARNING("%s message type %0x from %02x%02x%02x%02x%02x%02x\n", msg, uuid32, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        } else {
          // Finish the buffer
          uint freeBlocks = g_bufferBTProvider->getFreeBlocks();

          if (freeBlocks >= 1) { // Generator function for 4-5 elements
            int8_t txpw = ble_device_->haveTXPower() ? ble_device_->getTXPower() : 127;
            int rssiInt = ble_device_->haveRSSI() ? ble_device_->getRSSI() : 127;
            int8_t rssi = rssiInt < -128 ? -128 : (rssiInt > 127 ? 127 : (int8_t)rssiInt);
            DataBlock& genBlock = g_bufferBTProvider->nextToWrite(edf__none, edt_Generator, rssi).setPtr((const void*)&BTProvider::generateBTBlocks);
            genBlock._value.t32[1] = uuid32;
            genBlock._extra = txpw;
          }

          if (freeBlocks >= 1 && freeBlocks >= (serviceDataLength + 8) / 9) {
            EnumDataField fld = edf_servicedata;
            for (uint cpy; serviceDataLength > 0 || fld == edf_servicedata; serviceDataBuffer += cpy, serviceDataLength -= cpy, fld = edf__cont) {
              cpy = serviceDataLength > sizeof(DataBlock::_value) + sizeof(DataBlock::_extra) ? sizeof(DataBlock::_value) + sizeof(DataBlock::_extra) : serviceDataLength;
              g_bufferBTProvider->nextToWrite(fld, edt_Raw, cpy + etirFormatHexLower).setRaw(serviceDataBuffer, cpy);
            }
            //freeBlocks = btBuffer.getFreeBlocks();
          }

          int statFreeBlocksEnd = g_bufferBTProvider->getFreeBlocks();
          g_bufferBTProvider->closeMessage();
          g_BTProviderObj._packetsForwarded++;
          STFLOG_INFO("Total blocks used for the BT messages: %u\n", statFreeBlocksBegin - statFreeBlocksEnd); // not correct, will need fix
        }
      }
    }

    char buffer[260];
    toBuffer(buffer, sizeof(buffer), ble_device_->getManufacturerData());
    Serial.printf("Address: %s - Name: %s - Manufacturer: %s - RSSI: %d - UUID count: %u - Payload: %u\n",
                  ble_device_->getAddress().toString().c_str(),
                  ble_device_->getName().c_str(),
                  buffer,
                  ble_device_->getRSSI(),
                  ble_device_->getServiceUUIDCount(),
                  (uint)ble_device_->getPayloadLength());
    int cnt = ble_device_->getServiceDataCount();
    for (int idx = 0; idx < cnt; idx++) {
      NimBLEUUID uuid = ble_device_->getServiceDataUUID(idx);
      uint bs = uuid.bitSize();
      std::string str = ble_device_->getServiceData(idx);
      toBuffer(buffer, sizeof(buffer), str);
      Serial.printf("Address: %s UUID (%d) %s DATA (%d) %s\n", ble_device_->getAddress().toString().c_str(), bs, std::string(uuid).c_str(), str.length(), buffer);
    }
  }
} g_bleCallback;

void BTProvider::setup() {
  if (NimBLEDevice::getInitialized()) return;
  NimBLEDevice::setScanFilterMode(2);
  NimBLEDevice::setScanDuplicateCacheSize(200);
  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan(); // singleton, we don't need to store it anywhere
  scan->setActiveScan(false); // We don't support the extra packets: drains battery on the other side, kills wifi on this side, let's forget it...
  //scan->setInterval(52); // How often the scan occurs / switches channels; in milliseconds,
  scan->setInterval(97); // How often the scan occurs / switches channels; in milliseconds,
  scan->setWindow(30); // How long to scan during the interval; in milliseconds.
  scan->setMaxResults(0); // do not store the scan results, use callback only.
  scan->setAdvertisedDeviceCallbacks(&g_bleCallback, false);
  _packetLastReset = 0;
  _packetsScanned = _packetsForwarded = 0;
}

uint BTProvider::loop() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  bool isScanning = scan->isScanning();
  if (isConsumerReady()) {
    uint32_t readyTime = g_bufferBTProvider->getConsumer()->readyTime();
    if (_discoveryList._connectionTime < readyTime) _discoveryList._connectionTime = readyTime;
    if (!isScanning) scan->start(0, nullptr, false);
  } else {
    if (isScanning) scan->stop();
  }

  return 50;
}

const DiscoveryBlock BTProvider::_received = {edf_bt_scanned, edcSensor, "BT Packets Scanned", "Hz", nullptr};
const DiscoveryBlock BTProvider::_transmitted = {edf_bt_forwarded, edcSensor, "BT Packets Forwarded", "Hz", nullptr};
const DiscoveryBlock* BTProvider::_listSystem[] = {&_received, &_transmitted, nullptr};

uint BTProvider::systemDiscovery(DataBuffer* systemBuffer) {
  uint res = Discovery::addDiscoveryBlocks(systemBuffer, etitSYS, _listSystem);
  return res;
}

uint BTProvider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS) {
  if (systemBuffer == nullptr) return 2;
  float ellapsed = uptimeS == _packetLastReset ? 0.1f : (uptimeS - _packetLastReset);
  systemBuffer->nextToWrite(edf_bt_scanned, edt_Float, 3).setFloat(_packetsScanned / ellapsed);
  systemBuffer->nextToWrite(edf_bt_forwarded, edt_Float, 3).setFloat(_packetsForwarded / ellapsed);
  _packetsScanned = _packetsForwarded = 0; // we have a very low chance to lose 1 packet from the statistics due to concurrency, that's ok
  _packetLastReset = uptimeS;
  return 0;
}

} // namespace stf
