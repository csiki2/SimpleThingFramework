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
  static int getArrayIndex(const char* str, uint strLen, const char* arr[], uint arrLen);
};

} // namespace stf
