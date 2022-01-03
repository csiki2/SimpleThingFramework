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

STFLED_SAMPLE(1, 63)
STFLED_SAMPLE(0, 128)
STFLED_SAMPLE(010, 128, 191, 128)
STFLED_SAMPLE(01010, 128, 191, 128, 128, 191, 128)
STFLED_SAMPLE(Smooth, 3, 67, 63, 127, 3)

void __attribute__((weak)) Host::ledPlayEvent(int event) {
  LedTask::ledPlayEvent(event);
}

void __attribute__((weak)) Host::ledRegisterEvents() {
  LedTask::registerDefaultLedEvents();
}

struct LedChannel {
  LedSample* _sample = nullptr;
  uint32_t _startTime;
  uint32_t _loopTime;
  uint16_t _sampleElemTime;
  uint8_t _loopCount;
  uint8_t _brightness;

  bool getValue(uint8_t& ledValue, uint32_t timeMS); // return true if no more channels needs to be calculated (interrupt)
};

bool LedChannel::getValue(uint8_t& ledValue, uint32_t timeMS) {
  if (_sample == nullptr) return false;
  uint32_t diff = timeMS - _startTime;
  if (diff >= _loopTime) { // one or more loop is elapsed
    uint32_t loopsDone = diff / _loopTime;
    if (_loopCount != 0) {
      if (_loopCount <= loopsDone) { // the channel is finished, next...
        _sample = nullptr;
        return false;
      }
      _loopCount -= loopsDone;
    }
    uint32_t loopsTime = loopsDone * _loopTime;
    _startTime += loopsTime;
    diff -= loopsTime;
  }
  uint32_t sampleElemTime = _sampleElemTime;
  uint32_t sampleIndex = diff / sampleElemTime;
  if (sampleIndex >= _sample->size) {
    if (_loopCount == 1) _sample = nullptr;
    return false;
  }

  uint8_t sample = _sample->data[sampleIndex];
  int channelValue = ((sample & STFLEDSAMPLE_MASK) * 255 + STFLEDSAMPLE_MASK / 2) / STFLEDSAMPLE_MASK;
  if ((sample & STFLEDSAMPLE_INTERPOLATION) != 0 && sampleIndex + 1 < _sample->size) { // Interpolation !!!
    int value2 = ((_sample->data[sampleIndex + 1] & STFLEDSAMPLE_MASK) * 255 + STFLEDSAMPLE_MASK / 2) / STFLEDSAMPLE_MASK;
    int frac = diff - sampleIndex * sampleElemTime;
    channelValue = ((sampleElemTime - frac) * channelValue + frac * value2 + sampleElemTime / 2) / sampleElemTime;
  }
  channelValue = (channelValue * _brightness + 128) / 255;
  if (channelValue > ledValue) ledValue = channelValue;
  return (sample & STFLEDSAMPLE_INTERRUPT) != 0;
}

