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
#include <mutex>

namespace stf {

Object::~Object() {
  removeFromList(_objectHead, _objectMutex);
}

int Object::initPriority() {
  return 0;
}

Object** Object::getObjectHead() {
  return nullptr;
}

void Object::init() {
}

void Object::initObjects() {
  STFLOG_INFO("Init Objects - start\n");
  std::lock_guard<Mutex> lock(_objectMutex);
  int actPriority, nxtPriority = _objectHead != nullptr ? _objectHead->initPriority() : 0;
  for (Object* obj = _objectHead; obj != nullptr; obj = obj->_objectNext) {
    int priority = obj->initPriority();
    if (priority < nxtPriority) nxtPriority = priority;
  }
  do {
    actPriority = nxtPriority;
    STFLOG_INFO("Init Objects - priority %d\n", actPriority);
    for (Object** objPtr = &_objectHead; (*objPtr) != nullptr;) {
      Object* obj = *objPtr;
      Object* next = obj->_objectNext;
      int priority = obj->initPriority();
      if (priority == actPriority) {
        *objPtr = next; // _always_ remove from the list
        Object** newHead = obj->getObjectHead();
        if (newHead != nullptr) {
          obj->_objectNext = *newHead;
          *newHead = obj;
        } else {
          obj->_objectNext = nullptr;
        }
        obj->init();
      } else {
        objPtr = &obj->_objectNext;
      }
      if (priority > actPriority && (actPriority == nxtPriority || priority < nxtPriority)) nxtPriority = priority;
    }
  } while (nxtPriority > actPriority);
  STFLOG_INFO("Init Objects - end\n");
}

bool Object::addToList(Object*& head, Mutex& mutex) {
  std::lock_guard<Mutex> lock(mutex);
  _objectNext = head;
  head = this;
  return true;
}

bool Object::removeFromList(Object*& head, Mutex& mutex) {
  std::lock_guard<Mutex> lock(mutex);
  for (Object** next = &head; *next != nullptr; next = &(*next)->_objectNext) {
    if (*next == this) {
      *next = _objectNext;
      _objectNext = nullptr;
      return true;
    }
  }
  return false;
}

Object* Object::_objectHead = nullptr;
Mutex STFATTR_INIT(200) Object::_objectMutex;

} // namespace stf
