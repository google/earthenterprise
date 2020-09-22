// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/verref_storage.h"

#include "common/khStringUtils.h"
#include "common/khConstants.h"


// _VerRefDef implementation.
_VerRefDef::_VerRefDef() : ver_num(0) {
}

_VerRefDef::_VerRefDef(const std::string& _asset_name, std::uint32_t _ver_num)
    : asset_name(_asset_name),
      ver_num(_ver_num) {
}

_VerRefDef::_VerRefDef(const std::string& ref) {
  size_t pos = ref.rfind(kAssetVersionNumPrefix);
  assert(pos != std::string::npos);

  if (pos != std::string::npos) {
    asset_name = std::string(ref, 0, pos);
    char *end;
    ver_num = strtol(
        ref.c_str() + pos + kAssetVersionNumPrefix.size(), &end, 10);
  } else {
    ver_num = 0;
  }
}

const std::string& _VerRefDef::GetRef() const {
  if (Valid()) {
    if (ref.empty()) {
      // Build assetversion reference path.
      ref = asset_name + kAssetVersionNumPrefix + Itoa(ver_num);
    }
  } else {
    ref.clear();
  }

  return ref;
}

// VerRefGen implementation.
VerRefGen::VerRefGen() {
}

VerRefGen::VerRefGen(const std::string& val)
    :  start_(val) {
  _VerRefDef _tmp(val);
  finish_ = iterator(_VerRefDef(_tmp.asset_name, 0));
}
