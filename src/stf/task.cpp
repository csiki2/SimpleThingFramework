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

#include <stf/os.h>
#include <stf/data_buffer.h>

namespace stf {

// We always have "Main" task
template <>
const TaskDescriptor SimpleTask<EnumSimpleTask::Main>::_descriptor = {&_obj, "MainTask", 0, 0, 0};
template class SimpleTask<EnumSimpleTask::Main>;

TaskRoot::TaskRoot() {
}

TaskRoot::~TaskRoot() {
}

void TaskRoot::initTask(const TaskDescriptor* descriptor) {
  _descriptorPtr = descriptor;
  uint8_t order = descriptor->taskOrder;
  TaskRoot** task = &_taskHead;
  while (*task != nullptr && (*task)->_descriptorPtr->taskOrder <= order)
    task = &((*task)->_next);
  _next = *task;
  (*task) = this;
  _count++;
}

void TaskRoot::setup() {
  for (DataBuffer* db = _bufferHead; db != nullptr; db = (DataBuffer*)db->_objectNext) {
    STFLOG_INFO("TaskRoot::setup %p\n", this);
    db->setupProviders();
  }
}

uint TaskRoot::loop() {
  uint wait = 100;
  for (DataBuffer* db = _bufferHead; db != nullptr; db = (DataBuffer*)db->_objectNext) {
    uint res = db->loopProviders();
    if (res < wait) wait = res;
  }

  return wait;
}

TaskRoot* TaskRoot::_taskHead = nullptr;
int TaskRoot::_count = 0;

void TaskRoot::setupTasks() {
  STFLOG_INFO("TaskRoot::setupTasks started.\n");
  for (TaskRoot* tr = TaskRoot::_taskHead; tr != nullptr; tr = tr->_next) {
    STFLOG_INFO("Calling setup method for %s\n", tr->_descriptorPtr->taskName);
    tr->setup();
  }
  STFLOG_INFO("Total number of tasks: %d\n", TaskRoot::_count);

  STFLOG_INFO("TaskRoot::setupTasks - rts task creation\n");
  for (TaskRoot* tr = TaskRoot::_taskHead; tr != nullptr; tr = tr->_next) {
    const TaskDescriptor* desc = tr->_descriptorPtr;
    if (desc->taskStackSize != 0) {
      int core = desc->taskCore == tskNO_AFFINITY || desc->taskCore < portNUM_PROCESSORS ? desc->taskCore : 0;
      xTaskCreatePinnedToCore(loopTask, desc->taskName, desc->taskStackSize, (void*)tr, 1, &tr->_handle, core);
    }
  }
  STFLOG_INFO("TaskRoot::setupTasks ended.\n");
}

void TaskRoot::loopTask(void* ptr) {
  TaskRoot* tr = (TaskRoot*)ptr;
  for (;;) {
    uint wait = tr->loop();
    delay(wait);
  }
}

uint TaskRoot::loopTasks() {
  uint wait = 500;
  for (TaskRoot* tr = TaskRoot::_taskHead; tr != nullptr; tr = tr->_next) {
    if (tr->_descriptorPtr->taskStackSize == 0) {
      uint waitLocal = tr->loop();
      if (wait > waitLocal) wait = waitLocal;
    }
  }
  return wait;
}

} // namespace stf
