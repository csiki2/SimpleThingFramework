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
  inline JsonBuffer(char* buffer, uint size) : _buffer(buffer), _totalSize(size) {}

  inline bool isValid() { return _jsonSize < _totalSize; }

  void start();
  void finish();

  void startElement(const DataBlock& block, DataCache& cache);
  void setElementFailed();
  void addDataBlock(const DataBlock& block_, DataCache& cache);
  static int blockToStr(char* buffer, uint len, const DataBlock& block, DataCache& cache);

  const char* getTopic(const char* onEmpty = nullptr) const;

  void log(int level = STFLOG_LEVEL_INFO, bool topic = true, bool value = true);

  //protected:
  inline bool addChar(const char chr) {
    if (_pos < _jsonSize) {
      _buffer[_pos++] = chr;
      return true;
    }
    return false;
  }

  uint _pos;
  const char* _elementName;
  uint _elementPos;
  bool _elementFailed;
  uint _failCounter;

  char* _buffer;
  uint _jsonSize;
  uint _totalSize;

  static const char _openChars[];
  static const char _closeChars[];
};

template <uint SIZE>
class StaticJsonBuffer : public JsonBuffer {
  char _staticBuffer[SIZE];

public:
  inline StaticJsonBuffer() : JsonBuffer(_staticBuffer, SIZE){};
};

} // namespace stf

#endif
