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

Provider* Provider::_providerHead = nullptr;

Provider::Provider(DataBuffer* buffer) : _parentBuffer(buffer){};

void Provider::init() {
}

int Provider::initPriority() {
  return 20;
}

Object** Provider::getObjectHead() {
  return (Object**)&_providerHead;
}

Provider* Provider::getNext(Provider* provider, DataBuffer* parentBuffer) {
  provider = provider == nullptr ? _providerHead : (Provider*)provider->_objectNext;
  while (provider != nullptr && provider->_parentBuffer != parentBuffer)
    provider = (Provider*)provider->_objectNext;
  return provider;
}

void Provider::setup() {
}

bool Provider::isConsumerReady() const {
  if (_parentBuffer == nullptr) return false;
  Consumer* cons = _parentBuffer->getConsumer();
  return cons != nullptr && cons->isReady();
}

uint Provider::systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) {
  return 0;
}

void Provider::feedback(const FeedbackInfo& info) {
}

Consumer::Consumer() : _bufferHead(nullptr) {
  _messageCreated = _messageSent = 0;
}

uint32_t Consumer::readyTime() {
  return 0ull;
}

void Consumer::addBuffer(DataBuffer* buffer) {
  buffer->_consumerBufferNext = _bufferHead;
  buffer->_parentConsumer = this;
  _bufferHead = buffer;
}

DataBuffer* Consumer::getNextBuffer(DataBuffer* buffer) {
  return buffer != nullptr ? buffer->_consumerBufferNext : nullptr;
}

bool Consumer::send(JsonBuffer& jsonBuffer, bool retain) {
  return false;
}

bool Consumer::onCloseMessageEvent(JsonBuffer& jsonBuffer, DataCache& cache) {
  jsonBuffer.finish();
  _messageCreated++;
  bool res = false;
  if (jsonBuffer.isValid()) {
    // res = false;
    res = send(jsonBuffer, cache._flagRetain);
    if (res) _messageSent++;
    STFLOG_INFO("Sending %s MQTT message (%s) %s.\n", cache._flagRetain ? "retained" : "", jsonBuffer._buffer + jsonBuffer._jsonSize, res ? "succeeded" : "failed");
  } else {
    STFLOG_INFO("Invalid MQTT message.\n");
  }
  jsonBuffer.start();
  cache.reset();
  return res;
}

void Consumer::consumeBuffers(JsonBuffer& jsonBuffer) {
  for (DataBuffer* buffer = _bufferHead; buffer != nullptr; buffer = getNextBuffer(buffer)) {
    consumeBuffer(jsonBuffer, buffer);
  }
}

int Consumer::consumeBuffer(JsonBuffer& jsonBuffer, DataBuffer* buffer) {
  _messageSent = _messageCreated = 0;
  if (!buffer->hasClosedMessage()) return 0;

  DataCache cache;
  jsonBuffer.start();
  cache.forceReset();
  do {
    bool end = false;
    while (!end) {
      DataBlock& block = buffer->getReadBlock();
      if (block._type == edt_Generator) {
        DataFeeder feeder(*this, jsonBuffer);
        feeder.consumeGeneratorBlock(block, cache);
      } else {
        jsonBuffer.addDataBlock(block, cache);
        if (block.isClosedMessage()) onCloseMessageEvent(jsonBuffer, cache);
      }
      end = block.isClosedMessage();
      buffer->IncrementReadIndex();
    }
  } while (buffer->hasClosedMessage());
  return _messageSent;
}

void Consumer::broadcastFeedback(const FeedbackInfo& info) {
  for (DataBuffer* buffer = _bufferHead; buffer != nullptr; buffer = buffer->_consumerBufferNext) {
    for (Provider* provider = Provider::getNext(nullptr, buffer); provider != nullptr; provider = Provider::getNext(provider, buffer))
      provider->feedback(info);
  }
}

void FeedbackInfo::set(const char* topicInput, const uint8_t* payloadInput, unsigned int payloadLengthInput) {
  topic = topicInput;
  payload = payloadInput;
  payloadLength = payloadLengthInput;

  const char *fndB, *fndE;

  // Received MQTT message (home/SimpleThing_Test/MQTTtoSYS/EspDJ_295138/command/AC67B2295138_ota_switch)

  if ((fndB = strstr(topic, "MQTTto")) != nullptr && (fndE = strchr(fndB, '/')) != nullptr) {
    topicStr = fndB + 6;
    topicStrLen = fndE - topicStr;
    topicEnum = DataType::findTopicName(topicStr, topicStrLen);
  } else {
    topicStr = nullptr;
    topicStrLen = 0;
    topicEnum = (EnumTypeInfoTopic)-1;
  }

  idStr = (fndB = strrchr(topic, '/')) != nullptr ? fndB + 1 : nullptr;
  fieldStr = (idStr != nullptr && (fndB = strchr(idStr, '_')) != nullptr) ? fndB + 1 : nullptr;
  idStrLen = fieldStr != nullptr ? fndB - idStr : 0;

  fieldEnum = (EnumDataField)Helper::getArrayIndex(fieldStr, strlen(fieldStr), DataField::_list, DataField::_listNum);

  //const uint8_t mac[8]; TODO
  //uint macLen;
}

} // namespace stf
