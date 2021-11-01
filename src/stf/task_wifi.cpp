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
#  include <stf/util.h>

#  if STFWIFI_IOTWEBCONF == 1
#    include <stf/config_iotwebconf.h>
#    include <Preferences.h>
#    include <rom/rtc.h>
#  endif
#  if STFMQTT == 1
#    include <stf/link_mqtt.h>
#  endif

#  include <WiFi.h>

namespace stf {

template <>
const TaskDescriptor Task<NetTask>::_descriptor = {&_obj, "NetTask", 0, 0, 2};
template class Task<NetTask>;

const char* NetTask::_wifiSSID = STFWIFI_SSID;
const char* NetTask::_wifiPassword = STFWIFI_PASSWORD;

#  if STFMQTT == 1
const char* NetTask::_mqttServer = STFMQTT_SERVER;
const char* NetTask::_mqttPort = STFMQTT_PORT;
const char* NetTask::_mqttUser = STFMQTT_USER;
const char* NetTask::_mqttPassword = STFMQTT_PASSWORD;
#  endif

#  if STFWIFI_IOTWEBCONF == 1

bool NetTask::checkForConfigAtSetup() {
  Preferences setupStore;
  setupStore.begin(_setupStoreName);
  int8_t resetsOrig = _resetCounterForSetup = setupStore.getInt(_setupResetsPreference, 0);

  RESET_REASON r0 = rtc_get_reset_reason(0);
  STFLOG_INFO("Reset reasons: %d - reset counter %d\n", r0, _resetCounterForSetup);

  bool res = false;
  if (r0 != POWERON_RESET || (res = (++_resetCounterForSetup >= 3))) _resetCounterForSetup = 0;
  if (resetsOrig != _resetCounterForSetup) setupStore.putInt(_setupResetsPreference, _resetCounterForSetup);
  return res;
}

void NetTask::checkForConfigAtLoop() {
  if (_resetCounterForSetup != 0 && Host::uptimeSec32() >= 2) {
    _resetCounterForSetup = 0;
    Preferences setupStore;
    setupStore.begin(_setupStoreName);
    setupStore.putInt(_setupResetsPreference, 0);
  }
}

#  endif

bool NetTask::checkConnection() {
  uint64_t uptime = Host::uptimeMS64();
  if (WiFi.isConnected()) {
    STFLED_COMMAND(STFLEDEVENT_WIFI_CONNECTED);
    if (_wifiConnectionTry > 0) {
      _wifiConnectionTry = 0;
      _wifiConnectionTime = uptime;
      memcpy(Host::_ip4, &WiFi.localIP()[0], 4);
      Log::connected("wifi");
    }
    return true;
  }
  STFLED_COMMAND(STFLEDEVENT_WIFI_NOT_CONNECTED);
  if (uptime - _wifiConnectionTime > 5000 || _wifiConnectionTry == 0) {
    const char* wifi_hostName = WiFi.getHostname();
    if (wifi_hostName == NULL || strcmp(Host::_name, wifi_hostName) != 0) WiFi.setHostname(Host::_name);
    _wifiConnectionTry++;
    _wifiConnectionTime = uptime;
    WiFi.mode(WIFI_STA);
    WiFi.begin(_wifiSSID, _wifiPassword);
    Log::connecting("wifi", _wifiConnectionTry);
  }
  return false;
}

void NetTask::setup() {
#  if STFWIFI_IOTWEBCONF == 1
  bool ap = checkForConfigAtSetup();
  IotWebConfWrapper::setup(ap);
#  endif
#  if STFMQTT == 1
  MQTTConsumer::_obj.setup();
#  endif
}

uint32_t NetTask::loop() {
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
    uint32_t toWaitMQTT = MQTTConsumer::_obj.loop();
    if (toWaitMQTT != 0 && toWaitMQTT < toWait) toWait = toWaitMQTT;
  }
#  endif

  return toWait;
}
} // namespace stf

#endif
