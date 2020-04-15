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

//
//
//
// Routines to convert between the various numbering schemes for
// quadtree nodes in Keyhole.

#include "quadtree_utils.h"
#include <string.h>

namespace qtpacket {

// Substitutes for gUnit macros
#define DCHECK_LT(a,b) assert((a) < (b))

// root_numbering is the numbering system for the root quadset in
// Keyhole.  All other quadsets use default_numbering.
static const QuadtreeNumbering default_numbering(
    QuadtreeNumbering::kDefaultDepth, true);
static const QuadtreeNumbering root_numbering(
    QuadtreeNumbering::kRootDepth, false);

QuadtreeNumbering::QuadtreeNumbering(int depth, bool mangle_second_row)
    : TreeNumbering(4, depth, mangle_second_row) {
  subindex_to_levelxy_ = new LevelXY[num_nodes()];
  PrecomputeSubindexToLevelXY();
}

QuadtreeNumbering::~QuadtreeNumbering() {
  delete [] subindex_to_levelxy_;
}

void QuadtreeNumbering::SubindexToLevelXY(int subindex, int *level,
                                          int *x, int *y) const {
  assert(level);
  assert(x);
  assert(y);
  assert(InRange(subindex));

  const LevelXY &pos = subindex_to_levelxy_[subindex];
  *level = pos.level;
  *x = pos.x;
  *y = pos.y;
}

int QuadtreeNumbering::LevelXYToSubindex(int level, int x, int y) const {
  // TODO: Suggestion - faster way to do this:
  //
  // 1) First deal with the nastiness of the second row and reduce the problem
  //    to one of the subtrees rooted at the second row.
  //
  // 2) For a given level, use NodesAtLevels to compute the index of the
  //    leftmost node directly.
  //
  // 3) Use each bit of the (x, y) position to add to the leftmost
  //    index, taking into account the x, y layout.
  //
  // child( (0,0) (1,0) (1,1) (0,1) ) = ( 0 1 2 3 )
  //
  // l=0 => 0
  // l>0:
  // root_child = child( x>>(l-1) , y >>(l-1) )
  // index = nodesAtLevels(l-1) * root_child //weird lvl 1 numbering
  // index += nodesAtLevels(l-2) // everybody above you
  // // now count nodes at the same level as you, but numbered before you
  // qsize=1;
  // for i=0..l-1,
  //   index += qsize * child(x&1,y&1)
  //   qsize = qsize<<2;
  //   x = x>>1;
  //   y = y>>1;

  DCHECK_LT(level, depth());
  // Slow--just search for the answer.  Could be made faster by
  // precomputing a lookup table.
  for (int i = 0; i < num_nodes(); ++i) {
    const LevelXY *l = &subindex_to_levelxy_[i];
    if (l->level == level && l->x == x && l->y == y)
      return i;
  }
  // Should never get here--x or y out of range
  assert(false);
  return 0;
}

void QuadtreeNumbering::PrecomputeSubindexToLevelXY() {
  static const int xoffsets[] = { 0, 1, 1, 0 };
  static const int yoffsets[] = { 0, 0, 1, 1 };

  // Special-case the nasty second row of subindex numbering
  // TODO: This is broken for non-mangled trees
  if (depth() > 0) {
    memset(&subindex_to_levelxy_[0], 0, sizeof(subindex_to_levelxy_[0]));
    if (depth() > 1) {
      int nodes_in_subtree = (num_nodes() - 1) / branching_factor();
      for (int j = 0; j < branching_factor(); ++j) {
        int offset = 0;  // Offset from leftmost node on this level
        int next_level_left = 1;
        int grid_size = 4;
        int level = 1;

        // Compute "base" values of nodes at second level
        int base = 1 + j * nodes_in_subtree;
        int base_x = xoffsets[j];
        int base_y = yoffsets[j];
        for (int i = 0; i < nodes_in_subtree; ++i, ++offset) {
          // Moving to next level?
          if (i == next_level_left) {
            next_level_left = next_level_left * branching_factor() + 1;
            ++level;
            offset = 0;
            grid_size *= 2;
          }

          // Compute the x and y (see comment at the top of header)
          int counter = offset;
          int x = base_x << (level - 1);
          int y = base_y << (level - 1);
          int multiplier = 1;
          while (counter > 0) {
            x += xoffsets[(counter & 3)] * multiplier;
            y += yoffsets[(counter & 3)] * multiplier;
            counter /= 4;
            multiplier *= 2;
          }

          subindex_to_levelxy_[base + i].level = level;
          subindex_to_levelxy_[base + i].x = x;
          subindex_to_levelxy_[base + i].y = y;
        }
      }
    }
  }
}

