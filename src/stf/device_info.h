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

namespace stf {

struct DeviceInfo {
  const char* strId;
  const char* strMAC;
  uint8_t* mac;
  uint8_t macLen;

  inline bool cmpMAC(const DeviceInfo& di_) const {
    return macLen == di_.macLen && memcmp(mac, di_.mac, macLen) == 0;
  }
};

struct DeviceBlock {
  struct DeviceInfo info;

  char strIdBuffer[STF_STRID_SIZE];
  char strMACBuffer[STF_STRID_SIZE];
  uint8_t macBuffer[8];

  void reset();
  void create(const uint8_t* mac, uint macLen);
  void createMissing();
};

} // namespace stf
