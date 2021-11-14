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

const DiscoveryBlock* SystemProvider::_listSystem[] = {&Discovery::_Uptime_S, &Discovery::_Uptime_D, &Discovery::_Free_Memory, nullptr};

uint SystemProvider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  uint res = 0;
  switch (type) {
    case ESystemMessageType::Discovery:
      res = Discovery::addBlocks(systemBuffer, etitSYS, _listSystem, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
      break;
    case ESystemMessageType::Normal:
      res = 4;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf__topic, edt_Topic, etitSYS, eeiCacheDeviceHost).setPtr(&Host::_info);
        systemBuffer->nextToWrite(edf_uptime_s, edt_32, 0 + etiDoubleField, edf_uptime_d).set32(uptimeS, uptimeS / (24 * 60 * 60));
        systemBuffer->nextToWrite(edf_ip, edt_Raw, 4 + etirSeparatorDot + etirFormatNumber).setRaw(Host::_ip4, 4);
        systemBuffer->nextToWrite(edf_free_memory, edt_32, 0).set32(ESP.getFreeHeap());
        res = 0;
      }
      break;
    default:
      break;
  }
  return res;
}

void SystemProvider::requestReport() {
  g_SystemProviderObj._forceSystemReport = true;
}

uint SystemProvider::loop() {
  const uint waitTime = 50, updateTimeS = 120;

  Consumer* cons = g_bufferSystemProvider->getConsumer();
  if (cons == nullptr || !cons->isReady()) return waitTime;
  uint32_t uptime = Host::uptimeSec32();
  if (_forceSystemReport || _lastSystemReportTime < cons->readyTime() || _lastSystemReportTime + updateTimeS <= uptime) {
    _lastSystemReportSuccess = ESystemMessageType::None;
    _forceSystemReport = false;
  }

  if (_lastSystemReportSuccess == ESystemMessageType::All) return waitTime; // no report is needed
  if (_lastSystemReportSuccess == ESystemMessageType::None) _lastSystemReportTime = uptime;

  for (uint8_t type = 1; (type & (uint8_t)ESystemMessageType::All) != 0; type <<= 1) {
    if ((type & (uint8_t)_lastSystemReportSuccess) == 0 && generateSystemReport(g_bufferSystemProvider, uptime, (ESystemMessageType)type))
      _lastSystemReportSuccess = (ESystemMessageType)(type | (uint8_t)_lastSystemReportSuccess);
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
