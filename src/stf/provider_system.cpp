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

SystemProvider::SystemProvider() : Provider(g_bufferSystemProvider), lastSystemReport(0), forceSystemReport(false) {
}

const DiscoveryBlock* SystemProvider::_listSystem[] = {&Discovery::_Uptime_S, &Discovery::_Uptime_D, &Discovery::_Free_Memory, nullptr};
uint SystemProvider::systemDiscovery(DataBuffer* systemBuffer_) {
  uint res = Discovery::addDiscoveryBlocks(systemBuffer_, etitSYS, _listSystem, eeiNone, nullptr, Host::_name, "Test", "community", "0.01");
  return res;
}

uint SystemProvider::systemUpdate(DataBuffer* systemBuffer_, uint32_t uptimeS_) {
  if (systemBuffer_ == nullptr || systemBuffer_->getFreeBlocks() < 4) return 4;
  systemBuffer_->nextToWrite(edf__topic, edt_Topic, etitSYS, eeiCacheDeviceHost).setPtr(&Host::_info);
  systemBuffer_->nextToWrite(edf_uptime_s, edt_32, 0).set64(uptimeS_);
  systemBuffer_->nextToWrite(edf_uptime_d, edt_32, 0).set32(uptimeS_ / (24 * 60 * 60));
  systemBuffer_->nextToWrite(edf_ip, edt_Raw, 4 + etirSeparatorDot + etirFormatNumber).setRaw(Host::_ip4, 4);
  systemBuffer_->nextToWrite(edf_free_memory, edt_32, 0).set32(ESP.getFreeHeap());
  return 0;
}

void SystemProvider::requestReport() {
  SystemProviderObj.forceSystemReport = true;
}

uint SystemProvider::loop() {
  const uint waitTime = 50, updateTimeS = 120;

  Consumer* cons = g_bufferSystemProvider->getConsumer();
  if (cons == nullptr || !cons->isReady()) return waitTime;
  uint32_t uptime = Host::uptimeSec32();
  if (!forceSystemReport && lastSystemReport >= cons->readyTime() && lastSystemReport + updateTimeS > uptime) return waitTime; // no report is needed yet
  forceSystemReport = false;

  // Generate discovery only once per connection
  int sum = systemUpdate(nullptr, uptime);
  for (Provider* p = _providerHead; p != nullptr; p = (Provider*)p->_objectNext)
    sum += p->systemUpdate(nullptr, uptime);

  if (lastSystemReport < cons->readyTime()) {
    sum += systemDiscovery(nullptr);
    for (Provider* p = _providerHead; p != nullptr; p = (Provider*)p->_objectNext)
      sum += p->systemDiscovery(nullptr);
    if (g_bufferSystemProvider->getFreeBlocks() < sum) return waitTime;
    systemDiscovery(g_bufferSystemProvider);
    for (Provider* p = _providerHead; p != nullptr; p = (Provider*)p->_objectNext)
      p->systemDiscovery(g_bufferSystemProvider);
  } else {
    if (g_bufferSystemProvider->getFreeBlocks() < sum) return waitTime;
  }

  lastSystemReport = uptime;

  systemUpdate(g_bufferSystemProvider, uptime);
  for (Provider* p = _providerHead; p != nullptr; p = (Provider*)p->_objectNext)
    p->systemUpdate(g_bufferSystemProvider, uptime);
  g_bufferSystemProvider->closeMessage();

  return waitTime;
}

} // namespace stf
