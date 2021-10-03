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

namespace stf {

struct TaskDescriptor2;

class TaskRoot {
public:
  TaskRoot();
  virtual ~TaskRoot();
  virtual void setup();
  virtual uint loop() = 0;

  //protected:
  TaskRoot(const TaskRoot&) = delete;
  TaskRoot& operator=(const TaskRoot&) = delete;

  TaskHandle_t _handle;
  const TaskDescriptor2* _descriptorPtr;

  void initTask(const TaskDescriptor2* descriptor);
  static TaskRoot* _head;
  TaskRoot* _next;
  static int _count;

protected:
  static void setupTasks();
  static void loopTask(void* ptr);
  static uint loopTasks();

  friend void ::setup();
  friend void ::loop();
};

struct TaskDescriptor2 {
  const TaskRoot* task;
  const char* taskName;
  uint32_t taskStackSize;
  uint8_t taskCore;
  uint8_t taskOrder;
};

template <class T>
class Task : public TaskRoot {
public:
  inline Task() { initTask(&_descriptor); }
  //protected:
  static T _obj;
  static const TaskDescriptor2 _descriptor;
};

template <class T>
T Task<T>::_obj;

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
