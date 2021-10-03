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

namespace stf {

TaskRoot::TaskRoot() {
}

TaskRoot::~TaskRoot() {
}

void TaskRoot::initTask(const TaskDescriptor2* descriptor) {
  _handle = nullptr;
  _descriptorPtr = descriptor;
  uint8_t order = descriptor->taskOrder;
  TaskRoot** task = &_head;
  while (*task != nullptr && (*task)->_descriptorPtr->taskOrder <= order)
    task = &((*task)->_next);
  _next = *task;
  (*task) = this;
  _count++;
}

void TaskRoot::setup() {
}

TaskRoot* TaskRoot::_head = nullptr;
int TaskRoot::_count = 0;

void TaskRoot::setupTasks() {
  STFLOG_INFO("TaskRoot::setupTasks started.\n");
  for (TaskRoot* tr = TaskRoot::_head; tr != nullptr; tr = tr->_next) {
    STFLOG_INFO("Calling setup method for %s\n", tr->_descriptorPtr->taskName);
    tr->setup();
  }
  STFLOG_INFO("Total number of tasks: %d\n", TaskRoot::_count);

  STFLOG_INFO("TaskRoot::setupTasks - rts task creation\n");
  for (TaskRoot* tr = TaskRoot::_head; tr != nullptr; tr = tr->_next) {
    const TaskDescriptor2* desc = tr->_descriptorPtr;
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
  for (TaskRoot* tr = TaskRoot::_head; tr != nullptr; tr = tr->_next) {
    if (tr->_descriptorPtr->taskStackSize == 0) {
      uint waitLocal = tr->loop();
      if (wait > waitLocal) wait = waitLocal;
    }
  }
  return wait;
}

TaskRegister* TaskRegister::_head = nullptr;

TaskRegister::TaskRegister(const TaskDescriptor* descriptor_) : _descriptor(descriptor_) {
  TaskRegister** descPtrPtr = &_head;
  while (*descPtrPtr != nullptr && (*descPtrPtr)->_descriptor->_taskOrder <= _descriptor->_taskOrder)
    descPtrPtr = &((*descPtrPtr)->_next);
  _next = *descPtrPtr;
  (*descPtrPtr) = this;
}

void taskCoreLoop(void* ptr) {
  TaskDescriptor* desc = (TaskDescriptor*)ptr;
  for (;;) {
    uint wait = desc->_funcLoop(desc->_userPtr);
    delay(wait);
  }
}

void task_setup() {
  STFLOG_INFO("Call task_setup\n");
  for (TaskRegister* next = TaskRegister::_head; next != nullptr; next = next->_next)
    next->_descriptor->_funcSetup(next->_descriptor->_userPtr);
  STFLOG_INFO("Call task_setup - rts task creation\n");
  for (TaskRegister* next = TaskRegister::_head; next != nullptr; next = next->_next) {
    const TaskDescriptor* desc = next->_descriptor;
    if (desc->_taskStackSize != 0) {
      int core = desc->_taskCore == tskNO_AFFINITY || desc->_taskCore < portNUM_PROCESSORS ? desc->_taskCore : 0;
      xTaskCreatePinnedToCore(taskCoreLoop, desc->_taskName, desc->_taskStackSize, (void*)desc, 1, &next->_handle, core);
    }
  }
  STFLOG_INFO("Call task_setup finished\n");
}

uint task_loop() {
  uint wait = 500;
  for (TaskRegister* next = TaskRegister::_head; next != nullptr; next = next->_next) {
    const TaskDescriptor* desc = next->_descriptor;
    if (desc->_taskStackSize == 0) {
      uint waitLocal = desc->_funcLoop(desc->_userPtr);
      if (wait > waitLocal) wait = waitLocal;
    }
  }
  return wait;
}

} // namespace stf
