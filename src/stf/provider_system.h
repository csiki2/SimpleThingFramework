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
  void feedback(const FeedbackInfo& info) override;

  static void requestRetainedReport();
  static inline bool isLedEnabled() { return _obj._enableLed; }

protected:
  static SystemProvider _obj;
  static const DiscoveryBlock* _listSystemNormal[];
  static const DiscoveryBlock* _listSystemRetained[];
  static const DiscoveryBlock* _listSystemConnectivity[];

  bool generateSystemReport(DataBuffer* systemBuffer, ESystemMessageType type);

  EnumClassFlags<ESystemMessageType> _reportRequired;
  ElapsedTime _lastSystemReportTime;
  bool _forceRetainedReport = false;
  bool _forceDiscoveryReset = false;

  bool _enableLed = true;

  static const DiscoveryBlock _discoveryLed;
};

} // namespace stf
