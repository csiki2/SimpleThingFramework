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
#include <stf/data_buffer.h>
#include <stf/data_cache.h>
#include <stf/data_type.h>
#include <stf/mac2strid.h>
#include <stf/data_discovery.h>
#include <stf/util.h>

namespace stf {

const char* topicNormalFmt[] = {
    "SYS", // etitSYS
    "BT", // etitBT
};

EnumTypeInfoTopic findTopicInfo(const char* str_, uint strLen_) {
  return (EnumTypeInfoTopic)getArrayIndex(str_, strLen_, topicNormalFmt, sizeof(topicNormalFmt) / sizeof(topicNormalFmt[0]));
}

int fnDTNone(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  return -1;
}

int fnDTCache(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  return -1;
}

// Never should be called
int fnDTGenerator(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  return -1;
}

int fnDTTopic(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  uint topicIndex = block_._typeInfo & etitTopicSubjectMask;
  int len = -1;
  switch (block_._typeInfo & etitTopicTypeMask) {
    case etitStateSend:
      len = snprintf(buffer_, len_, "home/%s/%stoMQTT/%s", hostName, topicNormalFmt[topicIndex], cache_._device.info.strId);
      break;
    case etitConfig:
      len = snprintf(buffer_, len_, "homeassistant/%s/%s_%s/config", Discovery::topicConfigComponent[topicIndex], cache_._device.info.strMAC, DataField::list[cache_._block_device._field]);
      break;
    case etitState:
      len = snprintf(buffer_, len_, "+/+/%stoMQTT/%s", topicNormalFmt[topicIndex], cache_._device.info.strId);
      break;
    case etitCommand:
      len = snprintf(buffer_, len_, "home/%s/MQTTto%s/%s/command/%s_%s", hostName, topicNormalFmt[topicIndex], cache_._device.info.strId, cache_._device.info.strMAC, DataField::list[(uint)cache_._block_device._field]);
      break;
    default:
      break;
  }
  return len;
}

int fnDTDevice(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  int len = 0;
  uint typeInfo = block_._typeInfo;
  if (typeInfo & etidIdentifiersCached) {
    const char *id1 = cache_._device.info.strMAC, *id2 = cache_._device.info.strId;
    if (id2 != nullptr && (strlen(id2) == 0 || (id1 != nullptr && strcmp(id1, id2) == 0))) id2 = nullptr;
    if (id1 == nullptr || strlen(id1) == 0) {
      id1 = id2;
      id2 = nullptr;
    }
    if (id1 != nullptr) {
      int res = snprintf(buffer_ + len, len_ - len, "%s\"identifiers\":[\"%s\"%c", len != 0 ? "," : "", id1, id2 != nullptr ? ',' : ']');
      if (res < 0) return res;
      len += res;
      if (id2 != nullptr) {
        res = snprintf(buffer_ + len, len_ - len, "\"%s\"]", id2);
        if (res < 0) return res;
        len += res;
      }
    }
  }
  if (typeInfo & etidConnectionsCached) {
    const char* mac = cache_._device.info.strMAC;
    if (mac != nullptr && strlen(mac) != 0) {
      int res = snprintf(buffer_ + len, len_ - len, "%s\"connections\":[[\"mac\",\"%s\"]]", len != 0 ? "," : "", mac);
      if (res < 0) return res;
      len += res;
    }
  }
  if (!cache_._device.info.cmpMAC(*hostInfo)) {
    int res = snprintf(buffer_ + len, len_ - len, "%s\"via_device\":\"%s\"", len != 0 ? "," : "", hostInfo->strId);
    if (res < 0) return res;
    len += res;
  }
  static const char* fields[] = {nullptr, "name", "model", "manufacturer", "sw_version"};
  for (int idx = 0; idx < 2; idx++) {
    const char* value = (const char*)block_._value.tPtr[idx];
    if (value != nullptr) {
      uint tidx = typeInfo & etidSource0Mask;
      if (tidx > 0 && tidx < sizeof(fields) / sizeof(fields[0])) {
        int vlen = strlen(value);
        bool addId = vlen > 0 && strchr(" _-", value[vlen - 1]) != nullptr;
        int res = snprintf(buffer_ + len, len_ - len, "%s\"%s\":\"%s%s\"", len != 0 ? "," : "", fields[tidx], value, addId ? cache_._device.info.strId : "");
        if (res < 0) return res;
        len += res;
      }
    }
    typeInfo /= etidSource0Mask + 1;
  }

  return len;
}

int fnDTString(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  const char *str0, *str1;
  switch (block_._typeInfo & etisSource0Mask) {
    case etisSource0Ptr:
    case etisSource0FmtPtr: // "{{ value_json.%s }}"
      str0 = (const char*)block_._value.tPtr[0];
      break;
    case etisSource0DeviceId:
      str0 = cache_._device.info.strId;
      break;
    case etisSource0MACId:
      str0 = cache_._device.info.strMAC;
      break;
    case etisSource0CacheField:
      str0 = DataField::list[(uint)cache_._block_device._field];
      break;
    case etisSource0LocalField:
      str0 = DataField::list[(uint)block_._field];
      break;
    default:
      str0 = "";
      break;
  }
  switch (block_._typeInfo & etisSource1Mask) {
    case etisSource1Ptr:
      str1 = (const char*)block_._value.tPtr[1];
      break;
    case etisSource1CacheField:
      str1 = DataField::list[(uint)cache_._block_device._field];
      break;
    case etisSource1LocalField:
      str1 = DataField::list[(uint)block_._field];
      break;
    default:
      str1 = nullptr;
      break;
  }
  int len = -1;
  if ((block_._typeInfo & etisSource0Mask) == etisSource0FmtPtr)
    len = snprintf(buffer_, len_, str0 != nullptr ? str0 : "%s", str1 != nullptr ? str1 : "");
  else
    len = snprintf(buffer_, len_, str1 == nullptr ? "%s" : "%s_%s", str0 != nullptr ? str0 : "", str1);

  uint caseMask;
  if (len > 0 && len < len_ && (caseMask = block_._typeInfo & etisCaseMask) != etisCaseNothing) {
    switch (caseMask) {
      case etisCaseLower:
        for (int idx = 0; idx < len; idx++) buffer_[idx] = tolower(buffer_[idx]);
        break;
      case etisCaseUpper:
        for (int idx = 0; idx < len; idx++) buffer_[idx] = toupper(buffer_[idx]);
        break;
      case etisCaseSmart:
        for (int idx = 0; idx < len; idx++) {
          char cc = buffer_[idx], pc = (idx == 0 ? ' ' : buffer_[idx - 1]);
          buffer_[idx] = (cc == '_' ? ' ' : (pc == ' ' ? toupper(cc) : tolower(cc)));
        }
      default:
        break;
    }
  }

  return len;
}

int fnDTRaw(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_) {
  uint8_t size = block_._typeInfo & etirSizeMask;
  char fmt[6];
  int fmtType = block_._typeInfo & etirFormatMask;
  strcpy(fmt, fmtType == etirFormatHexUpper ? "%02X" : (fmtType == etirFormatHexLower ? "%02x" : "%u"));
  int fmtSeparator = block_._typeInfo & etirSeparatorMask;
  strcpy(fmt + strlen(fmt), fmtSeparator == etirSeparatorColon ? ":" : (fmtSeparator == etirSeparatorDot ? "." : ""));

  int len = 0;
  const uint8_t* buff = &block_._extra;
  for (int idx = 0; idx < size; idx++) {
    if (fmtSeparator != etirSeparatorNone && idx + 1 == size) fmt[strlen(fmt) - 1] = '\0';
    int res = snprintf(buffer_ + len, len_ - len, fmt, buff[idx]);
    if (res < 0) return res;
    len += res;
  }
  return len;
}

int fnDT32(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache) {
  uint32_t value = block_._value.t32[0];
  const char* fmt = (block_._typeInfo & 1) == 0 ? "%lu" : "%ld";
  int len = snprintf(buffer_, len_, fmt, value);
  return len;
}

int fnDTHex32(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache) {
  uint32_t value = block_._value.t32[0];
  uint size = block_._typeInfo & etihSizeMask;
  char hex = (block_._typeInfo & etihCaseUpper) != 0 ? 'X' : 'x';
  char prefix[3] = {(block_._typeInfo & etihPrefix) != 0 ? '0' : '\0', hex, 0};
  char fmt[10];
  snprintf(fmt, sizeof(fmt), "%s%%%s%d%c", prefix, size != 0 ? "0" : "", size, hex);
  int len = snprintf(buffer_, len_, fmt, value);
  return len;
}

int fnDT64(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache) {
  uint64_t value = block_._value.t64[0];
  const char* fmt = (block_._typeInfo & 1) == 0 ? "%llu" : "%lld";
  int len = snprintf(buffer_, len_, fmt, value);
  return len;
}

int fnDTFloat(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache) {
  float value = block_._value.tFloat[0];
  char fmt[5] = "%._f";
  fmt[2] = '0' + (block_._typeInfo & 7);
  int len = snprintf(buffer_, len_, fmt, value);
  return len;
}

const DataType DataType::list[] = {
#define E(e, enclose, support) {fnDT##e, enclose, support},
#include <stf/data_type.def>
#undef E
};

}; // namespace stf