 std::uint64_t QuadtreeNumbering::TraversalPathToGlobalNodeNumber(QuadtreePath path) {
  std::uint64_t num = 0;
  for (unsigned int i = 0; i < path.Level(); ++i)
    num = (num * 4) + path[i] + 1;
  return num;
}

QuadtreePath QuadtreeNumbering::GlobalNodeNumberToTraversalPath(std::uint64_t num) {
  QuadtreePath path;
  while (num > 0) {
    unsigned char blist = (num - 1) & 3;
    path = QuadtreePath(1, &blist) + path;
    num = (num - 1) / 4;
  }
  return path;
}

QuadtreePath
QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(std::uint64_t quadset_num,
                                                     int subindex) {
  if (quadset_num == 0)
    return root_numbering.SubindexToTraversalPath(subindex);

  QuadtreePath path = GlobalNodeNumberToTraversalPath(quadset_num);
  path = path + default_numbering.SubindexToTraversalPath(subindex);
  return path;
}

void
QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(QuadtreePath path,
                                                     std::uint64_t *quadset_num,
                                                     int *subindex) {
  if (static_cast<int>(path.Level()) < root_numbering.depth()) {
    *quadset_num = 0;
    *subindex = root_numbering.TraversalPathToSubindex(path);
  } else {
    // Split path at quadset boundary.  The root quadset goes to
    // length 3, and each child adds 4 more.
    int split = 4 * (path.Level() / 4) - 1;
    QuadtreePath quadset_path = QuadtreePath(path, split);
    *quadset_num = TraversalPathToGlobalNodeNumber(quadset_path);
    QuadtreePath subindex_path = QuadtreePath::RelativePath(quadset_path, path);
    *subindex = default_numbering.TraversalPathToSubindex(subindex_path);
  }
}

// Determine if path level corresponds to the (real or virtual) root
// node of a quadset
bool QuadtreeNumbering::IsQuadsetRootLevel(std::uint32_t level) {
  return
    (level == 0)
    || ((level >= kRootDepth - 1)
        && ((level - (kRootDepth - 1)) % (kDefaultDepth - 1) == 0));
}

void QuadtreeNumbering::QuadsetAndSubindexToLevelRowColumn(std::uint64_t quadset_num,
                                                           int subindex,
                                                           int *level,
                                                           int *row, int *col) {
  //  TraversalPathToLevelRowColumn(
  QuadtreePath path = QuadsetAndSubindexToTraversalPath(quadset_num, subindex);
  std::uint32_t ulevel, urow, ucol;
  path.GetLevelRowCol(&ulevel, &urow, &ucol);
  *level = ulevel;
  *row = urow;
  *col = ucol;
}


// Return the number of nodes (subindex values) in a quadset
int QuadtreeNumbering::NumNodes(std::uint64_t quadset_num) {
  return (quadset_num == 0)
                       ? root_numbering.num_nodes()
                       : default_numbering.num_nodes();
}

// Convert subindex to inorder numbering for a specified quadset
int QuadtreeNumbering::QuadsetAndSubindexToInorder(std::uint64_t quadset_num,
                                                   int subindex) {
  return (quadset_num == 0)
                       ? root_numbering.SubindexToInorder(subindex)
                       : default_numbering.SubindexToInorder(subindex);
}

// Get numbering for given quadset
const
QuadtreeNumbering &QuadtreeNumbering::Numbering(std::uint64_t quadset_num) {
  return (quadset_num == 0)
                       ? root_numbering
                       : default_numbering;
}


// Given a tile's level, row and column, return its Maps tile name.
std::string QuadtreeNumbering::LevelRowColumnToMapsTraversalPath(int level,
                                                            int row, int col) {
  QuadtreePath path(level, row, col);
  std::string str = "t";
  for (std::uint32_t i = 0; i < path.Level(); ++i)
    str.push_back('t' - path[i]);
  return str;
}

void
QuadtreeNumbering::MapsTraversalPathToLevelRowColumn(const std::string &path,
                                                     int *level,
                                                     int *row, int *col) {
  QuadtreePath path0based;
  // Maps quadtree paths are reversed (t = 0, q = 3)
  for (std::uint32_t i = 1; i < path.size(); ++i)
    path0based = path0based.Child('t' - path[i]);
  //  TraversalPathToLevelRowColumn(path0based, level, row, col);
  std::uint32_t ulevel, urow, ucol;
  path0based.GetLevelRowCol(&ulevel, &urow, &ucol);
  *level = ulevel;
  *row = urow;
  *col = ucol;
}

}  // namespace qtpacket
