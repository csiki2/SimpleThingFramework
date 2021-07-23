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

extern StaticDataBuffer<STFQUEUE_SIZE> dataBuffer;

// The provider feeds data into the buffer
// Since the buffer is a lockless queue all provider for the same buffer must run on the same task
class Provider {
public:
  Provider(DataBuffer* buffer_);

  virtual void setup();
  virtual uint loop() = 0;
  virtual uint systemDiscovery(DataBuffer* systemBuffer_);
  virtual uint systemUpdate(DataBuffer* systemBuffer_, uint32_t uptimeS_);

  void registerSystemUpdate();
  bool isConsumerReady() const;

protected:
  DataBuffer* buffer;
  Provider* providerNext;
  Provider* systemNext;
  static Provider* systemHead;

  friend class SystemProvider;
};

#define DEFINE_PROVIDERTASK(name, order, core, stackSize)                                                                         \
  name name##Obj;                                                                                                                 \
  void setupProviderTask(void*);                                                                                                  \
  uint loopProviderTask(void*);                                                                                                   \
  const TaskDescriptor descriptor##name##Task = {setupProviderTask, loopProviderTask, &name##Obj, #name, stackSize, core, order}; \
  TaskRegister register##name##Task(&descriptor##name##Task);

// A consumer can read one or more buffer
class Consumer {
public:
  Consumer();

  virtual void loop() = 0;
  virtual bool isReady() = 0;
  virtual uint32_t readyTime();

  void addBuffer(DataBuffer* buffer);
  DataBuffer* getNextBuffer(DataBuffer* buffer);

  virtual bool onCloseMessageEvent(JsonBuffer& jsonBuffer_, DataCache& cache_);

protected:
  int consumeBuffer(JsonBuffer& jsonBuffer_, DataBuffer* buffer_);
  virtual bool send(JsonBuffer& jsonBuffer_);

  DataBuffer* bufferHead;
  uint messageCreated;
  uint messageSent;
};

} // namespace stf
