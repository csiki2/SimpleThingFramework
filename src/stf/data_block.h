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
#include <stf/data_field.h>
#include <stf/data_type.h>

namespace stf {

// the size of a block should be 12 bytes
struct __attribute__((packed)) DataBlock {
  inline void reset() {
    (*(uint32_t*)this) = 0;
    _value.t32[0] = 0;
    _value.t32[1] = 0;
  }

  inline void closeMessage() { _closeMessage = 1; }
  inline bool isClosedMessage() const { return _closeMessage != 0; }

  inline DataBlock& setMAC48(const uint8_t* mac_) {
    for (int idx = 0; idx < 6; idx++) _value.t8[idx] = mac_[idx];
    _value.t16[3] = 0;
    return *this;
  }
  inline DataBlock& setMAC64(const uint8_t* mac_) {
    for (int idx = 0; idx < 8; idx++) _value.t8[idx] = mac_[idx];
    return *this;
  }

  inline DataBlock& setPtr(const void* ptr0_) {
    _value.tPtr[0] = (void*)ptr0_;
    return *this;
  }
  inline DataBlock& setPtr(const void* ptr0_, const void* ptr1_) {
    _value.tPtr[0] = (void*)ptr0_;
    _value.tPtr[1] = (void*)ptr1_;
    return *this;
  }

  inline DataBlock& set64(const uint64_t num_) {
    _value.t64[0] = num_;
    return *this;
  }

  inline DataBlock& set32(const uint32_t num0_) {
    _value.t32[0] = num0_;
    return *this;
  }
  inline DataBlock& set32(const uint32_t num0_, const uint32_t num1_) {
    _value.t32[0] = num0_;
    _value.t32[1] = num1_;
    return *this;
  }

  inline DataBlock& setFloat(const float num0_) {
    _value.tFloat[0] = num0_;
    return *this;
  }
  inline DataBlock& setFloat(const float num0_, const float num1_) {
    _value.tFloat[0] = num0_;
    _value.tFloat[1] = num1_;
    return *this;
  }

  inline DataBlock& setRaw(const uint8_t* raw_, uint len_) {
    memcpy(_type == edt_Raw ? &_extra : _value.t8, raw_, len_);
    return *this;
  }

  EnumDataType _type : 6;
  unsigned _closeComplexFlag : 1;
  unsigned _closeMessage : 1;

  uint8_t _typeInfo;
  EnumDataField _field : 8;
  uint8_t _extra;
  union {
    uint8_t t8[8];
    uint16_t t16[4];
    uint32_t t32[2];
    uint64_t t64[1];
    float tFloat[2];
    double tDouble[1];
    void* tPtr[2];
  } _value;
};

static_assert(sizeof(DataBlock) == 12, "DataBlock size should be 12");

} // namespace stf
