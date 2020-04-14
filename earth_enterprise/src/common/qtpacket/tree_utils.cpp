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
// tree nodes in Keyhole.
//

#include "tree_utils.h"

namespace qtpacket {

TreeNumbering::TreeNumbering(int branching_factor, int depth,
                             bool mangle_second_row)
    : depth_(depth),
      branching_factor_(branching_factor) {
  // Precompute the number of nodes at each level
  nodes_at_levels_ = new int[depth_ + 1];
  PrecomputeNodesAtLevels();

  // Compute number of nodes in the tree
  num_nodes_ = NodesAtLevels(depth);
  nodes_ = new NodeInfo[num_nodes_];

  // The second row only affects the inorder <-> subindex conversions.
  // We precompute this conversion table here.
  if (mangle_second_row) {
    // Special-case the nasty second row of subindex numbering
    if (depth > 0) {
      nodes_[0].subindex_to_inorder = 0;
      nodes_[0].inorder_to_level = 0;
      if (depth > 1) {
        int num = 1;
        // Inorder traversal of each child of the second row
        for (int i = 0; i < branching_factor; ++i)
          PrecomputeSubindexToInorder(num, 1, 0, 0, &num);
      }
    }
  } else {
    int num = 0;
    PrecomputeSubindexToInorder(0, 0, 0, 0, &num);
  }

  // Reverse the table
  for (int i = 0; i < num_nodes_; ++i)
    nodes_[nodes_[i].subindex_to_inorder].inorder_to_subindex = i;

  PrecomputeInorderToParent();
}

TreeNumbering::~TreeNumbering() {
  delete [] nodes_;
  delete [] nodes_at_levels_;
}

int TreeNumbering::TraversalPathToInorder(const QuadtreePath path) const {
  assert(static_cast<int>(path.Level()) < depth_);

  int index = 0;
  for (std::uint32_t i = 0; i < path.Level(); ++i) {
    assert(path[i] < QuadtreePath::kChildCount);

    index += 1 + path[i] * NodesAtLevels(depth_ - i - 1);
  }
  return index;
}

int TreeNumbering::TraversalPathToSubindex(const QuadtreePath path) const {
  return InorderToSubindex(TraversalPathToInorder(path));
}

QuadtreePath TreeNumbering::InorderToTraversalPath(int inorder) const {
  assert(InRange(inorder));

  // Walk down the tree, subtracting off the size of the child subtree
  // at each step.
  QuadtreePath path;
  int level = 1;
  while (inorder > 0) {
    int nodes_below = NodesAtLevels(depth_ - level);
    int step = (inorder - 1) / nodes_below;
    path = path.Child(step);
    inorder = inorder - step * nodes_below - 1;
    ++level;
  }

  return path;
}

QuadtreePath TreeNumbering::SubindexToTraversalPath(int subindex) const {
  return InorderToTraversalPath(SubindexToInorder(subindex));
}

bool TreeNumbering::GetChildrenSubindex(int subindex, int *indices) const {
  if (!GetChildrenInorder(SubindexToInorder(subindex), indices))
    return false;

  // Convert children to subindex
  for (int i = 0; i < branching_factor_; ++i)
    indices[i] = InorderToSubindex(indices[i]);
  return true;
}

bool TreeNumbering::GetChildrenInorder(int inorder, int *indices) const {
  assert(InRange(inorder));

  // At a leaf node?
  int level = GetLevelInorder(inorder);
  if (level == depth_ - 1)
    return false;

  // First child is inorder + 1.  The others differ by the number of
  // nodes in the subtree rooted at a child.
  int num_nodes_in_subtree = NodesAtLevels(depth_ - level - 1);

  for (int i = 0; i < branching_factor_; ++i)
    indices[i] = inorder + 1 + i * num_nodes_in_subtree;
  return true;
}

int TreeNumbering::GetParentSubindex(int subindex) const {
  int parent = GetParentInorder(SubindexToInorder(subindex));
  if (-1 == parent)
    return parent;

  return InorderToSubindex(parent);
}

int TreeNumbering::GetParentInorder(int inorder) const {
  assert(InRange(inorder));
  return nodes_[inorder].inorder_to_parent;
}

void TreeNumbering::PrecomputeSubindexToInorder(int base,
                                                int level, int offset,
                                                int left_index, int *num) {
  nodes_[base + left_index + offset].subindex_to_inorder = *num;
  nodes_[*num].inorder_to_level = level;
  *num = *num + 1;
  // Descend to children?
  if (level < depth_ - 1) {
    for (int i = 0; i < branching_factor_; ++i)
      PrecomputeSubindexToInorder(base,
                                  level + 1,
                                  offset * branching_factor_ + i,
                                  left_index * branching_factor_ + 1,
                                  num);
  }
}

void TreeNumbering::PrecomputeInorderToParent() {
  // Deal with root
  if (num_nodes_ > 0)
    nodes_[0].inorder_to_parent = -1;

  for (int i = 1; i < num_nodes_; ++i) {
    QuadtreePath path = InorderToTraversalPath(i);
    nodes_[i].inorder_to_parent =
      TraversalPathToInorder(path.Parent());
  }
}

void TreeNumbering::PrecomputeNodesAtLevels() {
  int i = 0;
  int num = 0;
  int num_at_bottom = 1;
  do {
    nodes_at_levels_[i] = num;
    num += num_at_bottom;
    num_at_bottom *= branching_factor_;
    ++i;
  } while (i <= depth_);
}

int TreeNumbering::NodesAtLevels(int level) const {
  return nodes_at_levels_[level];
}

}  // namespace qtpacket
