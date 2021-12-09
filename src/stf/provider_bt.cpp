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
#include <stf/util.h>

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

  feeder.nextToWrite(edf_bt_adv_type, edt_32, 1).set32(generatorBlock._value.t8[4]);
  feeder.nextToWrite(edf_bt_addr_type, edt_32, 1).set32(generatorBlock._value.t8[5]);
}

class BTProviderDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* bleDevice) {
    STFLED_COMMAND(STFLEDEVENT_BLE_RECEIVE);

    BTProvider::_discoveryList.updateDevices();
    g_BTProviderObj._packetsScanned++;

    NimBLEAddress addr = bleDevice->getAddress();
    BTPacket packet(bleDevice->getAdvType(), addr.getType(), addr.getNative(), bleDevice->getPayload(), bleDevice->getPayloadLength(), bleDevice->haveRSSI() ? bleDevice->getRSSI() : 127);

    // For test - filter
    DataBuffer* resolveBuffer = g_bufferBTProvider;
#ifdef STFBLE_TEST_MAC
    if (packet._mac[5] != (STFBLE_TEST_MAC)) resolveBuffer = nullptr; //return;
#endif
#ifdef STFBLE_TEST_XOR
    // Not perfect as service data may contain the mac and we don't change that
    packet._mac[5] ^= STFBLE_TEST_XOR;
#endif

    int statFreeBlocksBegin = g_bufferBTProvider->getFreeBlocks();
    EnumBTResult res = BTResolver::resolve(resolveBuffer, packet);
    if (res == EnumBTResult::Resolved && resolveBuffer == nullptr) res = EnumBTResult::Disabled;

    static const char* resMsg[] = {"(Resolved)", "(Unknown) ", "(NoBuffer)", "(Disabled)", "(InvalidR)"};
    packet.log(resMsg[res >= EnumBTResult::Resolved && res <= EnumBTResult::Disabled ? (int)res : 1]);
    if (res == EnumBTResult::Disabled) return;

    uint freeBlocks = g_bufferBTProvider->getFreeBlocks();
    if (!g_BTProviderObj._packetsFilterUnknown && res == EnumBTResult::Unknown && freeBlocks >= 1) {
      g_bufferBTProvider->nextToWrite(edf__topic, edt_Topic, etitBT, eeiCacheDeviceMAC48).setMAC48(packet._mac);
      res = EnumBTResult::Resolved;
      freeBlocks--;
    }

    if (res == EnumBTResult::Resolved) { // Finish the buffer
      if (freeBlocks >= 1) { // Generator function for extra elements
        uint len;
        const uint8_t* field = packet.getField(0x0a, len); // TXPower
        int8_t txpw = len == 1 ? field[0] : 127;
        DataBlock& genBlock = g_bufferBTProvider->nextToWrite(edf__none, edt_Generator, packet._rssi).setPtr((const void*)&BTProvider::generateBTBlocks);
        genBlock._extra = txpw;
        genBlock._value.t8[4] = packet._advType;
        genBlock._value.t8[5] = packet._macType;
        freeBlocks--;
      }
      if (freeBlocks >= 1 && freeBlocks >= (packet._payloadLength + 8) / 9) {
        EnumDataField fld = edf_bt_payload;
        uint len = packet._payloadLength;
        const uint8_t* buff = packet._payloadBuffer;
        for (uint cpy; len > 0 || fld == edf_bt_payload; buff += cpy, len -= cpy, fld = edf__cont) {
          cpy = len > sizeof(DataBlock::_value) + sizeof(DataBlock::_extra) ? sizeof(DataBlock::_value) + sizeof(DataBlock::_extra) : len;
          g_bufferBTProvider->nextToWrite(fld, edt_Raw, cpy + etirFormatHexLower).setRaw(buff, cpy);
        }
      }
      int statFreeBlocksEnd = g_bufferBTProvider->getFreeBlocks();
      g_bufferBTProvider->closeMessage();
      g_BTProviderObj._packetsForwarded++;
      STFLOG_INFO("Total blocks used for the BT messages: %u\n", statFreeBlocksBegin - statFreeBlocksEnd); // not correct, will need fix
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
    uint32_t consumerTime = g_bufferBTProvider->getConsumer()->ellapsedTimeSinceReady();
    uint32_t discoveryTime = _discoveryList._lastReadyTime.elapsedTime();
    if (discoveryTime > consumerTime) _discoveryList._lastReadyTime.reset();
    if (!isScanning) scan->start(0, nullptr, false);
  } else {
    if (isScanning) scan->stop();
  }

  return 50;
}

const DiscoveryBlock BTProvider::_received = {edf_bt_scanned, edcSensor, eecDiagnostic, "BT Packets Scanned", "Hz", nullptr};
const DiscoveryBlock BTProvider::_transmitted = {edf_bt_forwarded, edcSensor, eecDiagnostic, "BT Packets Forwarded", "Hz", nullptr};
const DiscoveryBlock BTProvider::_filterUnknown = {edf_bt_filter_unknown, edcSwitch, eecConfig, "BT Filter Unknown Messages", nullptr, nullptr};
const DiscoveryBlock* BTProvider::_listSystemNormal[] = {&_received, &_transmitted, nullptr};
const DiscoveryBlock* BTProvider::_listSystemRetained[] = {&_filterUnknown, nullptr};

