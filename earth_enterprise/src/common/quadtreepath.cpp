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


// QuadtreePath - define packed representation for quadtree path.

#include <string.h>
#include <algorithm>

#include "khEndian.h"
#include "quadtreepath.h"
#include <khAssert.h>
#include <khTileAddrConsts.h>

COMPILE_TIME_CHECK(QuadtreePath::kMaxLevel >= MaxClientLevel,
                   InvalidQuadtreePathkMaxLevel);

const uint32 QuadtreePath::kMaxLevel;
const uint32 QuadtreePath::kStoreSize;
const uint32 QuadtreePath::kChildCount;

// Construct from level, row, col.  Code adapated from kbf.cpp

QuadtreePath::QuadtreePath(uint32 level, uint32 row, uint32 col) : path_(0) {
  static const uint64 order[][2] = { {0, 3}, {1, 2} };

  assert(level <= kMaxLevel);

  for (uint32 j = 0; j < level; ++j) {
    uint32 right = 0x01 & (col >> (level - j - 1));
    uint32 top = 0x01 & (row >> (level - j - 1));
    path_ |= order[right][top] << (kTotalBits - ((j+1) * kLevelBits));
  }

  path_ |= level;
}

// The generation sequence is
// ***  +----+----+
// ***  | 2  | 3  |
// ***  +----+----+
// ***  | 0  | 1  |
// ***  +----+----+
// where as the quad sequence is
// ***  +----+----+
// ***  | 3  | 2  |
// ***  +----+----+
// ***  | 0  | 1  |
// ***  +----+----+
// So to convert a QuadtreePath to generation sequence need to twiddle all 2's
// to 3 and all 3's to 2's.
uint64 QuadtreePath::GetGenerationSequence() const {
  const uint32 level = Level();
  uint64 sequence = path_;
  uint64 check_for_2_or_3_mask = ((uint64)0x1) << (kTotalBits - 1);
  uint64 interchange_2_or_3_mask = ((uint64)0x01) << (kTotalBits - 2);

  for (uint32 j = 0; j < level; ++j, check_for_2_or_3_mask >>= 2,
                                     interchange_2_or_3_mask >>= 2) {
    if (sequence & check_for_2_or_3_mask) {
      sequence ^= interchange_2_or_3_mask;
    }
  }
  return sequence;
}


void QuadtreePath::FromBranchlist(uint32 level, const uchar blist[]) {
  assert(level <= kMaxLevel);

  for (uint32 j = 0; j < level; ++j) {
    path_ |= (blist[j] & kLevelBitMask) << (kTotalBits - ((j+1) * kLevelBits));
  }
  path_ |= level;
}


// Construct from blist (binary or ASCII - ignores all but lower 2
// bits of each level, depends on fact that lower 2 bits of '0', '1',
// '2', and '3' are same as binary representation)
QuadtreePath::QuadtreePath(uint32 level, const uchar blist[kMaxLevel]) :
    path_(0) {
  FromBranchlist(level, blist);
}

// like above, but as convenience works with std::string
QuadtreePath::QuadtreePath(const std::string &blist) : path_(0) {
  FromBranchlist(blist.size(), (uchar*)&blist[0]);
}


QuadtreePath::QuadtreePath(const QuadtreePath &other, uint32 level) : path_(0)
{
  uint32 lev = std::min(level, other.Level());
  path_ = other.PathBits(lev) | lev;
  assert(IsValid());
}

std::string
QuadtreePath::AsString(void) const {
  std::string result;
  result.resize(Level());
  for (uint i = 0; i < Level(); ++i) {
    result[i] = '0' + LevelBitsAtPos(i);
  }
  return result;
}

// Extract level, row, column (adapted from kbf.cpp)

void QuadtreePath::GetLevelRowCol(uint32 *level, uint32 *row, uint32 *col) const {
  static const uint32 rowbits[] = {0x00, 0x00, 0x01, 0x01};
  static const uint32 colbits[] = {0x00, 0x01, 0x01, 0x00};

  uint32 row_val = 0;
  uint32 col_val = 0;

  for (uint32 j = 0; j < Level(); ++j) {
    uint32 level_bits = LevelBitsAtPos(j);
    row_val = (row_val << 1) | (rowbits[level_bits]);
    col_val = (col_val << 1) | (colbits[level_bits]);
  }

  *level = Level();
  *row = row_val;
  *col = col_val;
}

