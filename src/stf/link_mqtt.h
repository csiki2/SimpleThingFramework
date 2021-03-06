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

#if STFMQTT == 1

#  include <stf/json_buffer.h>
#  include <stf/provider.h>

#  include <WiFi.h>
#  include <PubSubClient.h>

namespace stf {

class MQTTConsumer : public Consumer {
public:
  static MQTTConsumer _obj;

  void setup();
  uint32_t loop();

  bool isReady() override;

protected:
  MQTTConsumer();
  bool send(JsonBuffer& jsonBuffer, bool retain) override;
  void localSubscribe(const char* topicFormat, bool subscribe = true);

  static void callback(char* topic, byte* payload, unsigned int length);

  StaticJsonBuffer<STFMQTT_JSONBUFFER_SIZE> _jsonBuffer;
  uint8_t _connectionTry = 0;
  uint8_t _messageArrived = 0;

  WiFiClient _wifiClient;
  PubSubClient _client;
};

} // namespace stf

#endif
