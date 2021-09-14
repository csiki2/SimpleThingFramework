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
#include <stf/data_field.h>
#include <stf/mac2strid.h>

#include <WiFi.h>

namespace stf {

// Host - default values
DeviceBlock Host::_block;
const char* Host::_name = STF_THING_NAME;
const char* Host::_password = STF_THING_PASSWORD;
uint8_t Host::_ip4[4] = {0, 0, 0, 0};
uint32_t Host::_startingFreeHeap = 0;

void Host::setup() {
  _startingFreeHeap = ESP.getFreeHeap();
  Serial.begin(115200);
  STFLOG_INFO("Starting ESP32... free memory %u (%u)\n", _startingFreeHeap, ESP.getHeapSize());

  mac_to_strid_integrity_check();

  esp_efuse_mac_get_default(_block.macBuffer);
  _block.create(nullptr, 6);

  STFLOG_INFO("Host id: %s\n", _info.strId);
}

void logConnected(const char* name) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_INFO("Connected to the %s, memory used/free %6u/%6u\n", name, Host::_startingFreeHeap - heap, heap);
}

void logConnecting(const char* name, uint tryNum) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_INFO("Connecting to the %s (try %u), memory used/free %6u/%6u\n", name, tryNum, Host::_startingFreeHeap - heap, heap);
}

void logMemoryUsage(int level) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_PRINT(level, "memory used/free %6u/%6u\n", Host::_startingFreeHeap - heap, heap);
}

} // namespace stf
