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

class DataBuffer;
struct TaskDescriptor;

class TaskRoot : public Object {
public:
  TaskRoot();
  virtual ~TaskRoot();
  virtual void setup();
  virtual uint loop();

  //protected:
  TaskRoot(const TaskRoot&) = delete;
  TaskRoot& operator=(const TaskRoot&) = delete;

  TaskHandle_t _handle = nullptr;
  const TaskDescriptor* _descriptorPtr = nullptr;
  TaskRoot* _next = nullptr;
  DataBuffer* _bufferHead = nullptr;

  void initTask(const TaskDescriptor* descriptor);

  static int _count;

protected:
  static TaskRoot* _taskHead;

  static void setupTasks();
  static void loopTask(void* ptr);
  static uint loopTasks();

  friend void ::setup();
  friend void ::loop();
};

struct TaskDescriptor {
  const TaskRoot* task;
  const char* taskName;
  uint32_t taskStackSize;
  uint8_t taskCore;
  uint8_t taskOrder;
};

template <class T>
class Task : public TaskRoot {
public:
  inline Task() {}

  virtual void init();
  //protected:
  static T _obj;
  static const TaskDescriptor _descriptor;
};

template <class T>
T Task<T>::_obj;

template <class T>
void Task<T>::init() { initTask(&_descriptor); }

enum class EnumSimpleTask {
  Main = 0,
};

template <EnumSimpleTask ID>
class SimpleTask : public TaskRoot {
public:
  virtual void init();
  //protected:
  static SimpleTask<ID> _obj;
  static const TaskDescriptor _descriptor;
};

template <EnumSimpleTask ID>
SimpleTask<ID> SimpleTask<ID>::_obj;
template <EnumSimpleTask ID>
void SimpleTask<ID>::init() { initTask(&_descriptor); }

} // namespace stf
