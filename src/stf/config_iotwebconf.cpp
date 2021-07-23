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

#include <stf/config_iotwebconf.h>

#if STFWIFI_IOTWEBCONF == 1

namespace stf {

IotWebConfWrapper::IotWebConfWrapper(APObject* ap)
    : apObject(ap),
      webConf(hostName, ap != nullptr ? &ap->dnsServer : (DNSServer*)nullptr, ap != nullptr ? &ap->webServer : (WebServer*)nullptr, STF_THING_PASSWORD, STFWIFI_IOTWEBCONF_CONFVERSION)
#  if STFMQTT == 1
      ,
      paramMQTTGroup("mqtt", "MQTT configuration"),
      paramMQTTServer("Server", "mqtt_serv", mqttServer, sizeof(mqttServer), STFMQTT_SERVER),
      paramMQTTPort("Port", "mqtt_port", mqttPort, sizeof(mqttPort), STFMQTT_PORT),
      paramMQTTUser("User", "mqtt_user", mqttUser, sizeof(mqttUser), STFMQTT_USER),
      paramMQTTPassword("Password", "mqtt_pass", mqttPassword, sizeof(mqttPassword), STFMQTT_PASSWORD)
#  endif
{
  if (apObject != nullptr) apObject->setupTimer = 0;

  webConf.getWifiSsidParameter()->defaultValue = STFWIFI_SSID;
  webConf.getWifiPasswordParameter()->defaultValue = STFWIFI_PASSWORD;
}

IotWebConfWrapper::~IotWebConfWrapper() {
  if (apObject != nullptr) delete apObject;
  gObj = nullptr;
}

IotWebConfWrapper* IotWebConfWrapper::gObj = nullptr;

void IotWebConfWrapper::setup(bool forceAP) {
  if (gObj != nullptr) return;
  gObj = new IotWebConfWrapper(forceAP ? new APObject : nullptr);
  if (gObj->init()) return;
  delete gObj;
  gObj = new IotWebConfWrapper(new APObject);
  gObj->init();
}

uint32_t IotWebConfWrapper::loop() {
  if (gObj == nullptr || gObj->apObject == nullptr) return 0;
  if (gObj->apObject->setupTimer + (STFCONF_SETUP_TIMEOUT * 1000) < uptimeMS64()) {
    STFLOG_WARNING("Portal timeout reached, reset the device.");
    ESP.restart();
  }
  gObj->webConf.doLoop();
  STFLED_COMMAND(STFLEDEVENT_SETUP);
  return 10;
}

bool IotWebConfWrapper::init() {
  webConf.disableBlink();
  webConf.skipApStartup();
  addParameters();

  const char* mode;
  if (apObject == nullptr) {
    if (!webConf.loadConfig() && wifiSSID[0] == 0) return false;
    iotwebconf::WifiAuthInfo wifiAuthInfo = webConf.getWifiAuthInfo();
    hostName = webConf.getThingName();
    wifiSSID = wifiAuthInfo.ssid;
    wifiPassword = wifiAuthInfo.password;
    mode = "load config";
  } else {
    apObject->webServer.on("/", [] { gObj->handleAPRoot(); });
    apObject->webServer.on("/config", [] { gObj->webConf.handleConfig(); });
    apObject->webServer.onNotFound([] { gObj->webConf.handleNotFound(); });
    webConf.init();
    webConf.forceApMode(true);
    mode = "access point";
  }
#  if STFMQTT == 1
  stf::mqttServer = IotWebConfWrapper::mqttServer;
  stf::mqttPort = IotWebConfWrapper::mqttPort;
  stf::mqttUser = IotWebConfWrapper::mqttUser;
  stf::mqttPassword = IotWebConfWrapper::mqttPassword;
#  endif
  STFLOG_INFO("IotWebConf init %s mode, ", mode);
  logMemoryUsage(STFLOG_LEVEL_INFO);
  return true;
}

void IotWebConfWrapper::addParameters() {
#  if STFMQTT == 1
  paramMQTTGroup.addItem(&paramMQTTServer);
  paramMQTTGroup.addItem(&paramMQTTPort);
  paramMQTTGroup.addItem(&paramMQTTUser);
  paramMQTTGroup.addItem(&paramMQTTPassword);
  webConf.addParameterGroup(&paramMQTTGroup);
#  endif
}

class HtmlFormatProvider : public iotwebconf::HtmlFormatProvider {
public:
  virtual String getStyleInner() {
    String style = iotwebconf::HtmlFormatProvider::getStyleInner();
    style += " .lb{white-space:nowrap;padding-right:10px;margin-top:auto;margin-bottom:auto} input{text-align:right;max-width:75%;margin-left:auto}";
    return style;
  }
  virtual String getFormEnd() { return FPSTR("Go to <a href='config'>configure page</a> to change values.</form>\n"); }
};

class WebRequestWrapper : public iotwebconf::StandardWebRequestWrapper {
public:
  WebRequestWrapper(WebServer* server) : StandardWebRequestWrapper(server){};
  void sendContent(const String& content) override {
    // this is practically a hack due to the divided responsibility and the lack of proper interfaces, but the result is nice
    if (content.indexOf("<input") != -1) {
      if (content.indexOf("<input type='password'") != -1) return;
      String contentChanged = content;
      contentChanged.replace("<label", "<div style='display: flex;padding:0px' ><label class='lb'");
      contentChanged.replace("<input ", "<input disabled ");
      contentChanged.replace("/>", "/></div>");
      StandardWebRequestWrapper::sendContent(contentChanged);
      return;
    }
    StandardWebRequestWrapper::sendContent(content);
  };
};

void IotWebConfWrapper::handleAPRoot() {
  IotWebConf& webConf = gObj->webConf;
  if (gObj->apObject->setupTimer == 0) gObj->apObject->setupTimer = uptimeMS64();
  if (webConf.handleCaptivePortal()) return;

  // this is practically a hack due to the divided responsibility and the lack of proper interfaces, but the result is nice
  iotwebconf::HtmlFormatProvider* orig = webConf.getHtmlFormatProvider();
  WebRequestWrapper localWebRequestWrapper(&gObj->apObject->webServer);
  HtmlFormatProvider localHtmlFormatProvider;
  webConf.setHtmlFormatProvider(&localHtmlFormatProvider);
  webConf.handleConfig(&localWebRequestWrapper);
  webConf.setHtmlFormatProvider(orig);
}

} // namespace stf

#endif
