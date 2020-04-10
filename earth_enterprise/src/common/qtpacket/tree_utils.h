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
// tree nodes in Keyhole.
//

// Nodes have two numbering schemes:
//
// 1) "Subindex".  This numbering starts at the top of the tree
// and goes left-to-right across each level, like this:
//
//                    0
//                 /     \                           .
//               1  86 171 256
//            /     \                                .
//          2  3  4  5 ...
//        /   \                                      .
//       6 7 8 9  ...
//
// Notice that the second row is weird in that it's not left-to-right
// order.  HOWEVER, the root node in Keyhole is special in that it
// doesn't have this weird ordering.  It looks like this:
//
//                    0
//                 /     \                           .
//               1  2  3  4
//            /     \                                .
//          5  6  7  8 ...
//       /     \                                     .
//     21 22 23 24  ...
//
// The mangling of the second row is controlled by a parameter to the
// constructor.
//
// 2) "Inorder". The order of this node in an inorder traversal.
//
//                    0
//                 /     \                           .
//               1  86 171 256
//            /     \                                .
//           2 23 44 65  ...
//         /   \                                     .
//       3 8 13 18  ...
//
//
// Clients make requests for nodes using subindex order.  However, in
// a quadtree packet, nodes are stored in inorder.
//
//
// Nodes are also identified by traversal paths.  A traversal path is
// a sequence of integers that tell how to descend from the root of
// the tree.  For example:
//
// ()       the root node
// (1, 2)   Take child #3 of child #2 of the root node
//
// A traversal path is represented by a string, where each byte is one
// level of the path, or by a QuadtreePath object.
//
//
// This class contains some precomputed lookup tables, which would get
// very large if the tree is deep (size = O(branching_factor **
// depth)).  For keyhole, these trees are only 5 levels deep at most.
//

#ifndef COMMON_QTPACKET_TREE_UTILS_H__
#define COMMON_QTPACKET_TREE_UTILS_H__

#include <cstdint>
#include <string>
#include <common/base/macros.h>
#include <quadtreepath.h>

namespace qtpacket {

class TreeNumbering {
 public:
  TreeNumbering(int branching_factor, int depth, bool mangle_second_row);
  virtual ~TreeNumbering();
  TreeNumbering(const TreeNumbering&) = delete;
  TreeNumbering(TreeNumbering&&) = delete;
  TreeNumbering& operator=(const TreeNumbering&) = delete;
  TreeNumbering& operator=(TreeNumbering&&) = delete;

  // Return the total number of nodes in the tree
  int num_nodes() const {
    return num_nodes_;
  }

  int depth() const {
    return depth_;
  }

  int branching_factor() const {
    return branching_factor_;
  }

  // Return the inorder numbering for the given subindex numbering.
  int SubindexToInorder(int subindex) const {
    assert(InRange(subindex));
    return nodes_[subindex].subindex_to_inorder;
  }

  // Return the subindex numbering for the given inorder numbering.
  int InorderToSubindex(int inorder) const {
    assert(InRange(inorder));
    return nodes_[inorder].inorder_to_subindex;
  }

  // Return the inorder numbering for the given traversal path.
  int TraversalPathToInorder(const QuadtreePath path) const;
  int TraversalPathToInorder(const std::string &path) const {
    return TraversalPathToInorder(QuadtreePath(path));
  }

  // Return the subindex numbering for the given traversal path.
  int TraversalPathToSubindex(const QuadtreePath path) const;
  int TraversalPathToSubindex(const std::string &path) const {
    return TraversalPathToSubindex(QuadtreePath(path));
  }

  // Return the traversal path to the given inorder node.
  QuadtreePath InorderToTraversalPath(int inorder) const;

  QuadtreePath SubindexToTraversalPath(int subindex) const;

  // If this is the subindex of an interior node, fill in indices with
  // the subindex numbers of its child nodes and return true.  indices
  // must be at least branching_factor in length.
  //
  // If this is a leaf node, return false.
  bool GetChildrenSubindex(int subindex, int *indices) const;

  bool GetChildrenInorder(int inorder, int *indices) const;

  // Return the level of the node with the given subindex
  int GetLevelSubindex(int subindex) const {
    return GetLevelInorder(SubindexToInorder(subindex));
  }

  int GetLevelInorder(int inorder) const {
    assert(InRange(inorder));
    return nodes_[inorder].inorder_to_level;
  }

  // Return the index of this node's parent, or -1 if this node is the root
  int GetParentSubindex(int subindex) const;

  int GetParentInorder(int inorder) const;

 protected:

  bool InRange(int num) const {
    return (0 <= num) && (num < num_nodes_);
  }

 private:

  // base is added to every subindex during the traversal.
  // level is the level of the tree we're at (0 = top)
  // offset is the left-to-right offset of this node in this level of the tree.
  // left_index is the subindex of the leftmost node in this level.
  // num counts the nth node in the inorder traversal.
  void PrecomputeSubindexToInorder(int base,
                                   int level, int offset, int left_index,
                                   int *num);

  void PrecomputeInorderToParent();

  // Return the number of nodes in the tree at levels <= the given level
  int NodesAtLevels(int level) const;

  void PrecomputeNodesAtLevels();

  int depth_;
  int branching_factor_;
  int num_nodes_;
  struct NodeInfo {
    int subindex_to_inorder;
    int inorder_to_subindex;
    int inorder_to_level;
    int inorder_to_parent;
  };

  NodeInfo *nodes_;

  // Store the number of nodes in the tree at levels <= the given level
  int *nodes_at_levels_;
};

}  // namespace qtpacket

#endif  // COMMON_QTPACKET_TREE_UTILS_H__
