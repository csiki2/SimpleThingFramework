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

void setup() {
  stf::Host::setup();
  stf::TaskRoot::setupTasks();
  stf::task_setup();
}

void loop() {
  STFLED_COMMAND(STFLEDEVENT_WATCHDOG);
  uint wait1 = stf::task_loop();
  uint wait2 = stf::TaskRoot::loopTasks();
  delay(wait1<wait2 ? wait1 : wait2);
}