struct LedEvent {
  inline void Set(uint8_t iLed = 0, uint8_t iChn = 0, uint8_t iBrightness = 255, LedSample* iSample = &g_ledSample1, uint16_t iSampleElemTime = 50,
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
  uint8_t _valueSoft = 0; // the value than can be read and write
  uint8_t _valueCalc = 0; // the value that was calculated via the channels
  uint8_t _valueHard = 0; // the value that was last actually sent to the led
  uint8_t _reserved = 0;

  LedChannel* _channels = nullptr;
};

template <>
const TaskDescriptor Task<LedTask>::_descriptor = {&_obj, "LedTask", 2048, 0, 1};
template class Task<LedTask>;

LedEvent LedTask::_ledEvents[STFLED_MAXEVENTS];
const uint8_t LedTask::_ledPins[] = {STFLED_PINS};
const uint8_t LedTask::_ledType[] = {STFLED_TYPE};

#  define STFLED_NUM sizeof(LedTask::_ledPins)

LedObject LedTask::_leds[STFLED_NUM];

void LedTask::ledRegisterEvent(int event, int8_t led, int8_t chn, uint8_t brightness, LedSample* sample, uint16_t sampleElemTime, uint8_t loopCount, uint32_t loopTime, int32_t uptimeSync) {
  if (event >= 0 && event < STFLED_MAXEVENTS) _ledEvents[event].Set(led, chn, brightness, sample, sampleElemTime, loopCount, loopTime, uptimeSync);
}

void LedTask::ledPlayEvent(int event) {
  if (event >= 0) {
    if (event < STFLED_MAXEVENTS) {
      LedEvent& obj = _ledEvents[event];
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

  ledRegisterEvent(STFLEDEVENT_BLE_RECEIVE, 0, 1, 128, &g_ledSample1, 50, 1, 2000, -1);
}

void LedTask::ledPlayDirect(int8_t led, int8_t chn, uint8_t brightness, LedSample* sample, uint16_t sampleElemTime, uint8_t loopCount, uint32_t loopTime, int32_t uptimeSync) {
  if (led < 0 || chn < 0 || STFLED_NUM == 0) return; // valid case: disabled event
  if (led >= STFLED_NUM) led = 0;
  if (chn >= STFLED_CHANNELS) chn = STFLED_CHANNELS - 1;
  if (_leds[led]._channels == nullptr) {
    STFLOG_ERROR("Led %d is not managed.\n", led);
    return;
  }
  LedChannel& channel = _leds[led]._channels[chn];
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
    std::lock_guard<Mutex> lock(LedTask::_obj._mutexChannels);
    channel._loopTime = loopTime;
    channel._sampleElemTime = sampleElemTime;
    channel._sample = sample;
    channel._brightness = brightness;
    channel._loopCount = loopCount;
    channel._startTime = timeMS;
  }
}

void LedTask::setLed(int idx, int value) {
  if (!SystemProvider::isLedEnabled()) value = 0;

  uint8_t ledType = _ledType[idx] & STFLEDTYPE_MASK;
  if (ledType <= 15) // PWM
    ledcWrite(ledType, value > 0 ? (value <= 255 ? value : 255) : 0);
  else if (ledType == STFLEDTYPE_BINARY) // Binary
    digitalWrite(_ledPins[idx], value < 128 ? LOW : HIGH);
}

void LedTask::setup() {
  for (int idx = 0; idx < STFLED_NUM; idx++) {
    if ((_ledType[idx] & STFLEDTYPEFLAG_MANAGED) != 0) _leds[idx]._channels = new LedChannel[STFLED_CHANNELS];
    uint8_t ledType = _ledType[idx] & STFLEDTYPE_MASK;
    if (ledType <= 15) { // PWM
      ledcSetup(ledType, 8000, 8);
      ledcAttachPin(_ledPins[idx], ledType);
    } else if (ledType == STFLEDTYPE_BINARY) { // Binary
      pinMode(_ledPins[idx], OUTPUT);
    }
    setLed(idx, 0);
  }
}

uint LedTask::loop() {
  uint32_t timeMS = Host::uptimeMS32();

  {
    std::lock_guard<Mutex> lock(_mutexChannels);
    for (int idx = 0; idx < STFLED_NUM; idx++) {
      LedObject& led = _leds[idx];
      if (led._channels != nullptr) {
        led._valueCalc = 0;
        for (int channel = 0; channel < STFLED_CHANNELS; channel++) {
          if (led._channels[channel].getValue(led._valueCalc, timeMS)) break;
        }
        led._valueSoft = led._valueCalc;
      } else {
        led._valueCalc = led._valueSoft;
      }
    }
  }
  {
    std::lock_guard<Mutex> lock(_mutexLeds);
    for (int idx = 0; idx < STFLED_NUM; idx++) {
      LedObject& led = _leds[idx];
      if (led._valueCalc != led._valueHard)
        setLed(idx, led._valueHard = led._valueCalc);
    }
  }

  uint32_t ellapsed = Host::uptimeMS32() - timeMS;
  return ellapsed < 10 ? 10 - ellapsed : 1;
}

} // namespace stf

#endif
