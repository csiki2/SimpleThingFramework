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

#if STFTASK_LED == 1

#  include <stf/task.h>
#  include <stf/provider_system.h>

#  include <driver/ledc.h>
#  include <mutex>

namespace stf {

STFLED_SAMPLE(0, 128)
STFLED_SAMPLE(010, 128, 191, 128)
STFLED_SAMPLE(01010, 128, 191, 128, 128, 191, 128)
STFLED_SAMPLE(Smooth, 3, 67, 63, 127, 3)

struct LedChannel {
  LedSample* sample;
  uint32_t startTime;
  uint32_t loopTime;
  uint16_t sampleElemTime;
  uint8_t loopCount;
  uint8_t brightness;
};

struct LedEvent {
  inline void Set(uint8_t iLed = 0, uint8_t iChn = 0, uint8_t iBrightness = 255, LedSample* iSample = nullptr, uint16_t iSampleElemTime = 50,
                  uint8_t iLoopCount = 1, uint32_t iLoopTime = 1000, int32_t iUptimeSync = -1) {
    led = iLed;
    chn = iChn;
    brightness = iBrightness;
    sample = iSample;
    sampleElemTime = iSampleElemTime;
    loopCount = iLoopCount;
    loopTime = iLoopTime;
    uptimeSync = iUptimeSync;
  }

