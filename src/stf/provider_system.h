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
#include <stf/util.h>

namespace stf {

class DataFeeder;
class DiscoveryBlock;

class SystemProvider : public Provider {
public:
  SystemProvider();

  uint loop() override;
  uint systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) override;

  static void requestRetainedReport();

  static const DiscoveryBlock* _listSystemNormal[];
  static const DiscoveryBlock* _listSystemRetained[];

protected:
  bool generateSystemReport(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type);

  uint32_t _lastSystemReportTime = 0;
  EnumClassFlags<ESystemMessageType> _reportRequired;
  bool _forceSystemRetainedReport = false;
};

} // namespace stf
