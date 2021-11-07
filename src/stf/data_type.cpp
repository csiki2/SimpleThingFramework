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

const char* DataType::_topicNames[] = {
    "SYS", // etitSYS
    "BT", // etitBT
};

EnumTypeInfoTopic DataType::findTopicName(const char* str, uint strLen) {
  return (EnumTypeInfoTopic)Helper::getArrayIndex(str, strLen, _topicNames, sizeof(_topicNames) / sizeof(_topicNames[0]));
}

int DataType::fnDTNone(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  return -1;
}

// Never should be called
int DataType::fnDTGenerator(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  return -1;
}

int DataType::fnDTTopic(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  uint topicIndex = block._typeInfo & etitTopicSubjectMask;
  int resLen = -1;
  switch (block._typeInfo & etitTopicTypeMask) {
    case etitStateSend:
      resLen = snprintf(buffer, len, "home/%s/%stoMQTT/%s", Host::_name, _topicNames[topicIndex], cache._device.info.strId);
      break;
    case etitConfig:
      resLen = snprintf(buffer, len, "homeassistant/%s/%s_%s/config", Discovery::_topicConfigComponent[topicIndex], cache._device.info.strMAC, DataField::_list[cache._block_device._field]);
      break;
    case etitState:
      resLen = snprintf(buffer, len, "+/+/%stoMQTT/%s", _topicNames[topicIndex], cache._device.info.strId);
      break;
    case etitCommand:
      resLen = snprintf(buffer, len, "home/%s/MQTTto%s/%s/command/%s_%s", Host::_name, _topicNames[topicIndex], cache._device.info.strId, cache._device.info.strMAC, DataField::_list[(uint)cache._block_device._field]);
      break;
    default:
      break;
  }
  return resLen;
}

int DataType::fnDTDevice(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  int resLen = 0;
  uint typeInfo = block._typeInfo;
  if (typeInfo & etidIdentifiersCached) {
    const char *id1 = cache._device.info.strMAC, *id2 = cache._device.info.strId;
    if (id2 != nullptr && (strlen(id2) == 0 || (id1 != nullptr && strcmp(id1, id2) == 0))) id2 = nullptr;
    if (id1 == nullptr || strlen(id1) == 0) {
      id1 = id2;
      id2 = nullptr;
    }
    if (id1 != nullptr) {
      int res = snprintf(buffer + resLen, len - resLen, "%s\"identifiers\":[\"%s\"%c", resLen != 0 ? "," : "", id1, id2 != nullptr ? ',' : ']');
      if (res < 0) return res;
      resLen += res;
      if (id2 != nullptr) {
        res = snprintf(buffer + resLen, len - resLen, "\"%s\"]", id2);
        if (res < 0) return res;
        resLen += res;
      }
    }
  }
  if (typeInfo & etidConnectionsCached) {
    const char* mac = cache._device.info.strMAC;
    if (mac != nullptr && strlen(mac) != 0) {
      int res = snprintf(buffer + resLen, len - resLen, "%s\"connections\":[[\"mac\",\"%s\"]]", resLen != 0 ? "," : "", mac);
      if (res < 0) return res;
      resLen += res;
    }
  }
  if ((typeInfo & etidIdentifiersCached) != 0 && !cache._device.info.cmpMAC(Host::_info)) {
    int res = snprintf(buffer + resLen, len - resLen, "%s\"via_device\":\"%s\"", resLen != 0 ? "," : "", Host::_info.strId);
    if (res < 0) return res;
    resLen += res;
  }
  static const char* fields[] = {nullptr, "name", "model", "manufacturer", "sw_version"};
  for (int idx = 0; idx < 2; idx++) {
    const char* value = (const char*)block._value.tPtr[idx];
    if (value != nullptr) {
      uint tidx = typeInfo & etidSource0Mask;
      if (tidx > 0 && tidx < sizeof(fields) / sizeof(fields[0])) {
        int vlen = strlen(value);
        bool addId = vlen > 0 && strchr(" _-", value[vlen - 1]) != nullptr;
        int res = snprintf(buffer + resLen, len - resLen, "%s\"%s\":\"%s%s\"", resLen != 0 ? "," : "", fields[tidx], value, addId ? cache._device.info.strId : "");
        if (res < 0) return res;
        resLen += res;
      }
    }
    typeInfo /= etidSource0Mask + 1;
  }

  return resLen;
}

