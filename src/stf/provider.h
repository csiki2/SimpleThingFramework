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

#include <stf/data_buffer.h>
#include <stf/task.h>

namespace stf {

class JsonBuffer;

struct FeedbackInfo {
  void set(const char* topic, const uint8_t* payload, unsigned int length);
  bool next();

  bool checkPayload(const char* str) const;
  void generateMAC();

  const char* topic;

  const uint8_t* fullPayload;
  uint fullPayloadLength;

  const uint8_t* payload;
  uint payloadLength;

  const char* topicStr;
  uint topicStrLen;

  const char* idStr;
  uint idStrLen;

  const char* fieldStr;
  uint fieldStrLen;

  bool retained;

  EnumTypeInfoTopic topicEnum;
  EnumDataField fieldEnum;
  uint8_t mac[8];
  uint macLen;
};

enum class ESystemMessageType : uint8_t {
  None = 0,
  Discovery = 1,
  Normal = 2,
  Retained = 4,
  All = 7
};

class DiscoveryBlock;

// The provider feeds data into the buffer
// Since the buffer is a lockless queue all provider for the same buffer must run on the same task
class Provider : public Object {
public:
  Provider(DataBuffer* buffer);

  virtual void init() override;
  virtual int initPriority() override;
  virtual Object** getObjectHead();

  virtual void setup();
  virtual uint loop() = 0;
  virtual uint systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type);
  virtual void feedback(const FeedbackInfo& info);

  bool isConsumerReady() const;

  static Provider* getNext(Provider* provider, DataBuffer* parentBuffer);

protected:
  bool handleSimpleFeedback(const FeedbackInfo& info, const DiscoveryBlock& block, uint8_t* mac, uint macLen, bool* value);

  DataBuffer* _parentBuffer;

  static Provider* _providerHead;

  friend class DataBuffer;
  friend class Consumer;
  friend class SystemProvider;
};

#define DEFINE_PROVIDERTASK(name, order, core, stackSize) name g_##name##Obj;

// A consumer can read one or more buffer
class Consumer {
public:
  Consumer();

  virtual bool isReady() = 0;
  virtual uint32_t ellapsedTimeSinceReady();

  void addBuffer(DataBuffer* buffer);
  DataBuffer* getNextBuffer(DataBuffer* buffer);

  virtual bool onCloseMessageEvent(JsonBuffer& jsonBuffer, DataCache& cache);

protected:
  virtual void consumeBuffers(JsonBuffer& jsonBuffer);
  int consumeBuffer(JsonBuffer& jsonBuffer, DataBuffer* buffer);
  virtual bool send(JsonBuffer& jsonBuffer, bool retain);

  void broadcastFeedback(const FeedbackInfo& info);

  DataBuffer* _bufferHead;
  ElapsedTime _readyTime;
  uint _messageCreated;
  uint _messageSent;
};

} // namespace stf