// Preorder comparison operator

bool QuadtreePath::operator<(const QuadtreePath &other) const {
  uint32 minlev = (Level() < other.Level()) ? Level() : other.Level();

  // if same up to min level, then lower level comes first,
  // otherwise just do integer compare (most sig. bits are lower levels).
  uint64 mask = ~(~uint64(0) >> (minlev * kLevelBits));
  if (mask & (path_ ^ other.path_)) {   // if differ at min level
    return PathBits() < other.PathBits();
  } else {                              // else lower level is parent
    return Level() < other.Level();
  }
}

// Advance to next node in same level, return false at end of level

bool QuadtreePath::AdvanceInLevel() {
  uint64 path_bits = PathBits();
  uint64 path_mask = PathMask(Level());
  if (path_bits != path_mask) {
    path_ += uint64(1) << (kTotalBits - Level()*kLevelBits);
    assert(IsValid());
    return true;
  } else {
    return false;
  }
}

// Advance to next node in preorder, return false if no more nodes.
// Only nodes at levels <= max_level are generated.

bool QuadtreePath::Advance(uint32 max_level) {
  assert(max_level > 0);
  assert(Level() <= max_level);
  if (Level() < max_level) {
    *this = Child(0);
    return true;
  }  else {
    while (WhichChild() == kChildCount-1) {
      *this = Parent();
    }
    return AdvanceInLevel();
  }
}

// Return path to parent

QuadtreePath QuadtreePath::Parent() const {
  assert(Level() > 0);
  uint32 new_level = Level() - 1;

  return QuadtreePath((path_ & (kPathMask << kLevelBits*(kMaxLevel - new_level)))
                      | new_level);
}

// Return path to child (must be in range [0..3])

QuadtreePath QuadtreePath::Child(uint32 child) const {
  assert(Level() <= kMaxLevel);
  assert(child <= 3);
  uint32 new_level = Level() + 1;
  return QuadtreePath(PathBits()
                      | uint64(child) << (kTotalBits - new_level*kLevelBits)
                      | new_level);
}

// Push to/Pull from endian buffer for file storage
void QuadtreePath::Push(EndianWriteBuffer& buf) const {
  buf << path_;
}

void QuadtreePath::Pull(EndianReadBuffer& buf) {
  buf >> path_;
  if (!IsValid()) {
    throw khSimpleException(
        "QuadtreePath::Pull: invalid data");
  }
}

bool QuadtreePath::IsAncestorOf(const QuadtreePath &other) const {
  if (Level() <= other.Level()) {
    return PathBits(Level()) == other.PathBits(Level());
  } else {
    return false;
  }
}


QuadtreePath QuadtreePath::RelativePath(const QuadtreePath &parent,
                                        const QuadtreePath &child)
{
  assert(parent.IsAncestorOf(child));
  uint levelDiff = child.Level() - parent.Level();
  return QuadtreePath((child.PathBits() << (parent.Level() * kLevelBits)) |
                      levelDiff);
}

bool QuadtreePath::ChildTileCoordinates(int tile_width,
                                       const QuadtreePath& child_qpath,
                                       int* row, int* column,
                                       int* width) const {
  if (!IsAncestorOf(child_qpath)) {
    return false;
  }

  QuadtreePath relative_qpath = QuadtreePath::RelativePath(*this, child_qpath);
  *width = tile_width;
  *row = 0;
  *column = 0;
  // We will stop if we get to 1 pixel wide subtree...at that point we're done.
  for(uint32 level = 0; level < relative_qpath.Level() && *width > 1; ++level) {
    int quad = relative_qpath[level];
    *width >>= 1;
    if (quad == 0) {
      *row += *width;
    } else if (quad == 1) {
      *row += *width;
      *column += *width;
    } else if (quad == 2) {
      *column += *width;
    }  // quad == 3 is the upper left (image origin).
  }
  return true;
}

QuadtreePath QuadtreePath::Concatenate(const QuadtreePath sub_path) const {
  uint64 level = Level() + sub_path.Level();
  assert(level <= kMaxLevel);
  return QuadtreePath((path_ & kPathMask)
                      | ((sub_path.path_ & kPathMask) >> Level()*kLevelBits)
                      | level);
}

uint QuadtreePath::operator[](uint32 position) const
{
  assert(position < Level());
  return LevelBitsAtPos(position);
}