uint BTProvider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  uint res = 0;
  switch (type) {
    case ESystemMessageType::Discovery:
      res = Discovery::addBlocks(systemBuffer, etitSYS, _listSystemNormal);
      res += Discovery::addBlocks(systemBuffer, etitSYSR, _listSystemRetained);
      break;
    case ESystemMessageType::Retained:
      res = 1;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf_bt_filter_unknown, edt_String, etisSource0Ptr).setPtr(_packetsFilterUnknown ? "ON" : "OFF");
        res = 0;
      }
      break;

    case ESystemMessageType::Normal:
      res = 2;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        float ellapsed = uptimeS == _packetLastReset ? 0.1f : (uptimeS - _packetLastReset);
        systemBuffer->nextToWrite(edf_bt_scanned, edt_Float, 3).setFloat(_packetsScanned / ellapsed);
        systemBuffer->nextToWrite(edf_bt_forwarded, edt_Float, 3).setFloat(_packetsForwarded / ellapsed);
        _packetsScanned = _packetsForwarded = 0; // we have a very low chance to lose 1 packet from the statistics due to concurrency, that's ok
        _packetLastReset = uptimeS;
        res = 0;
      }
      break;
    default:
      break;
  }
  return res;
}

void BTProvider::feedback(const FeedbackInfo& info) {
  handleSimpleFeedback(info, "BT FilterUnknown", edf_bt_filter_unknown, _packetsFilterUnknown);
}

// BTPacket
BTPacket::BTPacket(uint8_t advType, uint8_t macType, const uint8_t* mac, const uint8_t* payload, uint payloadLength, int rssi) {
  _payloadBuffer = payload;
  _payloadLength = payloadLength;
  for (int idx = 0; idx < 6; idx++) _mac[idx] = mac[5 - idx]; // Reverse it to the proper order..
  _macType = macType;
  _advType = advType;
  _rssi = rssi < -128 ? -128 : (rssi > 127 ? 127 : (int8_t)rssi);
}

void BTPacket::log(const char* extraMsg, int level, bool endl) const {
  if (STFLOG_LEVEL < level) return;

  STFLOG_PRINT("BT packet %s AdvType: %u - AddrType %u", extraMsg, _advType, _macType);
  STFLOG_PRINT(" - Address: %02x:%02x:%02x:%02x:%02x:%02x - RSSI: %4d - Payload(%u): ",
               _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5], _rssi, _payloadLength);
  logPayload(level, endl);
}

void BTPacket::logPayload(int level, bool endl) const {
  if (STFLOG_LEVEL < level) return;
  const uint8_t* buffer = _payloadBuffer;
  uint len = _payloadLength, slen, hlen;
  for (; 0 < len; buffer += hlen, len -= hlen) {
    slen = 1 + *buffer;
    hlen = slen <= len ? slen : len;
    Util::writeHexToLog(buffer, hlen);
    if (slen != len) STFLOG_WRITE(slen < len ? ' ' : '.');
  }
  if (endl) STFLOG_WRITE('\n');
}

const uint8_t* BTPacket::getServiceDataByUUID(uint32_t uuid, uint& len) const {
  uint16_t u16 = uuid < 65536 ? (uint16_t)uuid : 0; // 0 is invalid uuid16
  uint buffLen = _payloadLength;
  const uint8_t* buffPtr = _payloadBuffer;
  while (buffLen >= 4) {
    uint slen = buffPtr[0] + 1;
    if (slen > buffLen) slen = buffLen;
    if (slen >= len + 4 && buffPtr[1] == 0x16 && u16 == *(uint16_t*)(buffPtr + 2)) {
      len = slen - 4;
      return buffPtr + 4;
    }
    if (slen >= len + 6 && buffPtr[1] == 0x20 && uuid == *(uint32_t*)(buffPtr + 2)) {
      len = slen - 6;
      return buffPtr + 6;
    }
    buffPtr += slen;
    buffLen -= slen;
  }
  len = 0;
  return nullptr;
}

const uint8_t* BTPacket::getField(uint8_t type, uint& len, int idx) {
  uint buffLen = _payloadLength;
  const uint8_t* buffPtr = _payloadBuffer;
  while (buffLen >= 2) {
    uint slen = buffPtr[0] + 1;
    if (slen > buffLen) slen = buffLen;
    if (slen >= 2 && buffPtr[1] == type && 0 == idx--) {
      len = slen - 2;
      return buffPtr + 2;
    }
    buffPtr += slen;
    buffLen -= slen;
  }
  len = 0;
  return nullptr;
}

uint BTPacket::countField(uint8_t type) {
  uint buffLen = _payloadLength, count = 0;
  const uint8_t* buffPtr = _payloadBuffer;
  while (buffLen >= 2) {
    uint slen = buffPtr[0] + 1;
    if (slen > buffLen) slen = buffLen;
    if (slen >= 1 && buffPtr[1] == type) count++;
    buffPtr += slen;
    buffLen -= slen;
  }
  return count;
}

bool BTPacket::checkMAC(uint8_t type, uint8_t m0, uint8_t m1, uint8_t m2) const {
  return _macType == type && _mac[0] == m0 && _mac[1] == m1 && _mac[2] == m2;
}

} // namespace stf
