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

#include <stf/os.h>

#if STFWIFI_IOTWEBCONF == 1

#  include <IotWebConf.h>
#  include <IotWebConfParameter.h>

namespace stf {
class IotWebConfWrapper {
public:
  static void setup(bool forceAP);
  static uint32_t loop();

protected:
  static IotWebConfWrapper* _obj;

  struct APObject {
    WebServer webServer;
    DNSServer dnsServer;
    uint32_t setupTimer;
  } * _apObject;

  IotWebConf _webConf;

#  if STFMQTT == 1
  char _mqttServer[STFCONF_PARAM_SIZE];
  char _mqttPort[6];
  char _mqttUser[STFCONF_PARAM_SIZE];
  char _mqttPassword[2 * STFCONF_PARAM_SIZE];

  iotwebconf::ParameterGroup _paramMQTTGroup;
  iotwebconf::TextParameter _paramMQTTServer;
  iotwebconf::NumberParameter _paramMQTTPort;
  iotwebconf::TextParameter _paramMQTTUser;
  iotwebconf::PasswordParameter _paramMQTTPassword;
#  endif

  IotWebConfWrapper(APObject* ap);
  virtual ~IotWebConfWrapper();
  bool init();
  void addParameters();
  void handleAPRoot();
};
} // namespace stf

#endif
