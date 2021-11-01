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

class Mutex;

class Object {
public:
  Object() { addToList(_objectHead, _objectMutex); }
  virtual ~Object();

  virtual void init();
  virtual int initPriority();
  virtual Object** getObjectHead();

  static void initObjects();

protected:
  bool addToList(Object*& head, Mutex& mutex);
  bool removeFromList(Object*& head, Mutex& mutex);

  static Object* _objectHead;
  static Mutex _objectMutex;
  Object* _objectNext;

  friend void ::setup();
  friend void ::loop();
};

} // namespace stf
