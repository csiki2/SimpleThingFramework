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

#  include <stf/util.h>

namespace stf {

IotWebConfWrapper::IotWebConfWrapper(APObject* ap)
    : _apObject(ap),
      _webConf(Host::_name, ap != nullptr ? &ap->dnsServer : (DNSServer*)nullptr, ap != nullptr ? &ap->webServer : (WebServer*)nullptr, STF_THING_PASSWORD, STFWIFI_IOTWEBCONF_CONFVERSION)
#  if STFMQTT == 1
      ,
      _paramMQTTGroup("mqtt", "MQTT configuration"),
      _paramMQTTServer("Server", "mqtt_serv", _mqttServer, sizeof(_mqttServer), STFMQTT_SERVER),
      _paramMQTTPort("Port", "mqtt_port", _mqttPort, sizeof(_mqttPort), STFMQTT_PORT),
      _paramMQTTUser("User", "mqtt_user", _mqttUser, sizeof(_mqttUser), STFMQTT_USER),
      _paramMQTTPassword("Password", "mqtt_pass", _mqttPassword, sizeof(_mqttPassword), STFMQTT_PASSWORD)
#  endif
{
  if (_apObject != nullptr) _apObject->setupTimer = 0;

  _webConf.getWifiSsidParameter()->defaultValue = STFWIFI_SSID;
  _webConf.getWifiPasswordParameter()->defaultValue = STFWIFI_PASSWORD;
}

IotWebConfWrapper::~IotWebConfWrapper() {
  if (_apObject != nullptr) delete _apObject;
  _obj = nullptr;
}

IotWebConfWrapper* IotWebConfWrapper::_obj = nullptr;

void IotWebConfWrapper::setup(bool forceAP) {
  if (_obj != nullptr) return;
  _obj = new IotWebConfWrapper(forceAP ? new APObject : nullptr);
  if (_obj->init()) return;
  delete _obj;
  _obj = new IotWebConfWrapper(new APObject);
  _obj->init();
}

uint32_t IotWebConfWrapper::loop() {
  if (_obj == nullptr || _obj->_apObject == nullptr) return 0;
  if (_obj->_apObject->setupTimer + STFCONF_SETUP_TIMEOUT < Host::uptimeSec32()) {
    STFLOG_WARNING("Portal timeout reached, reset the device.");
    ESP.restart();
  }
  _obj->_webConf.doLoop();
  STFLED_COMMAND(STFLEDEVENT_SETUP);
  return 10;
}

bool IotWebConfWrapper::init() {
  _webConf.disableBlink();
  _webConf.skipApStartup();
  addParameters();

  const char* mode;
  if (_apObject == nullptr) {
    if (!_webConf.loadConfig() && wifiSSID[0] == 0) return false;
    iotwebconf::WifiAuthInfo wifiAuthInfo = _webConf.getWifiAuthInfo();
    Host::_name = _webConf.getThingName();
    Host::_password = _webConf.getApPasswordParameter()->valueBuffer;
    wifiSSID = wifiAuthInfo.ssid;
    wifiPassword = wifiAuthInfo.password;
    mode = "load config";
  } else {
    _apObject->webServer.on("/", [] { _obj->handleAPRoot(); });
    _apObject->webServer.on("/config", [] { _obj->_webConf.handleConfig(); });
    _apObject->webServer.onNotFound([] { _obj->_webConf.handleNotFound(); });
    _webConf.init();
    _webConf.forceApMode(true);
    mode = "access point";
  }
#  if STFMQTT == 1
  stf::mqttServer = IotWebConfWrapper::_mqttServer;
  stf::mqttPort = IotWebConfWrapper::_mqttPort;
  stf::mqttUser = IotWebConfWrapper::_mqttUser;
  stf::mqttPassword = IotWebConfWrapper::_mqttPassword;
#  endif
  STFLOG_INFO("IotWebConf init %s mode, ", mode);
  Log::memoryUsage(STFLOG_LEVEL_INFO);
  return true;
}

void IotWebConfWrapper::addParameters() {
#  if STFMQTT == 1
  _paramMQTTGroup.addItem(&_paramMQTTServer);
  _paramMQTTGroup.addItem(&_paramMQTTPort);
  _paramMQTTGroup.addItem(&_paramMQTTUser);
  _paramMQTTGroup.addItem(&_paramMQTTPassword);
  _webConf.addParameterGroup(&_paramMQTTGroup);
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
  IotWebConf& webConf = _obj->_webConf;
  if (_obj->_apObject->setupTimer == 0) _obj->_apObject->setupTimer = Host::uptimeSec32();
  if (webConf.handleCaptivePortal()) return;

  // this is practically a hack due to the divided responsibility and the lack of proper interfaces, but the result is nice
  iotwebconf::HtmlFormatProvider* orig = webConf.getHtmlFormatProvider();
  WebRequestWrapper localWebRequestWrapper(&_obj->_apObject->webServer);
  HtmlFormatProvider localHtmlFormatProvider;
  webConf.setHtmlFormatProvider(&localHtmlFormatProvider);
  webConf.handleConfig(&localWebRequestWrapper);
  webConf.setHtmlFormatProvider(orig);
}

} // namespace stf

#endif
