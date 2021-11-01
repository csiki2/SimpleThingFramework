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

void JsonBuffer::logContent() {
  STFLOG_INFO("JsonBuffer topic: %s\n", jsonSize == totalSize ? "no_topic" : buffer + jsonSize);
  STFLOG_INFO("JsonBuffer result: %s\n", buffer);
}

void JsonBuffer::start() {
  failCounter = 0;
  pos = 0;
  jsonSize = totalSize;
  addChar('{');
}

void JsonBuffer::finish() {
  addChar('}');
  if (pos < jsonSize) buffer[pos] = 0;
  logContent();
  if (failCounter > 0) STFLOG_WARNING("JsonBuffer Failed to resolve all the data blocks (%u)\n", failCounter);
}

void JsonBuffer::setElementFailed() {
  pos = elementPos;
  elementFailed = true;
  failCounter++;
}

void JsonBuffer::startElement(const DataBlock& block, DataCache& cache_) {
  elementPos = pos;
  elementName = DataField::_list[block._field];
  elementFailed = false;
  if (block._field == edf__cont) return;

  int avail = jsonSize - pos;
  char prevChar = pos > 0 ? buffer[pos - 1] : '{'; // pos always should be >0
  int res = snprintf(buffer + pos, avail, "%s\"%s\":", prevChar == '{' || prevChar == '[' ? "" : ",", elementName);
  if (res < 0 || res >= avail)
    setElementFailed();
  else
    pos += res;
}

void JsonBuffer::addDataBlock(const DataBlock& block_, DataCache& cache_) {
  DataType type = DataType::_list[block_._type];
  if ((type._support & etSupportSaveToCache) != 0) cache_.addBlock(block_, block_._extra);

  if (type._coreType == ectNone) {
    blockToStr(nullptr, 0, block_, cache_);
    return;
  }
  if (block_._field == edf__topic) {
    int len = blockToStr(buffer + jsonSize, 0, block_, cache_);
    if (len >= 0 && pos + len + 1 < jsonSize) {
      jsonSize -= len + 1;
      blockToStr(buffer + jsonSize, len + 1, block_, cache_);
    } else {
      elementFailed = true;
      jsonSize = totalSize;
      failCounter++;
    }
    return;
  }

  if (cache_._headElem._field == edf__none) startElement(block_, cache_);
  if (!elementFailed) {
    int idx = blockToStr(buffer + pos, jsonSize - pos, block_, cache_);
    if (idx >= 0 && idx < jsonSize - pos)
      pos += idx;
    else
      setElementFailed();
  }
  if ((type._support & etSupportDoubleField) == 0 || (block_._typeInfo & etiDoubleField) == 0) return;
  // Let's go for one more round :)
  DataBlock block = block_;
  block._field = (EnumDataField)block._extra;
  block._value.t32[0] = block._value.t32[1];
  if (cache_._headElem._field == edf__none) startElement(block, cache_);
  if (!elementFailed) {
    int idx = blockToStr(buffer + pos, jsonSize - pos, block, cache_);
    if (idx >= 0 && idx < jsonSize - pos)
      pos += idx;
    else
      setElementFailed();
  }
}

// buffer_ is not allowed to be nullptr, buffer_[-1] must be valid
int JsonBuffer::blockToStr(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  const DataType& type_ = DataType::_list[block_._type];
  if (type_._coreType == ectNone) { // Just call it...
    type_._toStr(nullptr, 0, block_, cache_);
    return 0;
  }
  if (block_._field == edf__none) return 0;

  bool startElem = cache_._headElem._field == edf__none;
  bool complexElem = !startElem || (type_._coreType == ectArray || type_._coreType == ectObject);
  bool continueElem = block_._field == edf__cont;

  uint tlen = 0, clen = 0;
  char oc = openChars[type_._coreType];
  if (complexElem) {
    if (startElem) {
      cache_.setHeadElem(block_);
      if (tlen < len_) buffer_[tlen] = oc;
      tlen++;
    } else if (block_._field != edf__topic) {
      char prevChar = buffer_[-1]; // safe since we continue the previous block
      if (prevChar != '[' && prevChar != '{') {
        if (tlen < len_) buffer_[tlen] = ',';
        tlen++;
      }
    }
  }

  bool qm = type_._coreType == ectString || (type_._coreType == ectTopic && block_._field != edf__topic);
  if (continueElem) {
    if (buffer_[-1] == '\"') {
      clen = 1;
      if (len_ > 0) { // if we are not in measure mode, we can write one plus char
        buffer_--;
        len_++;
      }
    }
  } else if (qm) {
    if (tlen < len_) buffer_[tlen] = '\"';
    tlen++;
  }

  int len = type_._toStr(buffer_ + tlen, tlen < len_ ? len_ : 0, block_, cache_);
  if (len < 0) return len;
  tlen += len;
  if (qm) {
    if (tlen < len_) buffer_[tlen] = '\"';
    tlen++;
  }
  if (len == 0 && !qm && !startElem && tlen == 1) tlen = 0; // there was no data, remove the semicolon
  if (complexElem && block_._closeComplexFlag) {
    cache_._headElem._field = edf__none;
    if (tlen < len_) buffer_[tlen] = closeChars[type_._coreType];
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

const char JsonBuffer::openChars[] = {' ', ' ', ' ', '\"', '[', '{'};
const char JsonBuffer::closeChars[] = {' ', ' ', ' ', '\"', ']', '}'};

} // namespace stf

#endif
