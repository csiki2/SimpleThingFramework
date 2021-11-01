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

class BTProvider : public Provider {
public:
  BTProvider();

  uint loop() override;
  void setup() override;
  uint systemDiscovery(DataBuffer* systemBuffer) override;
  uint systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS) override;

  static double beaconDistance(int rssi, int txPower);
  static void generateBTBlocks(DataFeeder& feeder, const DataBlock& generatorBlock, DataCache& cache);

  uint32_t _packetLastReset;
  volatile uint16_t _packetsScanned;
  volatile uint16_t _packetsForwarded;

  static const DiscoveryBlock _received;
  static const DiscoveryBlock _transmitted;

  static BTDeviceGroup _discoveryList;
  static const DiscoveryBlock* _listSystem[];
};

} // namespace stf
