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

#include <stf/data_block.h>
#include <stf/data_field.h>

namespace stf {

struct DataCache {
  DataBlock _headElem; // filled when complex elem is used
  DataBlock _block1;
  DataBlock _block2;
  DataBlock _block_device;
  DeviceBlock _device;

  bool _flagDevice : 1;
  bool _flagBlock1 : 1;
  bool _flagBlock2 : 1;
  bool _flagHeadElem : 1;
  bool _flagFeederActive : 1;

  inline void reset() {
    resetHeadElem();

    if (_flagFeederActive) return;
    if (_flagDevice) {
      _device.reset();
      _block_device.reset();
    }
    if (_flagBlock1) _block1.reset();
    if (_flagBlock2) _block2.reset();

    _flagDevice = _flagBlock1 = _flagBlock2 = false;
  }

  inline void forceReset() {
    _headElem.reset();
    _block1.reset();
    _block2.reset();
    _block_device.reset();
    _device.reset();
    _flagDevice = _flagBlock1 = _flagBlock2 = _flagHeadElem = _flagFeederActive = false;
  }

  inline void setHeadElem(const DataBlock& block) {
    _headElem = block;
    _flagHeadElem = true;
  }

  inline void resetHeadElem() {
    if (_flagHeadElem) {
      _headElem.reset();
      _flagHeadElem = false;
    }
  }

  inline void setBlock1(const DataBlock& block) {
    _block1 = block;
    _flagBlock1 = true;
  }

  inline void setBlock2(const DataBlock& block) {
    _block2 = block;
    _flagBlock2 = true;
  }

  inline void setBlockDevice(const DataBlock& block) {
    _block_device = block;
    _flagDevice = true;
  }

  void addBlock(const DataBlock& block, uint8_t cmd);
};

} // namespace stf
