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

#  include <stf/task.h>
#  include <ArduinoOTA.h>

namespace stf {

DEFINE_STFTASK(OTA, 3, 0, 0, nullptr)

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

void setupOTATask(void*) {
  ArduinoOTA.setHostname(hostName).setPort(STFOTA_PORT).setPassword(hostPassword);
  ArduinoOTA.onStart(otaOnStart).onEnd(otaOnEnd).onProgress(otaOnProgress).onError(otaOnError);
}

uint32_t loopOTATask(void*) {
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

} // namespace stf

#endif
