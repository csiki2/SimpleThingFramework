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

#include <stf/task_ota.h>

#ifdef STFTASK_OTA

#  include <stf/provider_system.h>
#  include <stf/data_discovery.h>

namespace stf {

DEFINE_PROVIDERTASK(OTAProvider, 3, 0, 0)

OTAProvider::OTAProvider() : Provider(g_bufferSystemProvider) {
}

uint32_t OTAProvider::loop() {
  uint32_t toWait = 100;
  if (WiFi.isConnected() && _enabled) {
    if (_arduinoOTA == nullptr) {
      _arduinoOTA = new ArduinoOTAClass;
      _arduinoOTA->setHostname(Host::_name).setPort(STFOTA_PORT).setPassword(Host::_password);
      _arduinoOTA->onStart(onStart).onEnd(onEnd).onProgress(onProgress).onError(onError);
      _arduinoOTA->begin();
    }
    _arduinoOTA->handle();
  } else {
    if (_arduinoOTA != nullptr) {
      _arduinoOTA->end();
      delete _arduinoOTA;
      _arduinoOTA = nullptr;
    }
  }
  return toWait;
}

const DiscoveryBlock OTAProvider::_switch = {edf_ota, edcSwitch, eecConfig, "OTA", nullptr, nullptr};
const DiscoveryBlock* OTAProvider::_listSystem[] = {&_switch, nullptr};

uint OTAProvider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  uint res = 0;
  switch (type) {
    case ESystemMessageType::Discovery:
      res = Discovery::addBlocks(systemBuffer, etitSYSR, _listSystem);
      break;
    case ESystemMessageType::Retained:
      res = 1;
      if (systemBuffer != nullptr && systemBuffer->getFreeBlocks() >= res) {
        systemBuffer->nextToWrite(edf_ota, edt_String, etisSource0Ptr).setPtr(_enabled ? "ON" : "OFF");
        res = 0;
      }
      break;
    default:
      break;
  }
  return res;
}

void OTAProvider::feedback(const FeedbackInfo& info) {
  handleSimpleFeedback(info, "OTA", edf_ota, _enabled);
}

void OTAProvider::onStart() { STFLOG_WARNING("OTA update is starting.\n"); }
void OTAProvider::onEnd() { STFLOG_WARNING("OTA update finished.\n"); }
void OTAProvider::onProgress(unsigned progress_, unsigned total_) { STFLOG_WARNING("OTA progress %8u/%8u (%6.2f%%)\n", progress_, total_, 100. * progress_ / total_); }
void OTAProvider::onError(ota_error_t error_) {
  const char* errorType = nullptr;
  switch (error_) {
    case OTA_AUTH_ERROR:
      errorType = "Auth";
      break;
    case OTA_BEGIN_ERROR:
      errorType = "Begin";
      break;
    case OTA_CONNECT_ERROR:
      errorType = "Connect";
      break;
    case OTA_RECEIVE_ERROR:
      errorType = "Receive";
      break;
    case OTA_END_ERROR:
      errorType = "End";
      break;
    default:
      break;
  }
  STFLOG_WARNING("OTA error [%u]: %s %s\n", error_, errorType != nullptr ? errorType : "Unknown", errorType != nullptr ? "failed" : "error");
}

} // namespace stf

#endif
