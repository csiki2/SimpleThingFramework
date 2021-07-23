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

#if STFJSON == 1

#  include <stf/data_buffer.h>

namespace stf {

class JsonBuffer {
public:
  inline JsonBuffer(char* buffer_, uint size_) : buffer(buffer_), totalSize(size_) {}

  inline bool isValid() { return jsonSize < totalSize; }

  void start();
  void finish();

  void startElement(const DataBlock& block, DataCache& cache);
  void setElementFailed();
  void addDataBlock(const DataBlock& block_, DataCache& cache);
  static int blockToStr(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_);

  void logContent();

  //protected:
  inline bool addChar(const char chr) {
    if (pos < jsonSize) {
      buffer[pos++] = chr;
      return true;
    }
    return false;
  }

  uint pos;
  const char* elementName;
  uint elementPos;
  bool elementFailed;
  uint failCounter;

  char* buffer;
  uint jsonSize;
  uint totalSize;

  static const char openChars[];
  static const char closeChars[];
};

template <uint SIZE>
class StaticJsonBuffer : public JsonBuffer {
  char staticBuffer[SIZE];

public:
  inline StaticJsonBuffer() : JsonBuffer(staticBuffer, SIZE){};
};

} // namespace stf

#endif
