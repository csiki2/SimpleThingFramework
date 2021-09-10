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

#include <stf/data_block.h>
#include <stf/data_cache.h>
#include <stf/data_feeder.h>
#include <stf/json_buffer.h>
#include <stf/provider.h>
#include <stf/util.h>

namespace stf {

Provider* Provider::systemHead = nullptr;

Provider::Provider(DataBuffer* buffer_) : buffer(buffer_) {
  systemNext = nullptr;
  providerNext = buffer->providerHead;
  buffer->providerHead = this;
};

void Provider::setup() {
}

bool Provider::isConsumerReady() const {
  if (buffer == nullptr) return false;
  Consumer* cons = buffer->getConsumer();
  return cons != nullptr && cons->isReady();
}

void Provider::registerSystemUpdate() {
  systemNext = systemHead;
  systemHead = this;
}

uint Provider::systemDiscovery(DataBuffer* systemBuffer_) {
  return 0;
}

uint Provider::systemUpdate(DataBuffer* systemBuffer_, uint32_t uptimeS_) {
  return 0;
}

void Provider::feedback(const FeedbackInfo& info_) {
}

void setupProviderTask(void* ptr_) {
  ((Provider*)ptr_)->setup();
}

uint loopProviderTask(void* ptr_) {
  return ((Provider*)ptr_)->loop();
}

Consumer::Consumer() : bufferHead(nullptr) {
  messageCreated = messageSent = 0;
}

uint32_t Consumer::readyTime() {
  return 0ull;
}

void Consumer::addBuffer(DataBuffer* buffer) {
  buffer->bufferNext = bufferHead;
  buffer->parentConsumer = this;
  bufferHead = buffer;
}

DataBuffer* Consumer::getNextBuffer(DataBuffer* buffer) {
  return buffer != nullptr ? buffer->bufferNext : nullptr;
}

bool Consumer::send(JsonBuffer& jsonBuffer_) {
  return false;
}

bool Consumer::onCloseMessageEvent(JsonBuffer& jsonBuffer_, DataCache& cache_) {
  jsonBuffer_.finish();
  messageCreated++;
  bool res = false;
  if (jsonBuffer_.isValid()) {
    // res = false;
    res = send(jsonBuffer_);
    if (res) messageSent++;
    STFLOG_INFO("Sending MQTT message (%s) %s.\n", jsonBuffer_.buffer + jsonBuffer_.jsonSize, res ? "succeeded" : "failed");
  } else {
    STFLOG_INFO("Invalid MQTT message.\n");
  }
  jsonBuffer_.start();
  cache_.reset();
  return res;
}

void Consumer::consumeBuffers(JsonBuffer& jsonBuffer_) {
  for (DataBuffer* buffer = bufferHead; buffer != nullptr; buffer = getNextBuffer(buffer)) {
    consumeBuffer(jsonBuffer_, buffer);
  }
}

int Consumer::consumeBuffer(JsonBuffer& jsonBuffer_, DataBuffer* buffer) {
  messageSent = messageCreated = 0;
  if (!buffer->hasClosedMessage()) return 0;

  DataCache cache;
  jsonBuffer_.start();
  cache.forceReset();
  do {
    bool end = false;
    while (!end) {
      DataBlock& block = buffer->getReadBlock();
      if (block._type == edt_Generator) {
        DataFeeder feeder(*this, jsonBuffer_);
        feeder.consumeGeneratorBlock(block, cache);
      } else {
        jsonBuffer_.addDataBlock(block, cache);
        if (block.isClosedMessage()) onCloseMessageEvent(jsonBuffer_, cache);
      }
      end = block.isClosedMessage();
      buffer->IncrementReadIndex();
    }
  } while (buffer->hasClosedMessage());
  return messageSent;
}

void Consumer::broadcastFeedback(const FeedbackInfo& info_) {
  for (DataBuffer* buffer = bufferHead; buffer != nullptr; buffer = buffer->bufferNext) {
    for (Provider* provider = buffer->providerHead; provider != nullptr; provider = provider->providerNext)
      provider->feedback(info_);
  }
}

void FeedbackInfo::set(const char* topic_, const uint8_t* payload_, unsigned int length_) {
  topic = topic_;
  payload = payload_;
  payloadLength = length_;

  const char *fndB, *fndE;

  // Received MQTT message (home/SimpleThing_Test/MQTTtoSYS/EspDJ_295138/command/AC67B2295138_ota_switch)

  if ((fndB = strstr(topic, "MQTTto")) != nullptr && (fndE = strchr(fndB, '/')) != nullptr) {
    topicStr = fndB + 6;
    topicStrLen = fndE - topicStr;
    topicEnum = findTopicInfo(topicStr, topicStrLen);
  } else {
    topicStr = nullptr;
    topicStrLen = 0;
    topicEnum = (EnumTypeInfoTopic)-1;
  }

  idStr = (fndB = strrchr(topic, '/')) != nullptr ? fndB + 1 : nullptr;
  fieldStr = (idStr != nullptr && (fndB = strchr(idStr, '_')) != nullptr) ? fndB + 1 : nullptr;
  idStrLen = fieldStr != nullptr ? fndB - idStr : 0;

  fieldEnum = (EnumDataField)getArrayIndex(fieldStr, strlen(fieldStr), DataField::list, DataField::listNum);

  //const uint8_t mac[8]; TODO
  //uint macLen;
}

} // namespace stf
