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

  Mac::integrityCheck();

  esp_efuse_mac_get_default(_block.macBuffer);
  _block.create(nullptr, 6);

  STFLOG_INFO("Host id: %s\n", _info.strId);
}

uint32_t ElapsedTime::elapsedTime() const {
  if (_time == 0) return MaxElapsed;
  uint32_t diff = Host::uptimeMS32() - _time;
  if (diff < MaxElapsed) return diff;
  _time = 0; // we reset it so we never overrun from this point
  return MaxElapsed;
}

void ElapsedTime::reset() {
  _time = Host::uptimeMS32();
  if (_time == 0) _time = -1; // resolve the very rare case when uptime is collide with ElapsedTime's 0
}

bool ElapsedTime::operator==(const ElapsedTime& t) const {
  uint32_t uptime = Host::uptimeMS32();
  if (uptime - _time >= MaxElapsed) _time = 0;
  if (uptime - t._time >= MaxElapsed) t._time = 0;
  return _time == t._time;
}

ElapsedTime& ElapsedTime::operator=(const ElapsedTime& t) {
  if (Host::uptimeMS32() - t._time >= MaxElapsed) t._time = 0;
  _time = t._time;
  return *this;
}

} // namespace stf
