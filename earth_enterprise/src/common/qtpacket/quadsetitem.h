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


#ifndef COMMON_QTPACKET_QUADSETITEM_H__
#define COMMON_QTPACKET_QUADSETITEM_H__

#include <common/base/macros.h>
#include <khGuard.h>
#include <quadtreepath.h>
#include <qtpacket/quadtree_utils.h>

namespace qtpacket {

// QuadsetItem
//
// Define container for quadset member node as it passes through merge
// and quadset gather process. Contained node data must be copyable,
// and QuadsetItem must be copyable.

template <class ItemType> class QuadsetItem {
 public:
  QuadsetItem(const QuadtreePath qt_path,
              const ItemType &item)
      : qt_path_(qt_path),
        item_(item) {
    QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(qt_path_,
                                                         &quadset_num_,
                                                         &subindex_);
  }
  ~QuadsetItem() {}

  inline const QuadtreePath &qt_path() const { return qt_path_; }
  inline std::uint64_t quadset_num() const { return quadset_num_; }
  inline int subindex() const { return subindex_; }
  inline const ItemType &item() const { return item_; }
  bool operator>(const QuadsetItem<ItemType> &other) const {
    return qt_path_ > other.qt_path_;
  }
 private:
  QuadtreePath qt_path_;                // path to packet itself
  std::uint64_t quadset_num_;                  // containing quadset
  int subindex_;
  ItemType item_;
};

// QuadsetGroup - define structure for nodes for a quadset. This is
// used to collect nodes which belong to the same quadset, and then
// output as a group. The > operator is defined for postorder (see
// quadset_gather.h).

template <class ItemType> class QuadsetGroup {
 public:
  // Define structure to hold all QuadsetItem objects for a given
  // subindex
  typedef std::vector<ItemType> NodeContents;

  explicit QuadsetGroup(std::uint64_t quadset_num)
      : quadset_num_(quadset_num),
        num_nodes_(QuadtreeNumbering::NumNodes(quadset_num)),
        qt_root_(
            QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(quadset_num,
                                                                 0)),
        qt_last_(quadset_num == 0 ?
                 qt_root_ :
                 qt_root_.Child(QuadtreePath::kChildCount - 1)),
        nodes_(num_nodes_),
        has_children_(num_nodes_) {
  }
  ~QuadsetGroup() {}

  inline size_t size() const { return nodes_.size(); }
  inline const QuadtreePath &qt_root() const { return qt_root_; }
  inline const QuadtreePath &qt_last() const { return qt_last_; }
  inline std::uint64_t quadset_num() const { return quadset_num_; }

  void Add(const QuadsetItem<ItemType> &new_node) {
    assert(new_node.quadset_num() == quadset_num_);
    size_t inorder =
      QuadtreeNumbering::QuadsetAndSubindexToInorder(quadset_num_,
                                                     new_node.subindex());
    assert(inorder < nodes_.size());
    nodes_[inorder].push_back(new_node.item());
  }
  const NodeContents &Node(size_t node_index) const {
    return nodes_.at(node_index);
  }

  size_t EntryCount() const {
    size_t entry_count = 0;
    for (size_t i = 0; i < nodes_.size(); ++i) {
      entry_count += nodes_[i].size();
    }
    return entry_count;
  }

  void set_has_children_subindex(int subindex) {
    set_has_children_inorder(
        QuadtreeNumbering::QuadsetAndSubindexToInorder(quadset_num_,
                                                       subindex));
  }
  void set_has_children_inorder(int inorder_index) {
    has_children_.at(inorder_index) = true;
  }
  inline bool has_children_inorder(int inorder_index) const {
    return has_children_.at(inorder_index);
  }

  // Postorder > operator (different than QuadtreePath >).
  bool operator>(const QuadsetGroup &other) const {
    return QuadtreePath::IsPostorder(other.qt_root_, qt_root_);
  }

  inline bool operator==(const QuadsetGroup &other) const {
    return qt_root_ == other.qt_root_;
  }
  inline bool operator!=(const QuadsetGroup &other) const {
    return qt_root_ != other.qt_root_;
  }
  inline bool operator<(const QuadsetGroup &other) const {
    return other > *this;
  }
 private:
  std::uint64_t quadset_num_;
  int num_nodes_;
  const QuadtreePath qt_root_;          // path to root of quadset
  const QuadtreePath qt_last_;          // path to last possible node (postorder)
  std::vector<NodeContents> nodes_;     // indexed by node inorder

  // Indexed by node inorder. True if node has children anywhere in
  // the quadtree.
  std::vector<bool> has_children_;

  DISALLOW_COPY_AND_ASSIGN(QuadsetGroup);
};

}  // namespace qtpacket

#endif  // COMMON_QTPACKET_QUADSETITEM_H__
