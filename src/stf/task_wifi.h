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
extern const char* wifiSSID;
extern const char* wifiPassword;
#endif
#if STFMQTT == 1
extern const char* mqttServer;
extern const char* mqttPort;
extern const char* mqttUser;
extern const char* mqttPassword;
#endif

} // namespace stf
