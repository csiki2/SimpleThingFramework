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

#  include <PubSubClient.h>

namespace stf {

class MQTTConsumer : public Consumer {
public:
  void loop() override;
  bool isReady() override;
  uint32_t readyTime() override;

protected:
  bool send(JsonBuffer& jsonBuffer_) override;

  static StaticJsonBuffer<STFMQTT_JSONBUFFER_SIZE> jsonBuffer;
};

class PubSubClientWrapper {
public:
  static void setup();
  static uint32_t loop();

  static MQTTConsumer consumer;

protected:
  static PubSubClient client;
  static uint32_t connectionTime;
  static uint connectionTry;

  friend class MQTTConsumer;
};

} // namespace stf

#endif
