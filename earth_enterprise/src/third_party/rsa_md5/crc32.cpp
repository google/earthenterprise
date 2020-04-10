// Copyright 2017 Google Inc.
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

#include "crc.h"
#include "crc32.h"

// CRC-32 hash function used by Google Earth Enterprise Fusion (uses
// CRC class).

static CRC *crc = CRC::Default(32,0);   // just initialize once

std::uint32_t Crc32(const void *buffer, size_t buffer_len) {
  std::uint64_t lo, hi;                        // holders for CRC value
  crc->Empty(&lo, &hi);                 // init to CRC of empty string
  crc->Extend(&lo, &hi, buffer, buffer_len);
  return static_cast<std::uint32_t>(lo & 0xFFFFFFFF);
}
