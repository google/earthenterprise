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

#include "geindex/Traverser.h"

namespace geindex {

//-----------------------------------------------------------------------------
// MergeEntryBucket
//
// Merge source to supply entries from a single LoadedEntryBucket.
// Walks sequentially through the entries in the LoadedEntryBucket,
// returning the next one at each Advance().  Note that the entriy
// slots must be stored in preorder
//
//-----------------------------------------------------------------------------

template <class LoadedEntryBucket>
class MergeEntryBucket : public
  MergeSource<TraverserValue<typename LoadedEntryBucket::SlotStorageType> > {
 public:
  typedef typename LoadedEntryBucket::EntryType EntryType;
  typedef typename LoadedEntryBucket::SlotStorageType SlotStorageType;
  MergeEntryBucket(const BucketPath &path,
                   const EntryBucketAddr &addr,
                   IndexBundleReader &bundle,
                   EndianReadBuffer &tmpBuf)
      : MergeSource<TraverserValue<SlotStorageType> >(bundle.abs_path()
                                                      + ":E" + path.AsString()),
        entry_bucket_(addr, bundle, tmpBuf),
        bucket_path_(path),
        slot_path_(path),
        level_limit_(std::min(path.Level() + kQuadLevelsPerBucket - 1,
                              QuadtreePath::kMaxLevel)),
        is_valid_(false),
        next_slot_(0) {
    assert(0 == path.Level() % kQuadLevelsPerBucket);
    if (!Advance()) {
      throw khSimpleException(
          "MergeEntryBucket::MergeEntryBucket: entry bucket is empty");
    }
  }

  ~MergeEntryBucket() {}

  virtual const TraverserValue<SlotStorageType> &Current() const {
    assert(is_valid_);
    return current_entry_;
  }

  virtual bool Advance() {
    while (next_slot_ < kEntrySlotsPerBucket
           &&  entry_bucket_.IsSlotEmpty(next_slot_)) {
      AdvanceSlotIndex();
    }
    is_valid_ = next_slot_ < kEntrySlotsPerBucket;
    if (is_valid_) {
      current_entry_ =
        TraverserValue<SlotStorageType>(slot_path_,
                                        entry_bucket_.EntrySlot(next_slot_));
      AdvanceSlotIndex();
    }
    return is_valid_;
  }

  virtual void Close() {
  }
 private:
  inline void AdvanceSlotIndex() {
    slot_path_.Advance(level_limit_);
    ++next_slot_;
  }

  LoadedEntryBucket entry_bucket_;
  TraverserValue<SlotStorageType> current_entry_;
  const BucketPath bucket_path_;
  QuadtreePath slot_path_;
  const std::uint32_t level_limit_;
  bool is_valid_;
  BucketEntrySlotType next_slot_;
  DISALLOW_COPY_AND_ASSIGN(MergeEntryBucket<LoadedEntryBucket>);
};

//-----------------------------------------------------------------------------
// MergeChildBucket
//
// Merge source to supply entries from a single LoadedChildBucket.
// Walks through the child and entry addresses of the
// LoadedChildBucket in parallel, merging the entries from the entry
// bucket with the (recursively) generated entries below the child
// bucket.
//
//-----------------------------------------------------------------------------

template <class LoadedEntryBucket>
class MergeChildBucket : public
  MergeSource<TraverserValue<typename LoadedEntryBucket::SlotStorageType> > {
 public:
  typedef typename LoadedEntryBucket::EntryType EntryType;
  typedef typename LoadedEntryBucket::SlotStorageType SlotStorageType;
  MergeChildBucket(const BucketPath &path,
                   const ChildBucketAddr &addr,
                   IndexBundleReader &bundle,
                   EndianReadBuffer &tmpBuf)
      : MergeSource<TraverserValue<SlotStorageType> >(bundle.abs_path()
                                                      + ":C" + path.AsString()),
        child_bucket_(addr, bundle, tmpBuf),
        reader_(bundle),
        tmp_buf_(tmpBuf),
        bucket_path_(path),
        slot_path_(path + LoadedChildBucket::kFirstSubPath),
        is_valid_(false),
        next_slot_(0) {
    assert(0 == path.Level() % kQuadLevelsPerBucket);
    if (!Advance()) {
      throw khSimpleException(
          "MergeChildBucket::MergeChildBucket: child bucket is empty");
    }
  }

  ~MergeChildBucket() {}

  virtual const TraverserValue<SlotStorageType> &Current() const {
    assert(sub_merge_);
    return sub_merge_->Current();
  }

  virtual bool Advance() {
    // Advance existing merge until empty
    if (sub_merge_  &&  sub_merge_->Advance()) {
      return true;
    } else {
      // Advance index to next non-empty sub-path
      sub_merge_.clear();                   // delete old merge, if any
      if (next_slot_ >= kChildAddrsPerBucket)
        return false;
      khDeleteGuard<Merge<TraverserValue<SlotStorageType> > > new_merge;
      do {
        // If there is an entry slot at this index, add it to the merge
        if (child_bucket_.entryAddrs[next_slot_]) {
          new_merge = TransferOwnership(CreateSubMerge());
          new_merge->AddSource(
              TransferOwnership(
                  new MergeEntryBucket<LoadedEntryBucket>(
                      slot_path_,
                      child_bucket_.entryAddrs[next_slot_],
                      reader_,
                      tmp_buf_)));
        }

        // If there is a child slot at this index, add it to the merge
        if (child_bucket_.childAddrs[next_slot_]) {
          if (!new_merge) {
            new_merge = TransferOwnership(CreateSubMerge());
          }
          new_merge->AddSource(
              TransferOwnership(
                  new MergeChildBucket<LoadedEntryBucket>(
                      slot_path_,
                      child_bucket_.childAddrs[next_slot_],
                      reader_,
                      tmp_buf_)));
        }

      } while(!new_merge && AdvanceSlotIndex());
      sub_merge_ = TransferOwnership(new_merge);
      if (sub_merge_) {
        AdvanceSlotIndex();             // point to slot after current
        sub_merge_->Start();
        return true;
      } else {
        return false;
      }
    }
  }

  virtual void Close() {
  }
 private:
  static const QuadtreePath kFirstSubPath;

  Merge<TraverserValue<SlotStorageType> > *CreateSubMerge() const {
    return new Merge<TraverserValue<SlotStorageType> >
      (reader_.abs_path() + ":M" + slot_path_.AsString());
  }

  inline bool AdvanceSlotIndex() {
    slot_path_.AdvanceInLevel();
    return ++next_slot_ < kChildAddrsPerBucket;
  }

  LoadedChildBucket child_bucket_;

  // Merge of descendants of current slot index
  khDeleteGuard<Merge<TraverserValue<SlotStorageType> > > sub_merge_;

  IndexBundleReader &reader_;
  EndianReadBuffer &tmp_buf_;
  const BucketPath bucket_path_;
  QuadtreePath slot_path_;
  bool is_valid_;
  std::uint32_t next_slot_;
  DISALLOW_COPY_AND_ASSIGN(MergeChildBucket<LoadedEntryBucket>);
};

//------------------------------------------------------------------------
// Traverser<LoadedEntryBucket>
//------------------------------------------------------------------------

template <class LoadedEntryBucket>
Traverser<LoadedEntryBucket>::Traverser(const std::string &merge_source_name,
                                        geFilePool &filePool,
                                        const std::string &index_path)
    : MergeSource<MergeEntry>(merge_source_name),
      merge_root_(merge_source_name + ":merge_root"),
      tmp_buf_(),
      reader_(filePool, index_path) {
  // Read root child bucket and add to merge
  if (reader_.header.rootChildAddr) {
    merge_root_.AddSource(
        TransferOwnership(
            new MergeChildBucket<LoadedEntryBucket>(
                QuadtreePath(""),
                reader_.header.rootChildAddr,
                reader_,
                tmp_buf_)));
  }

  // Read root entry bucket and add to merge
  if (reader_.header.rootEntryAddr) {
    merge_root_.AddSource(
        TransferOwnership(
            new MergeEntryBucket<LoadedEntryBucket>(
                QuadtreePath(""),
                reader_.header.rootEntryAddr,
                reader_,
                tmp_buf_)));
  }

  merge_root_.Start();
}

//------------------------------------------------------------------------
// MergeEntryBucket<LoadedEntryBucket> explicit instatiations
//------------------------------------------------------------------------
template class MergeEntryBucket<CombinedTmeshBucket>;
template class MergeEntryBucket<BlendBucket>;
template class MergeEntryBucket<VectorBucket>;
template class MergeEntryBucket<UnifiedBucket>;

//------------------------------------------------------------------------
// Traverser<LoadedEntryBucket> explicit instatiations
//------------------------------------------------------------------------
template class Traverser<CombinedTmeshBucket>;
template class Traverser<BlendBucket>;
template class Traverser<VectorBucket>;
template class Traverser<UnifiedBucket>;

} // namespace geindex
