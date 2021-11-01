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

struct DataBlock;
struct DataField;
struct DataType;
struct DataCache;

// The features that a type supports from EnumTypeInfoGeneric (and don't use the bits for something else)
enum EnumTypeSupport {
  etSupportNone = 0,
  etSupportSaveToCache = 1,
  etSupportDoubleField = 2
};

enum EnumDataType {
#define E(e, enclose, support) edt_##e,
#include <stf/data_type.def>
#undef E
};

enum EnumExtraInfo {
  eeiNone = 0,

  // 3 bit
  eeiCacheBlock1 = 1,
  eeiCacheBlock2 = 2,
  eeiCacheDeviceMAC48 = 3,
  eeiCacheDeviceMAC64 = 4,
  eeiCacheDeviceHost = 5,
  eeiCacheDeviceMainHost = 6,
  eeiCacheMask = 7,
};

enum EnumTypeInfoSupport {
  etiDoubleField = 128, // so the extra field can be used
};

enum EnumTypeInfoTopic {
  etitSYS = 0,
  etitBT = 1,

  etitTopicSubjectMask = 7,

  etitStateSend = 0 << 3,
  etitConfig = 1 << 3,
  etitState = 2 << 3,
  etitCommand = 3 << 3,
  etitTopicTypeMask = 3 << 3,

};

enum EnumTypeInfoDevice {
  etidSource0Name = 1,
  etidSource0Model = 2,
  etidSource0Manufacturer = 3,
  etidSource0SWVersion = 4,
  etidSource0Mask = 7,

  etidSource1Name = 1 << 3,
  etidSource1Model = 2 << 3,
  etidSource1Manufacturer = 3 << 3,
  etidSource1SWVersion = 4 << 3,
  etidSource1Mask = 7 << 3,

  etidIdentifiersCached = 64,
  etidConnectionsCached = 128
};

enum EnumTypeInfoString {
  // 3 bit
  etisSource0Ptr = 0,
  etisSource0FmtPtr = 1,
  etisSource0DeviceId = 2,
  etisSource0MACId = 3,
  etisSource0CacheField = 4,
  etisSource0LocalField = 5,
  etisSource0Mask = 7,

  // 2 bit
  etisSource1Ptr = 0 << 3,
  etisSource1CacheField = 1 << 3,
  etisSource1LocalField = 2 << 3,
  etisSource1Mask = 3 << 3,

  // 2 bit
  etisCaseNothing = 0 << 5,
  etisCaseLower = 1 << 5,
  etisCaseUpper = 2 << 5,
  etisCaseSmart = 3 << 5,
  etisCaseMask = 3 << 5,
};

enum EnumTypeInfoRaw {
  etirSizeMask = 15,
  etirFormatNumber = 0 << 4,
  etirFormatHexUpper = 1 << 4,
  etirFormatHexLower = 2 << 4,
  etirFormatMask = 3 << 4,
  etirSeparatorNone = 0 << 6,
  etirSeparatorColon = 1 << 6,
  etirSeparatorDot = 2 << 6,
  etirSeparatorMask = 3 << 6,
};

enum EnumTypeInfoHex {
  etihSizeMask = 15,
  etihCaseUpper = 1 << 4,
  etihPrefix = 1 << 5
};

enum EnumCoreType {
  ectNone,
  ectTopic,
  ectNumber,
  ectString,
  ectArray,
  ectObject,
};

struct DataType {
  typedef int fnBlockToStr(char* buffer, uint len, const DataBlock& block, DataCache& cache);

  const fnBlockToStr* _toStr;
  EnumCoreType _coreType : 8;
  EnumTypeSupport _support;

  static const char* _topicNames[];
  static EnumTypeInfoTopic findTopicName(const char* str, uint strLen);

  static const DataType _list[];
  static const uint _listNum;

#define E(e, enclose, support) static int fnDT##e(char* buffer, uint len, const DataBlock& block, DataCache& cache);
#include <stf/data_type.def>
#undef E
};

}; // namespace stf
