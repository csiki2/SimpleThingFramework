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

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stf/settings.h>
#include <stf/device_info.h>

namespace stf {

uint32_t startingFreeHeap();

inline uint32_t uptimeMS32() { return millis(); }
inline uint64_t uptimeMS64() { return esp_timer_get_time() / 1000ULL; }

inline void mutexLock(xSemaphoreHandle handle) {
  for (; xSemaphoreTake(handle, portMAX_DELAY) != pdPASS;)
    ;
}
inline void mutexUnlock(xSemaphoreHandle handle) { xSemaphoreGive(handle); }

void logConnected(const char* name);
void logConnecting(const char* name, uint tryNum);
void logMemoryUsage(int level);
void os_setup();

extern const char* hostName;
extern const char* hostPassword;
extern uint8_t hostIP4[4];
extern const struct DeviceInfo* hostInfo;

} // namespace stf
