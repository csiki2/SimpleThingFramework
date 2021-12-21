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

#include <stf/data_block.h>
#include <stf/data_feeder.h>
#include <stf/data_discovery.h>
#include <stf/provider_system.h>

namespace stf {

SystemProvider SystemProvider::_obj;

SystemProvider::SystemProvider() : Provider(g_bufferSystemProvider) {
}

const DiscoveryBlock SystemProvider::_discoveryLed = {edf_led, edcSwitch, eecConfig, "LED", nullptr, nullptr};

const DiscoveryBlock* SystemProvider::_listSystemNormal[] = {&Discovery::_Device_Reset, &Discovery::_Discovery_Reset, &Discovery::_Uptime_S, &Discovery::_Uptime_D, &Discovery::_Free_Memory, nullptr};
const DiscoveryBlock* SystemProvider::_listSystemRetained[] = {&_discoveryLed, nullptr};

uint SystemProvider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  uint res = 0;
  switch (type) {
    case ESystemMessageType::Discovery:
      res = Discovery::addBlocks(systemBuffer, etitSYS, _listSystemNormal, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
      res += Discovery::addBlocks(systemBuffer, etitSYSR, _listSystemRetained, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
      res += Discovery::addBlock(systemBuffer, etitCONN, Discovery::_Connectivity, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
      break;
    case ESystemMessageType::Normal:
      res = 4;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf__topic, edt_Topic, etitSYS, eeiCacheDeviceHost).setPtr(&Host::_info);
        systemBuffer->nextToWrite(edf_uptime_s, edt_32, 0 + etiDoubleField, edf_uptime_d).set32(uptimeS, uptimeS / (24 * 60 * 60));
        systemBuffer->nextToWrite(edf_free_memory, edt_32, 0).set32(ESP.getFreeHeap());
        systemBuffer->nextToWrite(edf_ip, edt_Raw, 4 + etirSeparatorDot + etirFormatNumber).setRaw(Host::_ip4, 4);
        res = 0;
      }
      break;
    case ESystemMessageType::Retained:
      res = 2;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf__topic, edt_Topic, etitSYSR + etitRetain, eeiCacheDeviceHost).setPtr(&Host::_info);
        systemBuffer->nextToWrite(edf_led, edt_String, etisSource0Ptr).setPtr(_enableLed ? "ON" : "OFF");
        res = 0;
      }
    default:
      break;
  }
  return res;
}

void SystemProvider::feedback(const FeedbackInfo& info) {
  if (handleSimpleFeedback(info, Discovery::_Device_Reset, Host::_info.mac, Host::_info.macLen, nullptr)) ESP.restart();
  handleSimpleFeedback(info, Discovery::_Discovery_Reset, Host::_info.mac, Host::_info.macLen, &_forceDiscoveryReset);
  handleSimpleFeedback(info, _discoveryLed, Host::_info.mac, Host::_info.macLen, &_enableLed);
}

void SystemProvider::requestRetainedReport() {
  SystemProvider::_obj._forceRetainedReport = true;
}

uint SystemProvider::loop() {
  const uint waitTime = 50, updateTimeMS = 120000;

  Consumer* cons = g_bufferSystemProvider->getConsumer();
  if (cons == nullptr || !cons->isReady()) return waitTime;

  if (_forceRetainedReport) {
    _forceRetainedReport = false;
    _reportRequired += ESystemMessageType::Retained;
  }
  if (_forceDiscoveryReset) {
    _forceDiscoveryReset = false;
    _reportRequired += ESystemMessageType::Discovery;
  }

  uint32_t reportTime = _lastSystemReportTime.elapsedTime();
  if (reportTime > cons->ellapsedTimeSinceReady()) {
    _reportRequired = ESystemMessageType::All;
  } else if (reportTime >= updateTimeMS) {
    _reportRequired += ESystemMessageType::Normal;
    _reportRequired += ESystemMessageType::Retained;
  }

  if (_reportRequired.isEmpty()) return waitTime; // no report is needed
  if (_reportRequired.contains(ESystemMessageType::Normal)) _lastSystemReportTime.reset();

  for (ESystemMessageType nxt = ESystemMessageType::None; _reportRequired.next(nxt);) {
    if (generateSystemReport(g_bufferSystemProvider, nxt)) _reportRequired -= nxt;
  }

  return waitTime;
}

bool SystemProvider::generateSystemReport(DataBuffer* systemBuffer, ESystemMessageType type) {
  int sum = 0;
  uint32_t uptimeS = Host::uptimeSec32();
  for (Provider* p = _providerHead; p != nullptr; p = (Provider*)p->_objectNext)
    sum += p->systemUpdate(nullptr, uptimeS, type);
  if (g_bufferSystemProvider->getFreeBlocks() < sum) return false;

  systemUpdate(g_bufferSystemProvider, uptimeS, type); // always SystemProvider is the first
  for (Provider* p = _providerHead; p != nullptr; p = (Provider*)p->_objectNext)
    if (p != this) p->systemUpdate(g_bufferSystemProvider, uptimeS, type);
  g_bufferSystemProvider->closeMessage();
  return true;
}

} // namespace stf
