/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

//
//
//
// Routines to convert between the various numbering schemes for
// quadtree nodes in Keyhole.
//
// A quadtree is a specialization of a tree with a branching factor of 4.
// Nodes represent a tiling of a 2D surface, like the Earth.
//
//
// There is another numbering scheme for a traversal path in addition
// to the string representation documented in tree_utils.h.  In this
// scheme, each path through the tree is assigned a 64 bit integer.
// At each level, the 4 children are numbered 1 to 4 (NOT 0 to 3).  To
// add a level to the traversal, multiply the path so far by 4, and
// add the child.  Examples:
//
// 0x00000000       the root node
// 0x00000001       the first child of the root node
// 0x00000005       the first child of the first child of the root node
//
// This is what the "subindex" numbering would look like if it didn't
// have the strange second row.
//
// We refer to these 64-bit node names as "Global node numbers"
// because they are typically used to identify nodes in a very large
// quadtree (in particular, the quadtree that covers the whole Earth).
//
//
// The four children of a node represent a subdivision of the node
// into 4 equal pieces.  Children are ordered this way:
//
// 3 2
// 0 1
//
// When converting to x and y coordinates, x is positive to the right,
// and y is positive up.  So the coordinates for the 4 children above
// are:
//
// (0, 1) (1, 1)
// (0, 0) (1, 0)
//
// Two levels below a node, there are 16 children, and their ordering
// can be determined recursively:
//
// 14 15 10 11
// 12 13  8  9
//  3  2  7  6
//  0  1  4  5
//
// This ordering is implicitly assumed by the Keyhole client.
//
//
// The tile processing system identifies tiles by level, row, and
// column.  At a given level, the row and column start at (0, 0) in
// the upper left, with row increasing as you go down, and columns
// increasing toward the right:
//
// (row, column) =
//
// (0, 0) (0, 1) (0, 2) ...
// (1, 0) (1, 1) (1, 2) ...
// (2, 0) (2, 1) (2, 2) ...
// ...
//
// WARNING: THIS DESCRIBES THE MAGRATHEAN CONVENTION.  FUSION NUMBERS
// THE ROWS IN THE OPPOSITE DIRECTION (BOTTOM TO TOP)! Fusion
// numbering:
//
// (row, column) =
//
// (2, 0) (2, 1) (2, 2) ...
// (1, 0) (1, 1) (1, 2) ...
// (0, 0) (0, 1) (0, 2) ...

#ifndef COMMON_QTPACKET_QUADTREE_UTILS_H__
#define COMMON_QTPACKET_QUADTREE_UTILS_H__

#include <qtpacket/tree_utils.h>

namespace qtpacket {

class QuadtreeNumbering : public TreeNumbering {
 public:
  QuadtreeNumbering(int depth, bool mangle_second_row);
  virtual ~QuadtreeNumbering();

  static const int kDefaultDepth = 5;
  static const int kRootDepth = 4;

  // Compute the level and (x, y) position within the level for the given
  // subindex.
  void SubindexToLevelXY(int subindex, int *level, int *x, int *y) const;

  // Return the subindex for the given level and (x, y) position.
  int LevelXYToSubindex(int level, int x, int y) const;

  static std::uint64_t TraversalPathToGlobalNodeNumber(QuadtreePath path);
  static std::uint64_t TraversalPathToGlobalNodeNumber(const std::string &path) {
    return TraversalPathToGlobalNodeNumber(QuadtreePath(path));
  }
  static QuadtreePath GlobalNodeNumberToTraversalPath(std::uint64_t num);

  // Convert a traversal path to a quadset number, and a node within
  // the quadset.
  static void TraversalPathToQuadsetAndSubindex(QuadtreePath path,
                                                std::uint64_t *quadset_num,
                                                int *subindex);
  static void TraversalPathToQuadsetAndSubindex(const std::string &path,
                                                std::uint64_t *quadset_num,
                                                int *subindex) {
    TraversalPathToQuadsetAndSubindex(QuadtreePath(path),
                                      quadset_num,
                                      subindex);
  }

  static bool IsQuadsetRootLevel(std::uint32_t level);

  static QuadtreePath QuadsetAndSubindexToTraversalPath(std::uint64_t quadset_num,
                                                        int subindex);

  static void QuadsetAndSubindexToLevelRowColumn(std::uint64_t quadset_num,
                                                 int subindex,
                                                 int *level, int *row, int *col);

  // Return the number of nodes (subindex values) in a quadset
  static int NumNodes(std::uint64_t quadset_num);

  // Convert subindex to inorder numbering for a specified quadset
  static int QuadsetAndSubindexToInorder(std::uint64_t quadset_num,
                                         int subindex);

  // Get numbering for given quadset
  static const QuadtreeNumbering &Numbering(std::uint64_t quadset_num);

  // Given a tile's level, row and column, return its Maps tile name.
  static std::string LevelRowColumnToMapsTraversalPath(int level,
                                                       int row,
                                                       int col);

  // Given a tile's Maps name, find its level, row and column.
  static void MapsTraversalPathToLevelRowColumn(const std::string &path,
                                                int *level,
                                                int *row, int *col);

  static bool IsMapsTile(const std::string &key) {
    return !key.empty() && key[0] == 't';
  }

 private:

  void PrecomputeSubindexToLevelXY();

  struct LevelXY {
    int level;
    int x, y;
  };

  LevelXY *subindex_to_levelxy_;

  DISALLOW_COPY_AND_ASSIGN(QuadtreeNumbering);
};

}  // namespace qtpacket

#endif  // COMMON_QTPACKET_QUADTREE_UTILS_H__
