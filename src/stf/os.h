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

#define STFATTR_PACKED         __attribute__((packed))
#define STFATTR_INIT(priority) __attribute__((init_priority(priority)))

namespace stf {

class Mutex {
public:
  inline Mutex() { _handle = xSemaphoreCreateMutex(); }
  inline ~Mutex() { vSemaphoreDelete(_handle); }

  inline void lock() {
    for (; xSemaphoreTake(_handle, portMAX_DELAY) != pdPASS;)
      ;
  }
  inline void unlock() { xSemaphoreGive(_handle); }

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

protected:
  xSemaphoreHandle _handle;
};

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

/**
 * @brief Gives back the elapsed time since reset() in ms, but maximum MaxElapsed (currently ~23 days).
 * 
 * Any of the called methods (except reset()) might set the internal _time to 0 to not overflow and will give back only MaxElapsed from that point.
 * Any of the method should be called at least every 2^32-MaxElapsed time (currently ~26 days...) so it won't fail to detect the overrun.
 */
class ElapsedTime {
public:
  static constexpr uint32_t MaxElapsed = 2000000000;

  uint32_t elapsedTime() const;
  void reset();

  bool operator==(const ElapsedTime& t) const;
  inline bool operator!=(const ElapsedTime& t) const { return !operator==(t); }
  ElapsedTime& operator=(const ElapsedTime& t);

protected:
  mutable uint32_t _time = 0;
};

} // namespace stf
