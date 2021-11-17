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

class Log {
public:
  static void connected(const char* name);
  static void connecting(const char* name, uint tryNum);
  static void memoryUsage(int level);
};

class Helper {
public:
  static const char* strchr(const char* strB, const char* strE, const char fnd);
  static const char* stranychr(const char* strB, const char* strE, const char* fnd);
  static int getArrayIndex(const char* str, uint strLen, const char* arr[], uint arrLen);
};

template <typename E>
class EnumClassFlags {
public:
  inline EnumClassFlags& operator+=(uint32_t flag) {
    _flags = (E)((uint32_t)_flags | flag);
    return *this;
  }
  inline EnumClassFlags& operator+=(E flag) { return operator+=((uint32_t)flag); }

  inline EnumClassFlags& operator-=(uint32_t flag) {
    _flags = (E)((uint32_t)_flags & ~flag);
    return *this;
  }
  inline EnumClassFlags& operator-=(E flag) { return operator-=((uint32_t)flag); }

  inline EnumClassFlags& operator=(E flag) {
    _flags = flag;
    return *this;
  }
  inline EnumClassFlags& operator=(uint32_t flag) { return operator=((E)flag); }

  inline bool isEmpty() const { return _flags == (E)0; }
  inline bool contains(uint32_t flag) const { return flag == ((uint32_t)_flags & flag); }
  inline bool contains(E flag) const { return contains((uint32_t)flag); }

  inline bool next(E& it) const {
    uint32_t nxt = (uint32_t)it;
    uint32_t flg = (uint32_t)_flags;
    if (nxt != 0) flg &= ~((nxt << 1) - 1);
    nxt = flg & -flg;
    it = (E)nxt;
    return nxt != 0;
  }

protected:
  E _flags = (E)0;
};

} // namespace stf
