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
  etSupportCache = 1,
  etSupportDoubleField = 2
};

enum EnumDataType {
#define E(e, enclose, support) edt_##e,
#include <stf/data_type.def>
#undef E
};

enum EnumTypeInfoGeneric {
  etigNone = 0,

  // 3 bit
  etigCacheMask = 7 << 4,
  etigCacheBlock1 = 1 << 4,
  etigCacheBlock2 = 2 << 4,
  etigCacheDeviceMAC48 = 3 << 4,
  etigCacheDeviceMAC64 = 4 << 4,
  etigCacheDeviceHost = 5 << 4,
  etigDoubleField = 1 << 7,
};

enum EnumTypeInfoTopic {
  etitSYStoMQTT = 0, // For now, we might need to move it to _extra
  etitBTtoMQTT = 1,

  etitSubTopicMask = 7,
  etitConfig = 8,

  etigTopicMask = 15,
  etigHost = 128
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
  etisSource1TopicFilter = 3 << 3, // t32
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

typedef int fnBlockToStr(char* buffer_, uint len_, const DataBlock& block_, DataCache& cache_);

enum EnumCoreType {
  ectNone,
  ectTopic,
  ectNumber,
  ectString,
  ectArray,
  ectObject,
};

struct DataType {
  const fnBlockToStr* _toStr;
  EnumCoreType _coreType : 8;
  EnumTypeSupport _support;

  static const DataType list[];
  static const uint listNum;
};

}; // namespace stf
