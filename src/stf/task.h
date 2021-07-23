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

namespace stf {

typedef void (*TaskSetupFunction)(void*);
typedef uint (*TaskLoopFunction)(void*);

struct TaskDescriptor {
  TaskSetupFunction _funcSetup;
  TaskLoopFunction _funcLoop;
  void* _userPtr;

  const char* _taskName;
  uint _taskStackSize;
  int _taskCore;

  int _taskOrder;
};

class TaskRegister {
public:
  TaskRegister(const TaskDescriptor* descriptor_);

  const TaskDescriptor* _descriptor;
  TaskHandle_t _handle;
  TaskRegister* _next;

  static TaskRegister* _head;
};

void task_setup();
uint task_loop();

#define DEFINE_STFTASK(name, order, core, stackSize, userPtr)                                                                  \
  void setup##name##Task(void*);                                                                                               \
  uint loop##name##Task(void*);                                                                                                \
  const TaskDescriptor descriptor##name##Task = {setup##name##Task, loop##name##Task, userPtr, #name, stackSize, core, order}; \
  TaskRegister register##name##Task(&descriptor##name##Task);

} // namespace stf
