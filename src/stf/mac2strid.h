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

class Mac {
public:
  static void toStrId(char* id, const uint8_t mac[8], uint macLen = 6);
  static void toMacId(char* id, const uint8_t mac[8], uint macLen = 6);
  static void integrityCheck();

protected:
  struct MacMap {
    const char* name;
    const uint8_t* mac4;
    uint8_t subIdLen;
  };

  static uint32_t hash4(const uint8_t* mac4);
  static void hash4Str(char* buffer, uint len, const uint8_t* mac4);

  static inline bool cmp4(const uint8_t* mac4, const uint8_t* mac6) {
    if (mac4[0] != mac6[0] || mac4[1] != mac6[1] || mac4[2] != mac6[2]) return false;
    if ((mac4[3] & 0x04) != 0 && (mac4[3] & 0xf0) != (mac6[3] & 0xf0)) return false;
    return true;
  }

  static const MacMap _vendorList[];

  static const uint8_t _macEsp[];
  static const uint8_t _macQing[];
  static const uint8_t _macTelink[];
};

} // namespace stf
