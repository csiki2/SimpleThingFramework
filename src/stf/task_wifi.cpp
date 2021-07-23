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

#include <stf/os.h>

#if STFTASK_WIFI == 1

#  include <stf/task.h>

#  if STFWIFI_IOTWEBCONF == 1
#    include <stf/config_iotwebconf.h>
#  endif
#  if STFMQTT == 1
#    include <stf/link_mqtt.h>
#  endif

#  include <WiFi.h>

namespace stf {

DEFINE_STFTASK(Wifi, 2, 0, 0, nullptr)

uint64_t wifiConnectionTime = 0;
uint wifiConnectionTry = 0;

const char* wifiSSID = STFWIFI_SSID;
const char* wifiPassword = STFWIFI_PASSWORD;

#  if STFMQTT == 1
const char* mqttServer = STFMQTT_SERVER;
const char* mqttPort = STFMQTT_PORT;
const char* mqttUser = STFMQTT_USER;
const char* mqttPassword = STFMQTT_PASSWORD;
#  endif
} // namespace stf

#  if STFWIFI_IOTWEBCONF == 1

#    include <Preferences.h>
#    include <rom/rtc.h>

namespace stf {

int8_t resetCounterForSetup = 0;

const char setupStoreName[] = "setup";
const char setupResetsPreference[] = "resets";

bool checkForConfigAtSetup() {
  Preferences setupStore;
  setupStore.begin(setupStoreName);
  int8_t resetsOrig = resetCounterForSetup = setupStore.getInt(setupResetsPreference, 0);

  RESET_REASON r0 = rtc_get_reset_reason(0);
  STFLOG_INFO("Reset reasons: %d - reset counter %d\n", r0, resetCounterForSetup);

  bool res = false;
  if (r0 != POWERON_RESET || (res = (++resetCounterForSetup >= 3))) resetCounterForSetup = 0;
  if (resetsOrig != resetCounterForSetup) setupStore.putInt(setupResetsPreference, resetCounterForSetup);
  return res;
}

void checkForConfigAtLoop() {
  if (resetCounterForSetup != 0 && uptimeMS64() > 2000) {
    resetCounterForSetup = 0;
    Preferences setupStore;
    setupStore.begin(setupStoreName);
    setupStore.putInt(setupResetsPreference, 0);
  }
}

} // namespace stf
#  endif

namespace stf {

bool checkConnection() {
  uint64_t uptime = uptimeMS64();
  if (WiFi.isConnected()) {
    STFLED_COMMAND(STFLEDEVENT_WIFI_CONNECTED);
    if (wifiConnectionTry > 0) {
      wifiConnectionTry = 0;
      wifiConnectionTime = uptime;
      logConnected("wifi");
    }
    return true;
  }
  STFLED_COMMAND(STFLEDEVENT_WIFI_NOT_CONNECTED);
  if (uptime - wifiConnectionTime > 5000 || wifiConnectionTry == 0) {
    const char* wifi_hostName = WiFi.getHostname();
    if (wifi_hostName == NULL || strcmp(hostName, wifi_hostName) != 0) WiFi.setHostname(hostName);
    wifiConnectionTry++;
    wifiConnectionTime = uptime;
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPassword);
    logConnecting("wifi", wifiConnectionTry);
  }
  return false;
}

void setupWifiTask(void*) {
#  if STFWIFI_IOTWEBCONF == 1
  bool ap = checkForConfigAtSetup();
  IotWebConfWrapper::setup(ap);
#  endif
#  if STFMQTT == 1
  PubSubClientWrapper::setup();
#  endif
}

uint32_t loopWifiTask(void*) {
  uint32_t toWait = 100;
#  if STFWIFI_IOTWEBCONF == 1
  uint32_t toWaitConf = IotWebConfWrapper::loop();
  if (toWaitConf != 0) return toWaitConf;
  checkForConfigAtLoop();
#  endif

  bool connected = checkConnection();
#  ifdef CONFIG_ARDUINO_LOOP_STACK_SIZE
#  endif
#  if STFMQTT == 1
  if (connected) {
    uint32_t toWaitMQTT = PubSubClientWrapper::loop();
    if (toWaitMQTT != 0 && toWaitMQTT < toWait) toWait = toWaitMQTT;
  }
#  endif

  return toWait;
}
} // namespace stf

#endif
