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

DEFINE_PROVIDERTASK(SystemProvider, 3, 0, 0);

SystemProvider::SystemProvider() : Provider(g_bufferSystemProvider) {
}

const DiscoveryBlock* SystemProvider::_listSystemNormal[] = {&Discovery::_Uptime_S, &Discovery::_Uptime_D, &Discovery::_Free_Memory, nullptr};
const DiscoveryBlock* SystemProvider::_listSystemRetained[] = {&Discovery::_Free_Memory, nullptr};

uint SystemProvider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  uint res = 0;
  switch (type) {
    case ESystemMessageType::Discovery:
      res = Discovery::addBlocks(systemBuffer, etitSYS, _listSystemNormal, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
      res += Discovery::addBlocks(systemBuffer, etitSYSR, _listSystemRetained, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
      break;
    case ESystemMessageType::Normal:
      res = 3;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf__topic, edt_Topic, etitSYS, eeiCacheDeviceHost).setPtr(&Host::_info);
        systemBuffer->nextToWrite(edf_uptime_s, edt_32, 0 + etiDoubleField, edf_uptime_d).set32(uptimeS, uptimeS / (24 * 60 * 60));
        systemBuffer->nextToWrite(edf_ip, edt_Raw, 4 + etirSeparatorDot + etirFormatNumber).setRaw(Host::_ip4, 4);
        res = 0;
      }
      break;
    case ESystemMessageType::Retained:
      res = 2;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf__topic, edt_Topic, etitSYSR + etitRetain, eeiCacheDeviceHost).setPtr(&Host::_info);
        systemBuffer->nextToWrite(edf_free_memory, edt_32, 0).set32(ESP.getFreeHeap());
        res = 0;
      }
    default:
      break;
  }
  return res;
}

void SystemProvider::requestRetainedReport() {
  g_SystemProviderObj._forceSystemRetainedReport = true;
}

uint SystemProvider::loop() {
  const uint waitTime = 50, updateTimeS = 120;

  Consumer* cons = g_bufferSystemProvider->getConsumer();
  if (cons == nullptr || !cons->isReady()) return waitTime;
  uint32_t uptime = Host::uptimeSec32();

  if (_forceSystemRetainedReport) {
    _forceSystemRetainedReport = false;
    _reportRequired = (ESystemMessageType)((uint8_t)_reportRequired | (uint8_t)ESystemMessageType::Retained);
  }
  if (_lastSystemReportTime < cons->readyTime()) {
    _reportRequired = ESystemMessageType::All;
  } else if (_lastSystemReportTime + updateTimeS <= uptime) {
    _reportRequired = (ESystemMessageType)((uint8_t)_reportRequired | (uint8_t)ESystemMessageType::Normal | (uint8_t)ESystemMessageType::Retained);
  }

  if (_reportRequired == ESystemMessageType::None) return waitTime; // no report is needed
  if (((uint8_t)_reportRequired & (uint8_t)ESystemMessageType::Normal) != 0) _lastSystemReportTime = uptime;

  for (uint8_t type = 1; (type & (uint8_t)ESystemMessageType::All) != 0; type <<= 1) {
    if ((type & (uint8_t)_reportRequired) != 0 && generateSystemReport(g_bufferSystemProvider, uptime, (ESystemMessageType)type))
      _reportRequired = (ESystemMessageType)((uint8_t)_reportRequired & ~type);
  }

  return waitTime;
}

bool SystemProvider::generateSystemReport(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  int sum = 0;
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
