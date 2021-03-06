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

#include <stf/os.h>
#include <stf/mac2strid.h>

namespace stf {

void DeviceBlock::reset() {
  strIdBuffer[0] = strMACBuffer[0] = 0;
  info.macLen = 0;
  info.mac = macBuffer;
  info.strMAC = strMACBuffer;
  info.strId = strIdBuffer;
}

void DeviceBlock::create(const uint8_t* mac, uint macLen) {
  if (mac != nullptr) memcpy(macBuffer, mac, macLen);
  info.macLen = macLen;
  Mac::toMacId(strMACBuffer, macBuffer, info.macLen);
  Mac::toStrId(strIdBuffer, macBuffer, info.macLen);

  info.mac = macBuffer;
  info.strMAC = strMACBuffer;
  info.strId = strIdBuffer;
}

void DeviceBlock::createMissing() {
  if (info.strMAC == nullptr) {
    info.strMAC = strMACBuffer;
    strMACBuffer[0] = 0;
    if (info.macLen != 0) Mac::toMacId(strMACBuffer, info.mac, info.macLen);
  }
  if (info.strId == nullptr) {
    info.strId = strIdBuffer;
    strIdBuffer[0] = 0;
    if (info.macLen != 0) Mac::toStrId(strIdBuffer, info.mac, info.macLen);
  }
}

} // namespace stf
