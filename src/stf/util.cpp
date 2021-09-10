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

#include <stf/util.h>

namespace stf {

int getArrayIndex(const char* str_, uint strLen_, const char* arr_[], uint arrLen_) {
  if (str_ == nullptr) return -1;
  for (uint idx = 0; idx < arrLen_; idx++) {
    const char* cmp = arr_[idx];
    if (strncmp(str_, cmp, strLen_) == 0 && cmp[strLen_] == 0) return (int)idx;
  }
  return -1;
}

} // namespace stf
