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

inline void mutexLock(xSemaphoreHandle handle) {
  for (; xSemaphoreTake(handle, portMAX_DELAY) != pdPASS;)
    ;
}
inline void mutexUnlock(xSemaphoreHandle handle) { xSemaphoreGive(handle); }

void logConnected(const char* name);
void logConnecting(const char* name, uint tryNum);
void logMemoryUsage(int level);

class Host {
private:
  static DeviceBlock _block;

public:
  static void setup();

  static inline uint32_t uptimeMS32() { return millis(); }
  static inline uint64_t uptimeMS64() { return esp_timer_get_time() / 1000ULL; }
  static inline uint32_t uptimeSec32() { return (uint32_t)(esp_timer_get_time() / 1000000ULL); }

  static constexpr DeviceInfo& _info = _block.info;

  static const char* _name;
  static const char* _password;
  static uint8_t _ip4[4];

  static uint32_t _startingFreeHeap;
};

} // namespace stf
