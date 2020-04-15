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


// Test helpers for tests needing ordered or random quadtree paths

#include <cstdint>
#include <stdlib.h>
#include <quadtreepath.h>

namespace qtpathgen {

QuadtreePath HashPath(std::uint32_t hash_val) {
  // Generate a unique, reproducible, somewhat randomly ordered
  // quadtree path from hash_val
  const std::uint32_t kHash1 = 0xA36535A6;
  const std::uint32_t kHash2 = 0xF0A52BE1; 

  unsigned char blist[QuadtreePath::kMaxLevel];
  std::uint32_t bits = hash_val ^ kHash1;
  std::uint32_t level = 16;
  std::int32_t extra_levels = bits & 0x07;
  for (std::uint32_t i = 0; i < level; ++i) {
    blist[i] = bits & 0x03;
    bits = bits >> 2;
  }

  std::uint32_t bits2 = hash_val ^ kHash2;
  while (extra_levels-- > 0) {
    blist[level++] = bits2 & 0x03;
    bits2 = bits2 >> 2;
  }
  return QuadtreePath(level, blist);
}

} // namespace qtpathgen
