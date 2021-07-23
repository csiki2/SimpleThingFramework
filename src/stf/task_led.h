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

#ifndef STFLED_PINS
#  define STFLED_PINS LED_BUILTIN
#endif

#ifndef STFLED_PWMS
#  define LED_PWMS 0
#endif

#ifndef STFLED_CHANNELS
#  define STFLED_CHANNELS 8
#endif

#ifndef STFLED_MAXEVENTS
#  define STFLED_MAXEVENTS 16
#endif

#ifndef STFLED_COMMAND
#  define STFLED_COMMAND(event) stf::ledPlayEvent(event)
#endif

namespace stf {
struct LedSample {
  inline LedSample(uint8_t* data_, uint32_t size_) : data(data_), size(size_) {}
  uint8_t* data;
  uint32_t size;
};

#define STFLEDSAMPLE_MASK          63
#define STFLEDSAMPLE_INTERPOLATION 64
#define STFLEDSAMPLE_INTERRUPT     128
#define STFLED_SAMPLE(name, data...)      \
  uint8_t ledSampleData##name[] = {data}; \
  LedSample ledSample##name(ledSampleData##name, sizeof(ledSampleData##name));

extern LedSample ledSample0;
extern LedSample ledSample010;
extern LedSample ledSample01010;
extern LedSample ledSampleSmooth;

void ledPlayEvent(int event);
void ledPlayDirect(int8_t led, int8_t chn, uint8_t brightness, LedSample* sample, uint16_t sampleElemTime, uint8_t loopCount, uint32_t loopTime, int32_t uptimeSync);
void ledRegisterEvent(int event, int8_t led, int8_t chn, uint8_t brightness, LedSample* sample, uint16_t sampleElemTime, uint8_t loopCount, uint32_t loopTime, int32_t uptimeSync);

void registerLedEvents();
void registerDefaultLedEvents();

} // namespace stf
