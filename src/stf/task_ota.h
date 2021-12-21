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

#include <stf/os.h>

#ifdef STFTASK_OTA

#  include <stf/provider.h>
#  include <ArduinoOTA.h>

#  ifndef STFOTA_PORT
#    define STFOTA_PORT 3232
#  endif

namespace stf {

class DiscoveryBlock;

class OTAProvider : public Provider {
public:
  OTAProvider();

  uint loop() override;
  uint systemUpdate(DataBuffer* systemBuffer, uint32_t uptimeS, ESystemMessageType type) override;
  void feedback(const FeedbackInfo& info) override;

protected:
  static void onStart();
  static void onEnd();
  static void onProgress(unsigned progress, unsigned total);
  static void onError(ota_error_t error);

  bool _enabled = false;

  ArduinoOTAClass* _arduinoOTA = nullptr;

  static const DiscoveryBlock _switch;
};

} // namespace stf

#endif
