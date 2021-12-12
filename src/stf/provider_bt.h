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

#include <stf/provider.h>

namespace stf {

class DiscoveryBlock;
class BTDeviceGroup;
class DataFeeder;

class BTPacket {
public:
  BTPacket(uint8_t advType, uint8_t macType, const uint8_t* mac, const uint8_t* payload, uint payloadLength, int rssi);

  void log(const char* extraMsg = "", int level = STFLOG_LEVEL_INFO, bool endl = true) const;
  void logPayload(int level = STFLOG_LEVEL_INFO, bool endl = true) const;

  // len input: expected minimum length, len output: result length
  const uint8_t* getServiceDataByUUID(uint32_t uuid, uint& len) const;
  const uint8_t* getField(uint8_t type, uint& len, int idx = 0);
  uint countField(uint8_t type);
  bool checkMAC(uint8_t type, uint8_t m0, uint8_t m1, uint8_t m2) const;

  const uint8_t* _payloadBuffer;
  uint _payloadLength;

  uint8_t _mac[6];
  uint8_t _macType;
  uint8_t _advType;
  int8_t _rssi;
};

class BTProvider : public Provider {
public:
  BTProvider();

  uint loop() override;
  void setup() override;

  uint systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) override;
  void feedback(const FeedbackInfo& info) override;

  static double beaconDistance(int rssi, int txPower);
  static void generateBTBlocks(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache);

  uint32_t _packetLastReset;
  volatile uint16_t _packetsScanned;
  volatile uint16_t _packetsForwarded;
  bool _packetsFilterUnknown = true;
  bool _forceDiscoveryReset = false;

  static const DiscoveryBlock _received;
  static const DiscoveryBlock _transmitted;
  static const DiscoveryBlock _filterUnknown;

  static BTDeviceGroup _discoveryList;
  static const DiscoveryBlock* _listSystemNormal[];
  static const DiscoveryBlock* _listSystemRetained[];
};

} // namespace stf
