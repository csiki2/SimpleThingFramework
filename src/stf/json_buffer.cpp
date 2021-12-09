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

#include <stf/data_block.h>
#include <stf/data_cache.h>
#include <stf/json_buffer.h>

#if STFJSON == 1

namespace stf {

void JsonBuffer::log(int level, bool topic, bool value) {
  if (STFLOG_LEVEL < level || (!topic && !value)) return;
  // We will use this frequently, avoid using STFLOG_PRINT
  STFLOG_WRITE("JsonBuffer");
  if (topic) {
    STFLOG_WRITE(" (");
    STFLOG_WRITE(getTopic("no topic"));
    STFLOG_WRITE(')');
  }
  if (value) {
    STFLOG_WRITE(" - ");
    STFLOG_WRITE(_buffer);
  }
  STFLOG_WRITE('\n');
}

const char* JsonBuffer::getTopic(const char* onEmpty) const {
  return _jsonSize != _totalSize ? _buffer + _jsonSize : onEmpty;
}

void JsonBuffer::start() {
  _failCounter = 0;
  _pos = 0;
  _jsonSize = _totalSize;
  addChar('{');
}

void JsonBuffer::finish() {
  addChar('}');
  if (_pos < _jsonSize) _buffer[_pos] = 0;
  if (_failCounter > 0) STFLOG_WARNING("JsonBuffer Failed to resolve all the data blocks (%u)\n", _failCounter);
}

void JsonBuffer::setElementFailed() {
  _pos = _elementPos;
  _elementFailed = true;
  _failCounter++;
}

void JsonBuffer::startElement(const DataBlock& block, DataCache& cache) {
  _elementPos = _pos;
  _elementName = DataField::_list[block._field];
  _elementFailed = false;
  if (block._field == edf__cont) return;

  int avail = _jsonSize - _pos;
  char prevChar = _pos > 0 ? _buffer[_pos - 1] : '{'; // pos always should be >0
  int res = snprintf(_buffer + _pos, avail, "%s\"%s\":", prevChar == '{' || prevChar == '[' ? "" : ",", _elementName);
  if (res < 0 || res >= avail)
    setElementFailed();
  else
    _pos += res;
}

void JsonBuffer::addDataBlock(const DataBlock& block, DataCache& cache) {
  DataType type = DataType::_list[block._type];
  if ((type._support & etSupportSaveToCache) != 0) cache.addBlock(block, block._extra);

  if (type._coreType == ectNone) {
    blockToStr(nullptr, 0, block, cache);
    return;
  }
  if (block._field == edf__topic) {
    int len = blockToStr(_buffer + _jsonSize, 0, block, cache);
    if (len >= 0 && _pos + len + 1 < _jsonSize) {
      _jsonSize -= len + 1;
      blockToStr(_buffer + _jsonSize, len + 1, block, cache);
    } else {
      _elementFailed = true;
      _jsonSize = _totalSize;
      _failCounter++;
    }
    return;
  }

  if (cache._headElem._field == edf__none) startElement(block, cache);
  if (!_elementFailed) {
    int idx = blockToStr(_buffer + _pos, _jsonSize - _pos, block, cache);
    if (idx >= 0 && idx < _jsonSize - _pos)
      _pos += idx;
    else
      setElementFailed();
  }
  if ((type._support & etSupportDoubleField) == 0 || (block._typeInfo & etiDoubleField) == 0) return;
  // Let's go for one more round :)
  DataBlock block2 = block;
  block2._field = (EnumDataField)block._extra;
  block2._value.t32[0] = block2._value.t32[1];
  if (cache._headElem._field == edf__none) startElement(block2, cache);
  if (!_elementFailed) {
    int idx = blockToStr(_buffer + _pos, _jsonSize - _pos, block2, cache);
    if (idx >= 0 && idx < _jsonSize - _pos)
      _pos += idx;
    else
      setElementFailed();
  }
}

// buffer is not allowed to be nullptr, buffer[-1] must be valid
int JsonBuffer::blockToStr(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  const DataType& type = DataType::_list[block._type];
  if (type._coreType == ectNone) { // Just call it...
    type._toStr(nullptr, 0, block, cache);
    return 0;
  }
  if (block._field == edf__none) return 0;

  bool startElem = cache._headElem._field == edf__none;
  bool complexElem = !startElem || (type._coreType == ectArray || type._coreType == ectObject);
  bool continueElem = block._field == edf__cont;

  uint tlen = 0, clen = 0;
  char oc = _openChars[type._coreType];
  if (complexElem) {
    if (startElem) {
      cache.setHeadElem(block);
      if (tlen < len) buffer[tlen] = oc;
      tlen++;
    } else if (block._field != edf__topic) {
      char prevChar = buffer[-1]; // safe since we continue the previous block
      if (prevChar != '[' && prevChar != '{') {
        if (tlen < len) buffer[tlen] = ',';
        tlen++;
      }
    }
  }

  bool qm = type._coreType == ectString || (type._coreType == ectTopic && block._field != edf__topic);
  if (continueElem) {
    if (buffer[-1] == '\"') {
      clen = 1;
      if (len > 0) { // if we are not in measure mode, we can write one plus char
        buffer--;
        len++;
      }
    }
  } else if (qm) {
    if (tlen < len) buffer[tlen] = '\"';
    tlen++;
  }

  int res = type._toStr(buffer + tlen, tlen < len ? len : 0, block, cache);
  if (res < 0) return res;
  tlen += res;
  if (qm) {
    if (tlen < len) buffer[tlen] = '\"';
    tlen++;
  }
  if (len == 0 && !qm && !startElem && tlen == 1) tlen = 0; // there was no data, remove the semicolon
  if (complexElem && block._closeComplexFlag) {
    cache._headElem._field = edf__none;
    if (tlen < len) buffer[tlen] = _closeChars[type._coreType];
    tlen++;
  }
  return tlen - clen;
}

/*
enum EnumCoreType {
  ectNone,
  ectTopic,
  ectNumber,
  ectString,
  ectArray,
  ectObject,
};
*/

const char JsonBuffer::_openChars[] = {' ', ' ', ' ', '\"', '[', '{'};
const char JsonBuffer::_closeChars[] = {' ', ' ', ' ', '\"', ']', '}'};

} // namespace stf

#endif