  LedSample* sample = nullptr;
  int8_t led = 0;
  int8_t chn = 0;
  uint8_t brightness = 255;
  uint8_t loopCount = 1;
  uint32_t loopTime = 1000;
  int32_t uptimeSync = -1;
  uint16_t sampleElemTime = 50;
};

class LedObject {
public:
  LedObject() {
    value = 0;
    for (int idx = 0; idx < STFLED_CHANNELS; idx++)
      channels[idx].startTime = ~0;
  }
  LedChannel channels[STFLED_CHANNELS];
  Mutex mutex;
  int value;
};

template <>
const TaskDescriptor Task<LedTask>::_descriptor = {&_obj, "LedTask", 2048, 0, 1};
template class Task<LedTask>;

LedEvent LedTask::ledEvents[STFLED_MAXEVENTS];
uint8_t LedTask::ledPins[] = {STFLED_PINS};
uint8_t LedTask::ledPWM[] = {STFLED_PWMS};
#  define STFLED_NUM sizeof(LedTask::ledPins)

LedObject LedTask::leds[STFLED_NUM];

void LedTask::ledRegisterEvent(int event, int8_t led, int8_t chn, uint8_t brightness, LedSample* sample, uint16_t sampleElemTime, uint8_t loopCount, uint32_t loopTime, int32_t uptimeSync) {
  if (event >= 0 && event < STFLED_MAXEVENTS) ledEvents[event].Set(led, chn, brightness, sample, sampleElemTime, loopCount, loopTime, uptimeSync);
}

void LedTask::ledPlayEvent(int event) {
  if (event >= 0) {
    if (event < STFLED_MAXEVENTS) {
      LedEvent& obj = ledEvents[event];
      ledPlayDirect(obj.led, obj.chn, obj.brightness, obj.sample, obj.sampleElemTime, obj.loopCount, obj.loopTime, obj.uptimeSync);
    } else {
      ledPlayDirect(0, 0, 255, nullptr, 50, 1, 100, -1);
    }
  }
}

void LedTask::registerDefaultLedEvents() {
  // Default channel usage for LED 0:
  // 0 - watchdog
  // 1 - default
  // 2 - wifi
  // 3 - mqtt
  // last - setup

  ledRegisterEvent(STFLEDEVENT_DEFAULT, 0, 1, 255, &g_ledSample01010, 50, 1, 2000, -1);
  ledRegisterEvent(STFLEDEVENT_WATCHDOG, 0, 0, 128, &g_ledSample01010, 25, 1, 2000, 0);
  ledRegisterEvent(STFLEDEVENT_SETUP, 0, STFLED_CHANNELS - 1, 192, &g_ledSampleSmooth, 500, 1, 2000, 950);

  ledRegisterEvent(STFLEDEVENT_WIFI_CONNECTED, 0, -1, 128, &g_ledSample010, 25, 1, 2000, 1000); // Disabled
  ledRegisterEvent(STFLEDEVENT_WIFI_NOT_CONNECTED, 0, 2, 128, &g_ledSample010, 25, 1, 2000, 1000);

  ledRegisterEvent(STFLEDEVENT_MQTT_CONNECTED, 0, -1, 128, &g_ledSample010, 25, 1, 2000, 1100); // Disabled
  ledRegisterEvent(STFLEDEVENT_MQTT_NOT_CONNECTED, 0, 3, 128, &g_ledSample010, 25, 1, 2000, 1100);

  ledRegisterEvent(STFLEDEVENT_BLE_RECEIVE, 0, 1, 128, nullptr, 50, 1, 2000, -1);
}

void __attribute__((weak)) LedTask::registerLedEvents() {
  registerDefaultLedEvents();
}

void LedTask::ledPlayDirect(int8_t led, int8_t chn, uint8_t brightness, LedSample* sample, uint16_t sampleElemTime, uint8_t loopCount, uint32_t loopTime, int32_t uptimeSync) {
  if (led < 0 || chn < 0) return; // valid case: disabled event
  if (led >= STFLED_NUM) led = 0;
  if (chn >= STFLED_CHANNELS) chn = STFLED_CHANNELS - 1;
  LedChannel& channel = leds[led].channels[chn];
  uint32_t timeMS;
  if (uptimeSync != (int32_t)-1) {
    if (uptimeSync < 0) uptimeSync = (uptimeSync % loopTime) + loopTime;
    uint64_t uptime64 = Host::uptimeMS64();
    uint32_t uptime32 = (uint32_t)uptime64;
    uint32_t diff = (uint32_t)(uptime64 % (uint64_t)loopTime);
    timeMS = uptime32 - diff + uptimeSync;
    if (diff < uptimeSync) timeMS -= loopTime;
    if (timeMS != uptime32 && loopCount > 0 && loopCount < 255) loopCount++;
  } else {
    timeMS = Host::uptimeMS32();
  }
  {
    std::lock_guard<Mutex> lock(leds[led].mutex);
    channel.loopTime = loopTime;
    channel.sampleElemTime = sampleElemTime;
    channel.sample = sample;
    channel.brightness = brightness;
    channel.loopCount = loopCount;
    channel.startTime = timeMS;
  }
}

void LedTask::setLed(int idx, int value) {
  if (!SystemProvider::isLedEnabled()) value = 0;
  if (ledPWM[idx] > 15)
    digitalWrite(ledPins[idx], value < 128 ? LOW : HIGH);
  else
    ledcWrite(ledPWM[idx], value > 0 ? (value <= 255 ? value : 255) : 0);
}

void LedTask::setup() {
  for (int idx = 0; idx < STFLED_NUM; idx++) {
    if (ledPWM[idx] > 15) {
      pinMode(ledPins[idx], OUTPUT);
    } else {
      ledcSetup(ledPWM[idx], 8000, 8);
      ledcAttachPin(ledPins[idx], ledPWM[idx]);
    }
    setLed(idx, 0);
  }
  registerLedEvents();
}

uint LedTask::loop() {
  uint32_t timeMS = Host::uptimeMS32();
  for (int led = 0; led < STFLED_NUM; led++) {
    int ledValue = 0;
    std::lock_guard<Mutex> lock(leds[led].mutex);
    for (int channel = 0; channel < STFLED_CHANNELS; channel++) {
      LedChannel& channelObj = leds[led].channels[channel];
      if (channelObj.startTime == ~0) continue;
      uint32_t st = channelObj.startTime;
      uint32_t diff = timeMS - st;
      if (diff >= channelObj.loopTime) { // one or more loop is elapsed
        uint32_t loopsDone = diff / channelObj.loopTime;
        if (channelObj.loopCount != 0) {
          if (channelObj.loopCount <= loopsDone) { // the channel is finished, next...
            channelObj.startTime = ~0;
            continue;
          }
          channelObj.loopCount -= loopsDone;
        }
        uint32_t loopsTime = loopsDone * channelObj.loopTime;
        channelObj.startTime += loopsTime;
        diff -= loopsTime;
      }
      uint32_t sampleElemTime = channelObj.sampleElemTime;
      uint32_t sampleIndex = diff / sampleElemTime;
      if (channelObj.sample == nullptr) {
        int channelValue = sampleIndex > 0 ? 0 : channelObj.brightness;
        if (channelValue > ledValue) ledValue = channelValue;
      } else if (sampleIndex < channelObj.sample->size) {
        uint8_t sample = channelObj.sample->data[sampleIndex];
        int channelValue = ((sample & STFLEDSAMPLE_MASK) * 255 + STFLEDSAMPLE_MASK / 2) / STFLEDSAMPLE_MASK;
        if ((sample & STFLEDSAMPLE_INTERPOLATION) != 0 && sampleIndex + 1 < channelObj.sample->size) { // Interpolation !!!
          int value2 = ((channelObj.sample->data[sampleIndex + 1] & STFLEDSAMPLE_MASK) * 255 + STFLEDSAMPLE_MASK / 2) / STFLEDSAMPLE_MASK;
          int frac = diff - sampleIndex * sampleElemTime;
          channelValue = ((sampleElemTime - frac) * channelValue + frac * value2 + sampleElemTime / 2) / sampleElemTime;
        }
        channelValue = (channelValue * channelObj.brightness + 128) / 255;
        if (channelValue > ledValue) ledValue = channelValue;
        if ((sample & STFLEDSAMPLE_INTERRUPT) != 0) break;
      } else if (channelObj.loopCount == 1) {
        channelObj.startTime = ~0;
      }
    }
    if (leds[led].value != ledValue)
      setLed(led, leds[led].value = ledValue);
  }
  uint32_t ellapsed = Host::uptimeMS32() - timeMS;
  return ellapsed < 10 ? 10 - ellapsed : 1;
}

} // namespace stf

#endif
