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
}

void MQTTConsumer::callback(char* topic, byte* payload, unsigned int length) {
  if (_obj._messageArrived == 0 && strstr(topic, "/SYSRtoMQTT/") != nullptr) _obj._messageArrived = 1;

  FeedbackInfo info;
  info.set(topic, payload, length);

  for (bool send = !info.retained;;) {
    STFLOG_INFO("%10llu Received MQTT message (%s) [%*.*s|%*.*s|%*.*s][%d|%d] - %*.*s\n",
                Host::uptimeMS64(),
                topic,
                info.topicStrLen, info.topicStrLen, info.topicStr,
                info.idStrLen, info.idStrLen, info.idStr,
                info.fieldStrLen, info.fieldStrLen, info.fieldStr,
                info.topicEnum, info.fieldEnum,
                length, length, payload);
    if (send) _obj.broadcastFeedback(info);
    if (!(send = info.next())) break;
    length = info.payloadLength;
    payload = (byte*)info.payload;
  }
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
    if (_messageArrived < 2 && (_messageArrived == 1 || _readyTime.elapsedTime() > 5000)) {
      _messageArrived = 2;
      localSubscribe("home/%s/SYSRtoMQTT/%s", false);
    }
    // We might have setting retained, wait for that so they won't be overwritten
    if (_messageArrived == 2) consumeBuffers(_jsonBuffer);
    _client.loop();
    return 10;
  }
  STFLED_COMMAND(STFLEDEVENT_MQTT_NOT_CONNECTED);
  if (WiFi.isConnected()) {
    if (_connectionTry == 0 || _readyTime.elapsedTime() > 5000) {
      _connectionTry++;
      _readyTime.reset();
      Log::connecting("MQTT server", _connectionTry);
      const char* willTopicFormat = "home/%s/%stoMQTT/%s";
      char willTopic[strlen(willTopicFormat) + strlen(Host::_name) + strlen(DataType::_topicNames[etitCONN]) + strlen(Host::_info.strId)];
      sprintf(willTopic, willTopicFormat, Host::_name, DataType::_topicNames[etitCONN], Host::_info.strId);
      if (_client.connect(Host::_name, NetTask::_mqttUser, NetTask::_mqttPassword, willTopic, 0, true, R"({"connectivity":"OFF"})")) {
        _client.publish(willTopic, R"({"connectivity":"ON"})", true);
        Log::connected("MQTT server");
        _connectionTry = 0;
        _readyTime.reset();
        _messageArrived = 0;
        localSubscribe("home/%s/+/%s/command/#");
        localSubscribe("home/%s/SYSRtoMQTT/%s");
        return 10;
      } else {
        STFLOG_INFO("Unable to connect to the MQTT Server.\n");
      }
    }
  }
  return 0;
}

// Don't allow to the Providers (SystemProvider!!!) to generate any message till the status message is not arrived (or already waited 5sec)
bool MQTTConsumer::isReady() { return _client.connected() && _messageArrived == 2; }

bool MQTTConsumer::send(JsonBuffer& jsonBuffer, bool retain) {
  //return false;
  return _client.publish(jsonBuffer._buffer + jsonBuffer._jsonSize, (const uint8_t*)jsonBuffer._buffer, jsonBuffer._pos, retain);
}

void MQTTConsumer::localSubscribe(const char* topicFormat, bool subscribe) {
  char subscribeStr[strlen(topicFormat) + strlen(Host::_name) + strlen(Host::_info.strId)];
  sprintf(subscribeStr, topicFormat, Host::_name, Host::_info.strId);
  STFLOG_INFO("%10llu MQTT %s %s\n", Host::uptimeMS64(), subscribe ? "subscribe to" : "unsubscribe from", subscribeStr);
  if (subscribe)
    _client.subscribe(subscribeStr);
  else
    _client.unsubscribe(subscribeStr);
}

} // namespace stf

#endif
