/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef QUADTREEPATH_H__
#define QUADTREEPATH_H__

#include <string>
#include <khTypes.h>

class EndianWriteBuffer;
class EndianReadBuffer;

// QuadtreePath - define packed representation for quadtree path.
//
// The path is stored in a uint64.  The upper 48 bits store the path
// (up to 24 levels), each level represented by 2 bits, lower numbered
// levels in the more significant bits.  No bits represent level 0
// (which has only 1 tile), so the most significant two bits represent
// level 1.
//
// Data in memory is stored in host format.

class QuadtreePath {
 public:
  static const uint32 kMaxLevel = 24;  // number of levels represented
  static const uint32 kStoreSize = sizeof(uint64);  // bytes required to store
  static const uint32 kChildCount = 4;  // number of children at each level

  QuadtreePath() : path_(0) {}
  QuadtreePath(uint32 level, uint32 row, uint32 col);
  QuadtreePath(uint32 level, const uchar blist[kMaxLevel]);
  QuadtreePath(const std::string &blist);
  // copy other path, but prune at given level
  QuadtreePath(const QuadtreePath &other, uint32 level);

  inline bool IsValid() const {         // level in range, no stray bits
    return Level() <= kMaxLevel
      && (0 == (path_ & ~(PathMask(Level()) | kLevelMask)));
  }
  bool operator<(const QuadtreePath &other) const;
  inline bool operator>(const QuadtreePath &other) const { return other < *this; }
  inline bool operator==(const QuadtreePath &other) const {
    return path_ == other.path_;
  }
  inline bool operator!=(const QuadtreePath &other) const {
    return path_ != other.path_;
  }

  uint64 GetGenerationSequence() const;

  // Test for paths in postorder (normal ordering is preorder).  False
  // if path1 == path2.
  static inline bool IsPostorder(const QuadtreePath &path1,
                                 const QuadtreePath &path2) {
    return !path1.IsAncestorOf(path2)
      && (path2.IsAncestorOf(path1) || path2 > path1);
  }

  void GetLevelRowCol(uint32 *level, uint32 *row, uint32 *col) const;
  inline uint32 Level() const { return path_ & kLevelMask; }
  QuadtreePath Parent() const;
  QuadtreePath Child(uint32 child) const;
  inline uint32 WhichChild() const {
    return (path_ >> (kTotalBits - Level()*kLevelBits)) & kLevelBitMask;
  }
  void Push(EndianWriteBuffer& buf) const;
  void Pull(EndianReadBuffer& buf);
  std::string AsString(void) const;

  // Advance to next node in same level, return false at end of level
  bool AdvanceInLevel();

  // Advance to next node in preorder, return false at end of nodes <=
  // specified max level.
  bool Advance(uint32 max_level);

  // NOTE: this is inclusive (returns true if other is same path)
  bool IsAncestorOf(const QuadtreePath &other) const;

  // asserts parent.IsAncestorOf(child)
  static QuadtreePath RelativePath(const QuadtreePath &parent,
                                   const QuadtreePath &child);

  // Return the row, column and width of the subtile which the child_qpath
  // occupies. Note: the method does not go beyond a single pixel width for
  // the TileCoordinates, it will return the single pixel width and coordinate
  // in that case.
  // This is based on Earth quadtree tile pattern
  // +---+---+
  // | 3 | 2 |
  // +---+---+
  // | 0 | 1 |
  // +---+---+
  // tile_width: the width and height of the tile for this quadtree path.
  // child_qpath: the quadtree path of the child quad node for which we're
  //              computing the subimage.
  // row: the row index of the upper left part of the subimage within the
  //      current tile.
  // column: the column index of the upper left part of the subimage within the
  //      current tile.
  // width: the width and height of the subimage within the current tile.
  // returns false if the child_qpath is not a child of this path, true o.w.
  bool ChildTileCoordinates(int tile_width, const QuadtreePath& child_qpath,
                            int* row, int* column, int* width) const;

