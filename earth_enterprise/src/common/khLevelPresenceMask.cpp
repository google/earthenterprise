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

#include "common/khLevelPresenceMask.h"

#include <string.h>

#include "common/khExtents.h"


// khLevelPresenceMask
khLevelPresenceMask::khLevelPresenceMask(const khLevelPresenceMask &o)
    : khLevelCoverage(o),
      buf(TransferOwnership(new uchar[BufferSize()])) {
  memcpy(&buf[0], &o.buf[0], BufferSize());
}


khLevelPresenceMask::khLevelPresenceMask(uint lev,
                                         const khExtents<uint32> &extents,
                                         bool set_present)
    : khLevelCoverage(lev, extents),
      buf(TransferOwnership(new uchar[BufferSize()])) {
  SetPresence(set_present);
}


khLevelPresenceMask::khLevelPresenceMask(uint lev,
                                         const khExtents<uint32> &extents,
                                         uchar *src)
    : khLevelCoverage(lev, extents),
      buf(TransferOwnership(new uchar[BufferSize()])) {
  memcpy(&buf[0], src, BufferSize());
}


bool khLevelPresenceMask::GetPresence(uint32 row, uint32 col) const {
  if (extents.ContainsRowCol(row, col)) {
    uint32 r = row - extents.beginRow();
    uint32 c = col - extents.beginCol();
    uint32 tilePos = r * extents.numCols() + c;
    uint32 byteNum = tilePos / 8;
    uint32 byteOffset = tilePos % 8;

    assert(byteNum < BufferSize());
    return static_cast<bool>((buf[byteNum] >> byteOffset) & 0x1);
  } else {
    return false;
  }
}

void khLevelPresenceMask::SetPresence(bool set_present) {
  if (set_present)
    // Note: actually should be filled only elements that covered
    // by extent, but in GetPresence() we always check for belonging to
    // extent. So we just fill all elements with present.
    memset(&buf[0], 0xff, BufferSize());
  else
    memset(&buf[0], 0, BufferSize());
}

// Specialization of SetPresence() that is used from khPresenceMask.
// Here, we have an assertion that checks if we're setting a presence bit that
// hasn't been set.
template<> void khLevelPresenceMask::SetPresence(
    uint32 row, uint32 col, bool present, const Int2Type<false>&) {
  uint32 r = row - extents.beginRow();
  uint32 c = col - extents.beginCol();
  uint32 tilePos = r * extents.numCols() + c;
  uint32 byteNum = tilePos / 8;
  uint32 byteOffset = tilePos % 8;

  assert(byteNum < BufferSize());

  // Make sure if we're setting a presence bit that hasn't been set.
  // While not really a fatal problem, it does show that we're
  // repeating work and we shouldn't be doing that. :-)
  assert(!present || !(buf[byteNum] & (0x1 << byteOffset)));

  buf[byteNum] =
    (buf[byteNum] & ~(0x1 << byteOffset)) |  // strip out old value
      ((static_cast<uchar>(present) & 0x1) << byteOffset);  // 'or' in new value
}

// Specialization of SetPresence() that is used from khCoverageMask.
// Since dataset may have overlapped polygons, we may set covered/presence bit
// more than one time for specific quad. So, there is no an assertion whether
// we're setting a covered bit that hasn't been set.
template<> void khLevelPresenceMask::SetPresence(
    uint32 row, uint32 col, bool present, const Int2Type<true>&) {
  uint32 r = row - extents.beginRow();
  uint32 c = col - extents.beginCol();
  uint32 tilePos = r * extents.numCols() + c;
  uint32 byteNum = tilePos / 8;
  uint32 byteOffset = tilePos % 8;

  assert(byteNum < BufferSize());

  buf[byteNum] =
    (buf[byteNum] & ~(0x1 << byteOffset)) |  // strip out old value
      ((static_cast<uchar>(present) & 0x1) << byteOffset);  // 'or' in new value
}

// An explicit instantiation for SetPrecence() specializations.
template void khLevelPresenceMask::SetPresence<static_cast<int>(false)>(
    uint32, uint32, bool, const Int2Type<false>&);
template void khLevelPresenceMask::SetPresence<static_cast<int>(true)>(
    uint32, uint32, bool, const Int2Type<true>&);
