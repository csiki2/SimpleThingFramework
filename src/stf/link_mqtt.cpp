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

#include <stf/link_mqtt.h>

#if STFMQTT == 1

#  include <stf/data_block.h>
#  include <stf/provider.h>
#  include <stf/util.h>
#  include <stdlib.h>

namespace stf {

MQTTConsumer MQTTConsumer::_obj;

MQTTConsumer::MQTTConsumer() : _client(_wifiClient) {
  _connectionTime = 0;
  _connectionTry = 0;
}

void MQTTConsumer::callback(char* topic, byte* payload, unsigned int length) {
  FeedbackInfo info;
  info.set(topic, payload, length);
  STFLOG_INFO("Received MQTT message (%s) [%*.*s|%*.*s|%s][%d|%d] - %*.*s\n", topic,
              info.topicStrLen, info.topicStrLen, info.topicStr,
              info.idStrLen, info.idStrLen, info.idStr,
              info.fieldStr,
              info.topicEnum, info.fieldEnum,
              length, length, payload);
  _obj.broadcastFeedback(info);
}

void MQTTConsumer::setup() {
  int port = strtol(NetTask::_mqttPort, nullptr, 10);
  STFLOG_INFO("MQTT Server - %s:%d\n", NetTask::_mqttServer, port);
  _client.setServer(NetTask::_mqttServer, (uint16_t)port);
  _client.setBufferSize(STFMQTT_JSONBUFFER_SIZE);
  _client.setCallback(callback);

#  undef STF_BUFFER_DECLARE
#  define STF_BUFFER_DECLARE(name, size, task) _obj.addBuffer(&g_##name);
#  undef STF_BUFFER_PROVIDER
#  define STF_BUFFER_PROVIDER(name, provider)
  STFBUFFERS;
}

uint32_t MQTTConsumer::loop() {
  if (_client.connected()) {
    STFLED_COMMAND(STFLEDEVENT_MQTT_CONNECTED);
    consumeBuffers(_jsonBuffer);
    _client.loop();
    return 10;
  }
  STFLED_COMMAND(STFLEDEVENT_MQTT_NOT_CONNECTED);
  if (WiFi.isConnected()) {
    uint64_t uptime = Host::uptimeMS64();
    if (uptime - _connectionTime > 5000 || _connectionTry == 0) {
      _connectionTry++;
      _connectionTime = uptime;
      Log::connecting("MQTT server", _connectionTry);
      if (_client.connect(Host::_name, NetTask::_mqttUser, NetTask::_mqttPassword)) {
        Log::connected("MQTT server");
        _connectionTry = 0;
        _connectionTime = Host::uptimeSec32();
        char subscribeStr[32 + strlen(Host::_name) + strlen(Host::_info.strId)];
        sprintf(subscribeStr, "home/%s/+/%s/command/#", Host::_name, Host::_info.strId);
        STFLOG_INFO("MQTT subscribe to %s\n", subscribeStr);
        _client.subscribe(subscribeStr);
        return 10;
      } else {
        STFLOG_INFO("Unable to connect to the MQTT Server.\n");
      }
    }
  }
  return 0;
}

bool MQTTConsumer::isReady() { return _client.connected(); }
uint32_t MQTTConsumer::readyTime() { return _connectionTime; }

bool MQTTConsumer::send(JsonBuffer& jsonBuffer_) {
  //return false;
  return _client.publish(jsonBuffer_._buffer + jsonBuffer_._jsonSize, jsonBuffer_._buffer, jsonBuffer_._pos);
}

} // namespace stf

#endif
