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
#  include <stf/provider_system.h>
#  include <stf/provider_bt.h>

#  include <WiFi.h>
#  include <stdlib.h>

namespace stf {

void PubSubClientWrapper::setup() {
  STFLOG_INFO("Size of DataBlock: %u\n", sizeof(DataBlock));

  int port = strtol(mqttPort, nullptr, 10);
  STFLOG_INFO("MQTT Server - %s:%d\n", mqttServer, port);
  client.setServer(mqttServer, (uint16_t)port);
  client.setBufferSize(STFMQTT_JSONBUFFER_SIZE);
  consumer.addBuffer(&systemBuffer); // this is a wrong solution, will need refactor...
  consumer.addBuffer(&btBuffer); // this is a wrong solution, will need refactor...
}

uint32_t PubSubClientWrapper::loop() {
  if (client.connected()) {
    STFLED_COMMAND(STFLEDEVENT_MQTT_CONNECTED);
    consumer.loop();
    client.loop();
    return 10;
  }
  STFLED_COMMAND(STFLEDEVENT_MQTT_NOT_CONNECTED);
  if (WiFi.isConnected()) {
    uint64_t uptime = uptimeMS64();
    if (uptime - connectionTime > 5000 || connectionTry == 0) {
      connectionTry++;
      connectionTime = uptime;
      logConnecting("MQTT server", connectionTry);
      if (client.connect(hostName, mqttUser, mqttPassword)) {
        logConnected("MQTT server");
        connectionTry = 0;
        connectionTime = uptimeMS64() / 1000;
        return 10;
      } else {
        STFLOG_INFO("Unable to connect to the MQTT Server.\n");
      }
    }
  }
  return 0;
}

WiFiClient eClient;

PubSubClient PubSubClientWrapper::client(eClient);
uint32_t PubSubClientWrapper::connectionTime = 0;
uint PubSubClientWrapper::connectionTry = 0;
MQTTConsumer PubSubClientWrapper::consumer;

StaticJsonBuffer<STFMQTT_JSONBUFFER_SIZE> MQTTConsumer::jsonBuffer;

void MQTTConsumer::loop() {
  for (DataBuffer* buffer = bufferHead; buffer != nullptr; buffer = getNextBuffer(buffer)) {
    consumeBuffer(jsonBuffer, buffer);
  }
}

bool MQTTConsumer::isReady() { return PubSubClientWrapper::client.connected(); }
uint32_t MQTTConsumer::readyTime() { return PubSubClientWrapper::connectionTime; }

bool MQTTConsumer::send(JsonBuffer& jsonBuffer_) {
  //return false;
  return PubSubClientWrapper::client.publish(jsonBuffer_.buffer + jsonBuffer_.jsonSize, jsonBuffer_.buffer, jsonBuffer_.pos);
}

} // namespace stf

#endif
