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


// QuadtreePath - define packed representation for quadtree path.

#include <string.h>
#include <algorithm>

#include "khEndian.h"
#include "quadtreepath.h"
#include <khTileAddrConsts.h>

static_assert(QuadtreePath::kMaxLevel >= MaxClientLevel,
                   "Invalid Quadtree Path kMax Level");

const std::uint32_t QuadtreePath::kMaxLevel;
const std::uint32_t QuadtreePath::kStoreSize;
const std::uint32_t QuadtreePath::kChildCount;

// Construct from level, row, col.  Code adapated from kbf.cpp

QuadtreePath::QuadtreePath(std::uint32_t level, std::uint32_t row, std::uint32_t col) : path_(0) {
  static const std::uint64_t order[][2] = { {0, 3}, {1, 2} };

  assert(level <= kMaxLevel);

  for (std::uint32_t j = 0; j < level; ++j) {
    std::uint32_t right = 0x01 & (col >> (level - j - 1));
    std::uint32_t top = 0x01 & (row >> (level - j - 1));
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
 std::uint64_t QuadtreePath::GetGenerationSequence() const {
  const std::uint32_t level = Level();
  std::uint64_t sequence = path_;
  std::uint64_t check_for_2_or_3_mask = ((std::uint64_t)0x1) << (kTotalBits - 1);
  std::uint64_t interchange_2_or_3_mask = ((std::uint64_t)0x01) << (kTotalBits - 2);

  for (std::uint32_t j = 0; j < level; ++j, check_for_2_or_3_mask >>= 2,
                                     interchange_2_or_3_mask >>= 2) {
    if (sequence & check_for_2_or_3_mask) {
      sequence ^= interchange_2_or_3_mask;
    }
  }
  return sequence;
}


void QuadtreePath::FromBranchlist(std::uint32_t level, const unsigned char blist[]) {
  assert(level <= kMaxLevel);

  for (std::uint32_t j = 0; j < level; ++j) {
    path_ |= (blist[j] & kLevelBitMask) << (kTotalBits - ((j+1) * kLevelBits));
  }
  path_ |= level;
}


// Construct from blist (binary or ASCII - ignores all but lower 2
// bits of each level, depends on fact that lower 2 bits of '0', '1',
// '2', and '3' are same as binary representation)
QuadtreePath::QuadtreePath(std::uint32_t level, const unsigned char blist[kMaxLevel]) :
    path_(0) {
  FromBranchlist(level, blist);
}

// like above, but as convenience works with std::string
QuadtreePath::QuadtreePath(const std::string &blist) : path_(0) {
  FromBranchlist(blist.size(), (unsigned char*)&blist[0]);
}


QuadtreePath::QuadtreePath(const QuadtreePath &other, std::uint32_t level) : path_(0)
{
  std::uint32_t lev = std::min(level, other.Level());
  path_ = other.PathBits(lev) | lev;
  assert(IsValid());
}

std::string
QuadtreePath::AsString(void) const {
  std::string result;
  result.resize(Level());
  for (unsigned int i = 0; i < Level(); ++i) {
    result[i] = '0' + LevelBitsAtPos(i);
  }
  return result;
}

// Extract level, row, column (adapted from kbf.cpp)

void QuadtreePath::GetLevelRowCol(std::uint32_t *level, std::uint32_t *row, std::uint32_t *col) const {
  static const std::uint32_t rowbits[] = {0x00, 0x00, 0x01, 0x01};
  static const std::uint32_t colbits[] = {0x00, 0x01, 0x01, 0x00};

  std::uint32_t row_val = 0;
  std::uint32_t col_val = 0;

  for (std::uint32_t j = 0; j < Level(); ++j) {
    std::uint32_t level_bits = LevelBitsAtPos(j);
    row_val = (row_val << 1) | (rowbits[level_bits]);
    col_val = (col_val << 1) | (colbits[level_bits]);
  }

  *level = Level();
  *row = row_val;
  *col = col_val;
}

// Preorder comparison operator

bool QuadtreePath::operator<(const QuadtreePath &other) const {
  std::uint32_t minlev = (Level() < other.Level()) ? Level() : other.Level();

  // if same up to min level, then lower level comes first,
  // otherwise just do integer compare (most sig. bits are lower levels).
  std::uint64_t mask = ~(~std::uint64_t(0) >> (minlev * kLevelBits));
  if (mask & (path_ ^ other.path_)) {   // if differ at min level
    return PathBits() < other.PathBits();
  } else {                              // else lower level is parent
    return Level() < other.Level();
  }
}

// Advance to next node in same level, return false at end of level

bool QuadtreePath::AdvanceInLevel() {
  std::uint64_t path_bits = PathBits();
  std::uint64_t path_mask = PathMask(Level());
  if (path_bits != path_mask) {
    path_ += std::uint64_t(1) << (kTotalBits - Level()*kLevelBits);
    assert(IsValid());
    return true;
  } else {
    return false;
  }
}

// Advance to next node in preorder, return false if no more nodes.
// Only nodes at levels <= max_level are generated.

bool QuadtreePath::Advance(std::uint32_t max_level) {
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
  std::uint32_t new_level = Level() - 1;

  return QuadtreePath((path_ & (kPathMask << kLevelBits*(kMaxLevel - new_level)))
                      | new_level);
}

// Return path to child (must be in range [0..3])

QuadtreePath QuadtreePath::Child(std::uint32_t child) const {
  assert(Level() <= kMaxLevel);
  assert(child <= 3);
  std::uint32_t new_level = Level() + 1;
  return QuadtreePath(PathBits()
                      | std::uint64_t(child) << (kTotalBits - new_level*kLevelBits)
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
  unsigned int levelDiff = child.Level() - parent.Level();
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
  for(std::uint32_t level = 0; level < relative_qpath.Level() && *width > 1; ++level) {
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
  std::uint64_t level = Level() + sub_path.Level();
  assert(level <= kMaxLevel);
  return QuadtreePath((path_ & kPathMask)
                      | ((sub_path.path_ & kPathMask) >> Level()*kLevelBits)
                      | level);
}

unsigned int QuadtreePath::operator[](std::uint32_t position) const
{
  assert(position < Level());
  return LevelBitsAtPos(position);
}
