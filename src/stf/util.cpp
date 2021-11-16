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

#include <stf/util.h>

namespace stf {

void Log::connected(const char* name) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_INFO("Connected to the %s, memory used/free %6u/%6u\n", name, Host::_startingFreeHeap - heap, heap);
}

void Log::connecting(const char* name, uint tryNum) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_INFO("Connecting to the %s (try %u), memory used/free %6u/%6u\n", name, tryNum, Host::_startingFreeHeap - heap, heap);
}

void Log::memoryUsage(int level) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_PRINT(level, "memory used/free %6u/%6u\n", Host::_startingFreeHeap - heap, heap);
}

int Helper::getArrayIndex(const char* str, uint strLen, const char* arr[], uint arrLen) {
  if (str == nullptr) return -1;
  for (uint idx = 0; idx < arrLen; idx++) {
    const char* cmp = arr[idx];
    if (strncmp(str, cmp, strLen) == 0 && cmp[strLen] == 0) return (int)idx;
  }
  return -1;
}

const char* Helper::strchr(const char* strB, const char* strE, const char fnd) {
  for (; strB < strE; strB++)
    if (*strB == fnd) return strB;
  return nullptr;
}

const char* Helper::stranychr(const char* strB, const char* strE, const char* fnd) {
  for (; strB < strE; strB++)
    if (::strchr(fnd, *strB) != nullptr) return strB;
  return nullptr;
}

} // namespace stf
