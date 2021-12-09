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
  if (STFLOG_LEVEL < level) return;
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_PRINT("memory used/free %6u/%6u\n", Host::_startingFreeHeap - heap, heap);
}

int Util::getArrayIndex(const char* str, uint strLen, const char* arr[], uint arrLen) {
  if (str == nullptr) return -1;
  for (uint idx = 0; idx < arrLen; idx++) {
    const char* cmp = arr[idx];
    if (strncmp(str, cmp, strLen) == 0 && cmp[strLen] == 0) return (int)idx;
  }
  return -1;
}

const char* Util::strchr(const char* strB, const char* strE, const char fnd) {
  for (; strB < strE; strB++)
    if (*strB == fnd) return strB;
  return nullptr;
}

const char* Util::stranychr(const char* strB, const char* strE, const char* fnd) {
  for (; strB < strE; strB++)
    if (::strchr(fnd, *strB) != nullptr) return strB;
  return nullptr;
}

void Util::writeHexToLog(const uint8_t* src, uint len, char hex10) {
  for (uint idx = 0; idx < len; idx++) {
    uint8_t chr = src[idx];
    uint8_t chr0 = chr >> 4, chr1 = chr & 15;
    STFLOG_WRITE(chr0 >= 10 ? chr0 + hex10 - (uint8_t)10 : chr0 + '0');
    STFLOG_WRITE(chr1 >= 10 ? chr1 + hex10 - (uint8_t)10 : chr1 + '0');
  }
}

void Util::writeHexToBuffer(uint8_t* buffer, const uint8_t* src, uint len, char hex10) {
  for (uint idx = 0; idx < len; idx++) {
    uint8_t chr = src[idx];
    uint8_t chr0 = chr >> 4, chr1 = chr & 15;
    buffer[0] = chr0 >= 10 ? chr0 + hex10 - (uint8_t)10 : chr0 + '0';
    buffer[1] = chr1 >= 10 ? chr1 + hex10 - (uint8_t)10 : chr1 + '0';
    buffer += 2;
  }
}

} // namespace stf
