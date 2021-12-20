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

#include "../stf_user_config.h"
#include <stf/object.h>
#include <stf/task.h>

// str id buffer has to be minimum 20 character long buffer (see mac2strid)
#define STF_STRID_SIZE 20

// The Device
#ifndef STF_THING_NAME
#  define STF_THING_NAME "SimpleThing"
#endif

#ifndef STF_THING_PASSWORD
#  define STF_THING_PASSWORD "SimpleThing"
#endif

#ifndef STF_LOCAL_MAC
#  define STF_LOCAL_MAC 0x32, 0x43, 0x53
#endif

// Logging

#ifdef STFLOG_HEADER
#  include STFLOG_HEADER
#endif

#define STFLOG_LEVEL_SILENT  0
#define STFLOG_LEVEL_FATAL   1
#define STFLOG_LEVEL_ERROR   2
#define STFLOG_LEVEL_WARNING 3
#define STFLOG_LEVEL_INFO    4
#define STFLOG_LEVEL_DEBUG   5

#ifndef STFLOG_LEVEL
#  define STFLOG_LEVEL STFLOG_LEVEL_DEBUG
#endif

#ifndef STFLOG_PRINT
#  define STFLOG_PRINT(fmt...) Serial.printf(fmt)
#endif
#ifndef STFLOG_WRITE
#  define STFLOG_WRITE(chrOrStr) Serial.write(chrOrStr)
#endif

#if STFLOG_LEVEL < STFLOG_LEVEL_FATAL
#  undef STFLOG_FATAL
#  define STFLOG_FATAL(fmt...)
#elif !defined(STFLOG_FATAL)
#  define STFLOG_FATAL(fmt...) STFLOG_PRINT(fmt)
#endif

#if STFLOG_LEVEL < STFLOG_LEVEL_ERROR
#  undef STFLOG_ERROR
#  define STFLOG_ERROR(fmt...)
#elif !defined(STFLOG_ERROR)
#  define STFLOG_ERROR(fmt...) STFLOG_PRINT(fmt)
#endif

#if STFLOG_LEVEL < STFLOG_LEVEL_WARNING
#  undef STFLOG_WARNING
#  define STFLOG_WARNING(fmt...)
#elif !defined(STFLOG_WARNING)
#  define STFLOG_WARNING(fmt...) STFLOG_PRINT(fmt)
#endif

#if STFLOG_LEVEL < STFLOG_LEVEL_INFO
#  undef STFLOG_INFO
#  define STFLOG_INFO(fmt...)
#elif !defined(STFLOG_INFO)
#  define STFLOG_INFO(fmt...) STFLOG_PRINT(fmt)
#endif

#if STFLOG_LEVEL < STFLOG_LEVEL_DEBUG
#  undef STFLOG_DEBUG
#  define STFLOG_DEBUG(fmt...)
#elif !defined(STFLOG_DEBUG)
#  define STFLOG_DEBUG(fmt...) STFLOG_PRINT(fmt)
#endif

// LED handling

#define STFLEDEVENT_DEFAULT            0
#define STFLEDEVENT_WATCHDOG           1
#define STFLEDEVENT_SETUP              2
#define STFLEDEVENT_WIFI_CONNECTED     3
#define STFLEDEVENT_WIFI_NOT_CONNECTED 4
#define STFLEDEVENT_WIFI_OTA           5
#define STFLEDEVENT_MQTT_CONNECTED     6
#define STFLEDEVENT_MQTT_NOT_CONNECTED 7
#define STFLEDEVENT_MQTT_SEND          8
#define STFLEDEVENT_MQTT_COMMAND       9
#define STFLEDEVENT_BLE_RECEIVE        10
#define STFLEDEVENT_RF_RECEIVE         11

#ifndef STFTASK_LED
#  define STFTASK_LED 1
#endif

#if STFTASK_LED == 1
#  define STFLED_HEADER "stf/task_led.h"
#endif

#ifdef STFLED_HEADER
#  include STFLED_HEADER
#endif

#ifndef STFLED_COMMAND
#  define STFLED_COMMAND(event)
#endif

// WIFI
#ifndef STFTASK_WIFI
#  define STFTASK_WIFI 1
#endif

#if STFTASK_WIFI == 1
#  include <stf/task_wifi.h>
#endif

#if STFMQTT == 1
#  undef STFJSON
#  define STFJSON 1
#endif

// Buffers ( + Providers)

#ifndef STFBUFFER_0
#  define STFBUFFER_0
#endif
#ifndef STFBUFFER_1
#  define STFBUFFER_1
#endif
#ifndef STFBUFFER_2
#  define STFBUFFER_2
#endif
#ifndef STFBUFFER_3
#  define STFBUFFER_3
#endif
#ifndef STFBUFFER_4
#  define STFBUFFER_4
#endif
#define STFBUFFERS STFBUFFER_0 STFBUFFER_1 STFBUFFER_2 STFBUFFER_3 STFBUFFER_4
