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
#  include <ArduinoOTA.h>

namespace stf {

DEFINE_PROVIDERTASK(OTAProvider, 3, 0, 0)

void otaOnStart() { STFLOG_WARNING("OTA update is starting.\n"); }
void otaOnEnd() { STFLOG_WARNING("OTA update finished.\n"); }
void otaOnProgress(unsigned progress_, unsigned total_) { STFLOG_WARNING("OTA progress %8u/%8u (%6.2f%%)\n", progress_, total_, 100. * progress_ / total_); }
void otaOnError(ota_error_t error_) {
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

OTAProvider::OTAProvider() : Provider(g_bufferSystemProvider) {
  enabled = true;
}

void OTAProvider::setup() {
  ArduinoOTA.setHostname(Host::_name).setPort(STFOTA_PORT).setPassword(Host::_password);
  ArduinoOTA.onStart(otaOnStart).onEnd(otaOnEnd).onProgress(otaOnProgress).onError(otaOnError);
}

uint32_t OTAProvider::loop() {
  uint32_t toWait = 100;
  if (WiFi.isConnected()) {
    static bool otaInit = false;
    if (!otaInit) {
      ArduinoOTA.begin();
      otaInit = true;
    } else {
      ArduinoOTA.handle();
    }
  }
  return toWait;
}

const DiscoveryBlock OTASwitch = {edf_ota, edcSwitch, "OTA", nullptr, nullptr};
const DiscoveryBlock* _OTAlistSystem[] = {&OTASwitch, nullptr};
uint OTAProvider::systemDiscovery(DataBuffer* systemBuffer_) {
  uint res = Discovery::addDiscoveryBlocks(systemBuffer_, etitSYS, _OTAlistSystem);
  return res;
}

uint OTAProvider::systemUpdate(DataBuffer* systemBuffer_, uint32_t uptimeS_) {
  if (systemBuffer_ == nullptr) return 1;
  systemBuffer_->nextToWrite(edf_ota, edt_String, etisSource0Ptr).setPtr(enabled ? "ON" : "OFF");
  return 0;
}

void OTAProvider::feedback(const FeedbackInfo& info_) {
  if (info_.fieldEnum == edf_ota) {
    bool set = info_.payloadLength == 2 && strncmp((const char*)info_.payload, "ON", 2) == 0;
    STFLOG_INFO("OTA command detected %u - %*.*s\n", enabled, info_.payloadLength, info_.payloadLength, info_.payload);
    if (set != enabled) {
      enabled = set;
      SystemProvider::requestReport();
    }
  }
}

} // namespace stf

#endif