  // Concatenate paths
  QuadtreePath Concatenate(const QuadtreePath sub_path) const;
  inline QuadtreePath operator+(const QuadtreePath other) const {
    return Concatenate(other);
  }

  // This is useful for converting a prefix of the QuadtreePath into
  // an array index at the specified level.
  // QuadtreePath("23121").AsIndex(4) -> (binary) 10110110 -> 182
  // QuadtreePath("31").AsIndex(2) -> (binary) 1101 -> 13
  // QuadtreePath("1").AsIndex(1) -> (binary) 01 -> 1
  // QuadtreePath("").AsIndex(0) -> 0
  inline uint64 AsIndex(uint32 level) const {
    return (path_ >> (kTotalBits - level*kLevelBits));
  }

  // return the branch made at the specified position
  // --- example ---
  // QuadtreePath path("201");
  // path[0] -> 2
  // path[1] -> 0
  // path[2] -> 1
  uint operator[](uint32 position) const;

  // *************************************************************************
  // ***  Quadrant routines
  // ***    - quadrants are numbered like this
  // ***
  // ***  +----+----+
  // ***  | 2  | 3  |
  // ***  +----+----+
  // ***  | 0  | 1  |
  // ***  +----+----+
  // *************************************************************************
  // Gets offset for parent cells pixel_buffer(which is ordered from left to
  // right and then from bottom to top). Refer Minifytile.
  static uint32 QuadToBufferOffset(uint quad, uint32 tileWidth,
                                   uint32 tileHeight) {
    assert(quad < 4);
    switch (quad) {
      case 0:
        return 0;
      case 1:
        return tileWidth / 2;
      case 2:
        return (tileHeight * tileWidth) / 2;
      case 3:
        return ((tileHeight + 1) * tileWidth) / 2;
    }
    return 0;

  }

  // ***  +----+----+
  // ***  | 2  | 3  |
  // ***  +----+----+
  // ***  | 0  | 1  |
  // ***  +----+----+
  // find the tile in the next level (more detail) that maps to the
  // specified quad in range [0,3]
  // Better named MagnifyRowColToChildRowColForQuad.
  static void MagnifyQuadAddr(uint32 inRow, uint32 inCol, uint inQuad,
                              uint32 &outRow, uint32 &outCol)
  {
    assert(inQuad < 4);
    switch(inQuad) {
      case 0:
        outRow = inRow*2;
        outCol = inCol*2;
        break;
      case 1:
        outRow = inRow*2;
        outCol = (inCol*2) + 1;
        break;
      case 2:
        outRow = (inRow*2) + 1;
        outCol = inCol*2;
        break;
      case 3:
        outRow = (inRow*2) + 1;
        outCol = (inCol*2) + 1;
        break;
    }
  }

 private:
  void FromBranchlist(uint32 level, const uchar blist[]);
  QuadtreePath(uint64 path) : path_(path) { assert(IsValid()); }
  static const uint32 kLevelBits = 2;       // bits per level in packed path_
  static const uint64 kLevelBitMask = 0x03;
  static const uint32 kTotalBits = 64;      // total storage bits
  static const uint64 kPathMask = ~(~uint64(0) >> (kMaxLevel * kLevelBits));
  static const uint64 kLevelMask = ~kPathMask;
  inline uint64 PathBits() const { return path_ & kPathMask; }
  inline uint64 PathMask(uint32 level) const {
    return kPathMask << ((kMaxLevel - level) * kLevelBits);
  }
  // like PathBits(), but clamps to supplied level
  inline uint64 PathBits(uint32 level) const { return path_ & PathMask(level);}
  inline uint32 LevelBitsAtPos(uint32 position) const {
    return (path_ >> (kTotalBits - (position+1)*kLevelBits)) & kLevelBitMask;
  }

  friend class QuadtreePathUnitTest;
  // This is the only data which should be stored in an instance of
  // this class
  uint64 path_;
};

#endif  // QUADTREEPATH_H__
