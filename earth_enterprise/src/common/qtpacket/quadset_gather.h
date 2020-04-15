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


#ifndef COMMON_QTPACKET_QUADSET_GATHER_H__
#define COMMON_QTPACKET_QUADSET_GATHER_H__

#include <merge/merge.h>
#include <quadtreepath.h>
#include <qtpacket/quadsetitem.h>

namespace qtpacket {

// QuadsetGather
//
// Gather inputs by quadset, in preparation for generation of quadtree
// packets.  Inputs are in standard order (preorder) by quadtree path.
// The output consists of quadset aggregations in *postorder*
// (i.e. for each quadset node, output subtrees in order 0, 1, 2, 3,
// followed by the parent). Postorder is used because all the
// information for a quadset is not known until all of the children
// have been processed.
//
// QuadsetGather takes as input a MergeSource (which may be a Merge)
// with input type ItemType.  QuadsetGather is a MergeSource with type
// QuadsetGroup<ItemType>.  The ItemType class must have a data
// accessor called qt_path() which returns a const QuadtreePath object
// reference, and a quadset_num() accessor which returns a std::uint64_t
// quadset number.

template <class ItemType> class QuadsetGather
    : public MergeSource<QuadsetGroup<ItemType> > {
 public:
  // The input source should be completely set up and started before
  // calling this constructor.
  QuadsetGather(const std::string &source_name,
                khTransferGuard<MergeSource<QuadsetItem<ItemType> > > source)
      : MergeSource<QuadsetGroup<ItemType> >(source_name),
        items_(kMaxQuadsetLevel+1),
        source_(source),
        source_finished_(false) {
    Advance();
  }
  ~QuadsetGather() {}

  // Advance runs until it can output a completed QuadsetGroup
  virtual bool Advance() {
    if (!source_) {
      throw khSimpleException("QuadsetGather::Advance: already Closed: ")
        << MergeSource<QuadsetGroup<ItemType> >::name();
    }

    while (!source_finished_) {
      QuadtreePath current_path = source_->Current().qt_path();

      // Output any items which precede current input
      OutputPredecessor(current_path);
      if (current_)
        return true;

      // Create or add to QuadsetGroup containing input
      std::uint64_t quadset_num;
      int subindex;
      QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(
          current_path, &quadset_num, &subindex);
      std::uint32_t qslevel = QuadsetLevel(current_path);
      assert(qslevel < items_.size());

      if (!items_[qslevel]) {
        items_.Assign(qslevel, new QuadsetGroup<ItemType>(quadset_num));
      } else {
        assert(items_[qslevel]->quadset_num() == quadset_num);
      }
      items_[qslevel]->Add(source_->Current());
      AddParents(current_path);
      source_finished_ = !source_->Advance();
    }

    FlushOutput();
    return current_;
  }

  virtual const QuadsetGroup<ItemType> &Current() const {
    if (current_) {
      return *current_;
    } else {
      throw khSimpleException("QuadsetGather::Current: "
                              "no output available: ")
                                << MergeSource<QuadsetGroup<ItemType> >::name();
    }
  }

  virtual void Close() {
    if (!std::uncaught_exception()) {
      if (current_) {
        notify(NFY_WARN, "QuadsetGather::Close: not all data processed");
        current_.clear();
      }
      source_->Close();
    }
  }
 private:
  static const std::uint32_t kLevelsPerQuadset = 4;
  static const std::uint32_t kMaxQuadsetLevel
  = QuadtreePath::kMaxLevel / kLevelsPerQuadset;

  // Compute the level of the quadset to which an item belongs. Note
  // that applying this function to the quadtree path of a
  // QuadsetGather object (other than root) will give incorrect
  // results. The path of a QuadsetGather object is actually the path
  // to the phantom root node, which resides in the parent, one level
  // less.
  inline std::uint32_t QuadsetLevel(const QuadtreePath &qt_path) {
    return qt_path.Level() / kLevelsPerQuadset;
  }

  // Flush any remaining QuadsetGroup items to output (current_).
  void FlushOutput() {
    // Use reverse order, since low index follows high index
    for (int pos = items_.size() - 1; pos >= 0; --pos) {
      if (items_[pos]) {
        current_ = TransferOwnership(items_.Take(pos));
        return;
      }
    }
    current_.clear();                   // no output
  }

  // Output any items which precede the specified path.
  void OutputPredecessor(const QuadtreePath qt_path) {
    for (int pos = items_.size() - 1; pos >= 0; --pos) {
      if (items_[pos]) {
        if (QuadtreePath::IsPostorder(items_[pos]->qt_last(), qt_path)) {
          current_ = TransferOwnership(items_.Take(pos));
        } else {
          current_.clear();             // no output
        }
        return;
      }
    }
    current_.clear();                   // no output
  }

  // AddParents - ensure that we have marked the existence of all
  // parents of the current node
  void AddParents(const QuadtreePath qt_path) {
    QuadtreePath parent_path(qt_path);
    while (parent_path.Level() > 0) {
      parent_path = parent_path.Parent();
      std::uint32_t quadset_level = QuadsetLevel(parent_path);

      std::uint64_t quadset_num;
      int subindex;
      QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(
          parent_path, &quadset_num, &subindex);

      if (!items_[quadset_level]) {
        items_.Assign(quadset_level,
                      new QuadsetGroup<ItemType>(quadset_num));
      }
      items_[quadset_level]->set_has_children_subindex(subindex);

      // Each quadset root node (except the root of the root) has a
      // phantom node which can contain a child flag.  Make sure this
      // gets set, since the path above resolves to the corresponding
      // leaf node of the parent quadset.
      if (quadset_level < kMaxQuadsetLevel
          && parent_path.Level() > 0
          && QuadtreeNumbering::IsQuadsetRootLevel(parent_path.Level())) {
        ++quadset_level;
        assert(items_[quadset_level]);
        items_[quadset_level]->set_has_children_subindex(0); // phantom root
      }
    }
  }

  // Vector of partially complete items at each level. If an item
  // exists at a given level, the items above it (lower indices) are
  // ancestors.
  khDeletingVector<QuadsetGroup<ItemType> > items_;

  khDeleteGuard<MergeSource<QuadsetItem<ItemType> > > source_;
  bool source_finished_;
  khDeleteGuard<QuadsetGroup<ItemType> > current_;
};

}  // namespace qtpacket

#endif  // COMMON_QTPACKET_QUADSET_GATHER_H__
