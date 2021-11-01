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

#ifndef STFWIFI_SSID
#  define STFWIFI_SSID ""
#endif

#ifndef STFWIFI_PASSWORD
#  define STFWIFI_PASSWORD ""
#endif

#ifndef STFWIFI_IOTWEBCONF
#  define STFWIFI_IOTWEBCONF 1
#endif

#ifndef STFMQTT
#  define STFMQTT 1
#endif

#if STFMQTT == 1
#  ifndef STFMQTT_SERVER
#    define STFMQTT_SERVER "mqtt"
#  endif
#  ifndef STFMQTT_PORT
#    define STFMQTT_PORT "1883"
#  endif
#  ifndef STFMQTT_USER
#    define STFMQTT_USER "mqtt"
#  endif
#  ifndef STFMQTT_PASSWORD
#    define STFMQTT_PASSWORD ""
#  endif
#  ifndef STFMQTT_JSONBUFFER_SIZE
#    define STFMQTT_JSONBUFFER_SIZE 1024
#  endif
#endif

#if STFWIFI_IOTWEBCONF == 1
#  ifndef STFWIFI_IOTWEBCONF_CONFVERSION
#    define STFWIFI_IOTWEBCONF_CONFVERSION "ver0"
#  endif
#  ifndef STFCONF_PARAM_SIZE
#    define STFCONF_PARAM_SIZE 33
#  endif
#  ifndef STFCONF_SETUP_TIMEOUT
#    define STFCONF_SETUP_TIMEOUT 300
#  endif
#endif

namespace stf {
#if STFTASK_WIFI == 1

class NetTask : public Task<NetTask> {
public:
  virtual void setup() override;
  virtual uint loop() override;

  bool checkConnection();

  uint64_t _wifiConnectionTime = 0;
  uint _wifiConnectionTry = 0;

  static const char* _wifiSSID;
  static const char* _wifiPassword;

#  if STFMQTT == 1
  static const char* _mqttServer;
  static const char* _mqttPort;
  static const char* _mqttUser;
  static const char* _mqttPassword;
#  endif
#  if STFWIFI_IOTWEBCONF == 1
  int8_t _resetCounterForSetup = 0;

  const char* _setupStoreName = "setup";
  const char* _setupResetsPreference = "resets";

  bool checkForConfigAtSetup();
  void checkForConfigAtLoop();
#  endif
};

#endif

} // namespace stf
