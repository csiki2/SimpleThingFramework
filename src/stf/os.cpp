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

// Default value
DeviceBlock hostBlock;
const DeviceInfo* hostInfo = &hostBlock.info;
const char* hostName = STF_THING_NAME;
const char* hostPassword = STF_THING_PASSWORD;

uint32_t savedStartingFreeHeap = 0;

uint32_t startingFreeHeap() { return savedStartingFreeHeap; }

void logConnected(const char* name) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_INFO("Connected to the %s, memory used/free %6u/%6u\n", name, startingFreeHeap() - heap, heap);
}

void logConnecting(const char* name, uint tryNum) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_INFO("Connecting to the %s (try %u), memory used/free %6u/%6u\n", name, tryNum, startingFreeHeap() - heap, heap);
}

void logMemoryUsage(int level) {
  uint32_t heap = ESP.getFreeHeap();
  STFLOG_PRINT(level, "memory used/free %6u/%6u\n", startingFreeHeap() - heap, heap);
}

const char* hw = "Hello World!";
const char* hw2 = nullptr;

void os_setup() {
  Serial.begin(115200);
  stf::savedStartingFreeHeap = ESP.getFreeHeap();
  STFLOG_INFO(hw2 = "Starting ESP32... free memory %u (%u)\n", stf::savedStartingFreeHeap, ESP.getHeapSize());

  mac_to_strid_integrity_check();

  esp_efuse_mac_get_default(hostBlock.macBuffer);
  hostBlock.create(nullptr, 6);

  STFLOG_INFO("Host id: %s\n", hostInfo->strId);
}

} // namespace stf