int DataType::fnDTString(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  const char *str0, *str1;
  switch (block._typeInfo & etisSource0Mask) {
    case etisSource0Ptr:
    case etisSource0FmtPtr: // "{{ value_json.%s }}"
      str0 = (const char*)block._value.tPtr[0];
      break;
    case etisSource0DeviceId:
      str0 = cache._device.info.strId;
      break;
    case etisSource0MACId:
      str0 = cache._device.info.strMAC;
      break;
    case etisSource0CacheField:
      str0 = DataField::_list[(uint)cache._block_device._field];
      break;
    case etisSource0LocalField:
      str0 = DataField::_list[(uint)block._field];
      break;
    default:
      str0 = "";
      break;
  }
  switch (block._typeInfo & etisSource1Mask) {
    case etisSource1Ptr:
      str1 = (const char*)block._value.tPtr[1];
      break;
    case etisSource1CacheField:
      str1 = DataField::_list[(uint)cache._block_device._field];
      break;
    case etisSource1LocalField:
      str1 = DataField::_list[(uint)block._field];
      break;
    default:
      str1 = nullptr;
      break;
  }
  int resLen = -1;
  if ((block._typeInfo & etisSource0Mask) == etisSource0FmtPtr)
    resLen = snprintf(buffer, len, str0 != nullptr ? str0 : "%s", str1 != nullptr ? str1 : "");
  else
    resLen = snprintf(buffer, len, str1 == nullptr ? "%s" : "%s_%s", str0 != nullptr ? str0 : "", str1);

  uint caseMask;
  if (resLen > 0 && resLen < len && (caseMask = block._typeInfo & etisCaseMask) != etisCaseNothing) {
    switch (caseMask) {
      case etisCaseLower:
        for (int idx = 0; idx < resLen; idx++) buffer[idx] = tolower(buffer[idx]);
        break;
      case etisCaseUpper:
        for (int idx = 0; idx < resLen; idx++) buffer[idx] = toupper(buffer[idx]);
        break;
      case etisCaseSmart:
        for (int idx = 0; idx < resLen; idx++) {
          char cc = buffer[idx], pc = (idx == 0 ? ' ' : buffer[idx - 1]);
          buffer[idx] = (cc == '_' ? ' ' : (pc == ' ' ? toupper(cc) : tolower(cc)));
        }
      default:
        break;
    }
  }

  return resLen;
}

int DataType::fnDTRaw(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  uint8_t size = block._typeInfo & etirSizeMask;
  char fmt[6];
  int fmtType = block._typeInfo & etirFormatMask;
  strcpy(fmt, fmtType == etirFormatHexUpper ? "%02X" : (fmtType == etirFormatHexLower ? "%02x" : "%u"));
  int fmtSeparator = block._typeInfo & etirSeparatorMask;
  strcpy(fmt + strlen(fmt), fmtSeparator == etirSeparatorColon ? ":" : (fmtSeparator == etirSeparatorDot ? "." : ""));

  int resLen = 0;
  const uint8_t* buff = &block._extra;
  for (int idx = 0; idx < size; idx++) {
    if (fmtSeparator != etirSeparatorNone && idx + 1 == size) fmt[strlen(fmt) - 1] = '\0';
    int res = snprintf(buffer + resLen, len - resLen, fmt, buff[idx]);
    if (res < 0) return res;
    resLen += res;
  }
  return resLen;
}

int DataType::fnDT32(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  uint32_t value = block._value.t32[0];
  const char* fmt = (block._typeInfo & 1) == 0 ? "%lu" : "%ld";
  int resLen = snprintf(buffer, len, fmt, value);
  return resLen;
}

int DataType::fnDTHex32(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  uint32_t value = block._value.t32[0];
  uint size = block._typeInfo & etihSizeMask;
  char hex = (block._typeInfo & etihCaseUpper) != 0 ? 'X' : 'x';
  char prefix[3] = {(block._typeInfo & etihPrefix) != 0 ? '0' : '\0', hex, 0};
  char fmt[10];
  snprintf(fmt, sizeof(fmt), "%s%%%s%d%c", prefix, size != 0 ? "0" : "", size, hex);
  int resLen = snprintf(buffer, len, fmt, value);
  return resLen;
}

int DataType::fnDT64(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  uint64_t value = block._value.t64[0];
  const char* fmt = (block._typeInfo & 1) == 0 ? "%llu" : "%lld";
  int resLen = snprintf(buffer, len, fmt, value);
  return resLen;
}

int DataType::fnDTFloat(char* buffer, uint len, const DataBlock& block, DataCache& cache) {
  float value = block._value.tFloat[0];
  char fmt[5] = "%._f";
  fmt[2] = '0' + (block._typeInfo & 7);
  int resLen = snprintf(buffer, len, fmt, value);
  return resLen;
}

const DataType DataType::_list[] = {
#define E(e, enclose, support) {DataType::fnDT##e, enclose, support},
#include <stf/data_type.def>
#undef E
};

}; // namespace stf
