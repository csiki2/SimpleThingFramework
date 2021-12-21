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
#include <stf/provider_system.h>
#include <stf/data_discovery.h>
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

bool Provider::handleSimpleFeedback(const FeedbackInfo& info, const DiscoveryBlock& block, uint8_t* mac, uint macLen, bool* value) {
  if (mac != nullptr && (macLen != info.macLen || memcmp(info.mac, mac, macLen) != 0)) return false;
  if (info.fieldEnum == block._field) {
    if (block._component == edcButton && info.checkPayload("PRESS")) {
      if (value != nullptr) *value = true;
      STFLOG_INFO("%s command detected - %*.*s\n", block.getName(), info.payloadLength, info.payloadLength, info.payload);
      return true;
    }
    if (block._component == edcSwitch) {
      bool set = info.checkPayload("ON");
      bool rrq = set != *value && strstr(info.topic, "/command/") != nullptr;
      STFLOG_INFO("%s command detected %u from %u - %*.*s%s\n", block.getName(), set, *value, info.payloadLength, info.payloadLength, info.payload, rrq ? " - report request" : "");
      if (set != *value) {
        *value = set;
        if (rrq) SystemProvider::requestRetainedReport();
        return true;
      }
    }
  }
  return false;
}

Consumer::Consumer() : _bufferHead(nullptr) {
  _messageCreated = _messageSent = 0;
}

uint32_t Consumer::ellapsedTimeSinceReady() {
  return _readyTime.elapsedTime();
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
    STFLOG_INFO("Sending %sMQTT message (%s) %s.\n", cache._flagRetain ? "retained " : "", jsonBuffer.getTopic("no topic"), res ? "succeeded" : "failed");
    jsonBuffer.log(STFLOG_LEVEL_INFO, false);
  } else {
    STFLOG_INFO("Invalid MQTT message (%s).\n", jsonBuffer.getTopic("no topic"));
    jsonBuffer.log(STFLOG_LEVEL_INFO, false);
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
  fullPayload = payload = payloadInput;
  fullPayloadLength = payloadLength = payloadLengthInput;
  retained = false;

  const char *fndB, *fndE;

  if ((fndE = strstr(topic, "toMQTT/")) != nullptr && (fndB = (const char*)memrchr(topic, '/', fndE - topic)) != nullptr) {
    // Received MQTT message (home/SimpleThing_Test/SYSRtoMQTT/EspDJ_AABBCC) [SYSR|AC67B2AABBCC|][1|-1] - {"ota":"OFF"}
    topicStr = fndB + 1;
    topicStrLen = fndE - topicStr;
    topicEnum = DataType::findTopicName(topicStr, topicStrLen);
    idStr = Host::_info.strMAC;
    idStrLen = strlen(idStr);
    memcpy(mac, Host::_info.mac, macLen = Host::_info.macLen);
    retained = true;
    fieldStr = "";
    fieldStrLen = 0;
    fieldEnum = EnumDataField::edf__none;
    payloadLength = 0;
  } else if ((fndB = strstr(topic, "/MQTTto")) != nullptr && (fndE = strchr(fndB + 1, '/')) != nullptr) {
    // Received MQTT message (home/SimpleThing_Test/MQTTtoSYSR/EspDJ_AABBCC/command/AC67B2AABBCC_ota) [SYSR|AC67B2AABBCC|ota][1|19] - ON
    topicStr = fndB + 7;
    topicStrLen = fndE - topicStr;
    topicEnum = DataType::findTopicName(topicStr, topicStrLen);
    idStr = (fndB = strrchr(topic, '/')) != nullptr ? fndB + 1 : "";
    fieldStr = (idStr != nullptr && (fndB = strchr(idStr, '_')) != nullptr) ? fndB + 1 : "";
    fieldStrLen = strlen(fieldStr);
    idStrLen = fieldStr != nullptr ? fndB - idStr : 0;
    generateMAC();
    int idx = Util::getArrayIndex(fieldStr, fieldStrLen, DataField::_list, DataField::_listNum);
    fieldEnum = (EnumDataField)(idx >= 0 ? idx : 0);
  } else {
    topicStr = idStr = fieldStr = "";
    topicStrLen = idStrLen = fieldStrLen = 0;
    topicEnum = EnumTypeInfoTopic::etitNONE;
    fieldEnum = EnumDataField::edf__none;
    macLen = 0;
  }
}

bool FeedbackInfo::next() {
  if (!retained) return false;

  const char *fndB, *fndE;
  const char* str = (const char*)payload + payloadLength;
  if (*str == '"') str++;

  fieldStr = "";
  payload = fullPayload + fullPayloadLength;
  fieldStrLen = payloadLength = 0;

  const char* pe = (const char*)payload;

  fndB = Util::strchr(str, pe, '"');
  if (fndB == nullptr) return false;
  fndE = Util::strchr(fndB + 1, pe, '"');
  if (fndE == nullptr) return false;

  fieldStr = fndB + 1;
  fieldStrLen = fndE - fieldStr;
  int idx = Util::getArrayIndex(fieldStr, fieldStrLen, DataField::_list, DataField::_listNum);
  fieldEnum = (EnumDataField)(idx >= 0 ? idx : 0);

  fndB = Util::strchr(fndE, pe, ':');
  if (fndB == nullptr) return false;
  while (++fndB < pe && isspace(*fndB))
    ;
  if (fndB == pe || *fndB == '{' || *fndB == '[') return false; // not supported
  if (*fndB != '"') { // Easy case
    if ((fndE = Util::stranychr(fndB, pe, ",}")) == nullptr) return false;
    while (fndB < fndE && isspace(fndE[-1])) fndE--;
    payload = (const uint8_t*)fndB;
    payloadLength = fndE - fndB;
    return true;
  }
  for (fndE = ++fndB;;) {
    if ((fndE = Util::strchr(fndE, pe, '"')) == nullptr) return false;
    if (fndE[-1] != '\'') break;
  }
  payload = (const uint8_t*)fndB;
  payloadLength = fndE - fndB;
  return true;
}

bool FeedbackInfo::checkPayload(const char* str) const {
  return payloadLength == strlen(str) && strncmp((const char*)payload, str, payloadLength) == 0;
}

void FeedbackInfo::generateMAC() {
  macLen = 0;
  if (idStrLen % 2 == 1) return;
  for (int idx = 0; idx < idStrLen; idx += 2) {
    uint c0 = tolower(idStr[idx]);
    uint c1 = tolower(idStr[idx + 1]);
    if ((c0 < 'a' || c0 > 'f') && (c0 < '0' || c0 > '9')) return;
    if ((c1 < 'a' || c1 > 'f') && (c1 < '0' || c1 > '9')) return;
    c0 -= c0 <= '9' ? '0' : 'a' - 10;
    c1 -= c1 <= '9' ? '0' : 'a' - 10;
    mac[idx / 2] = (c0 << 4) + c1;
  }
  macLen = idStrLen / 2;
}

} // namespace stf
