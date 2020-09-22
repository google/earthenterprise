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

// Traverser - traverse an index in preorder.  The Traverser class is
// defined as a MergeSource, so that data from multiple indices can be
// merged together.
//

#ifndef COMMON_GEINDEX_TRAVERSER_H__
#define COMMON_GEINDEX_TRAVERSER_H__

#include <cstdint>
#include <string>
#include <common/base/macros.h>
#include <merge/merge.h>
#include <quadtreepath.h>
#include <geindex/shared.h>
#include <khGuard.h>
#include <khEndian.h>
#include <cstdint>
#include <geFilePool.h>
#include <geindex/shared.h>
#include <geindex/Entries.h>
#include <geindex/IndexBundleReader.h>

namespace geindex {

// TraverserValue<Value> - define an extension to an value type which
// includes quadtree path and source file information.

template <class Value>
class TraverserValue : public Value {
 public:
  TraverserValue() {}
  TraverserValue(QuadtreePath qt_path,
                 const Value &value)
      : Value(value),
        qt_path_(qt_path) {
  }
  void set_qt_path(const QuadtreePath qt_path) { qt_path_ = qt_path; }
  inline QuadtreePath qt_path() const { return qt_path_; }
  bool operator>(const TraverserValue<Value> &other) const {
    return qt_path_ > other.qt_path_;
  }
 private:
  QuadtreePath qt_path_;
};

// Traverser<LoadedEntryBucket> - Traverse an index in preorder,
// returning index entries.
//
// Note: Use Traverser for ordered access.  Use Reader for random
// access to records.
//

template <class LoadedEntryBucket>
class Traverser
    : public
      MergeSource<TraverserValue<typename LoadedEntryBucket::SlotStorageType> > {
 public:
  static const size_t kMaxBucketLevel = (QuadtreePath::kMaxLevel
                                         + geindex::kQuadLevelsPerBucket - 1)
      / geindex::kQuadLevelsPerBucket;
  typedef typename LoadedEntryBucket::EntryType EntryType;
  typedef typename LoadedEntryBucket::SlotStorageType SlotStorageType;
  typedef TraverserValue<SlotStorageType> MergeEntry;

  Traverser(const std::string &merge_source_name,
            geFilePool &filePool,
            const std::string &index_path);
  virtual ~Traverser() {}
  virtual const MergeEntry &Current() const {
    return merge_root_.Current();
  }
  virtual bool Advance() {
    return merge_root_.Advance();
  }
  virtual void Close() {
    merge_root_.Close();
  }
  inline bool Finished() {
    return merge_root_.Finished();
  }
  inline bool Active() {
    return merge_root_.Active();
  }
  inline std::uint32_t GetPacketExtra(std::uint32_t packetfile_num) const {
    return reader_.header.GetPacketExtra(packetfile_num);
  }
  inline const IndexBundleReader& GetIndexBundleReader(void) const {
    return reader_;
  }
 private:
  // Current entry for merge
  Merge<MergeEntry> merge_root_;

  LittleEndianReadBuffer tmp_buf_;

  IndexBundleReader reader_;
  DISALLOW_COPY_AND_ASSIGN(Traverser);
};


// ****************************************************************************
// ***  AdaptingTraverserBase
// ***
// ***  Wrap a different traverser so it returns a TypedEntry
// ***  (or TypedProviderEntry) instead of
// ***  the more specific sub types of entry
// ***
// **   Typed Bucket must be UnifiedBucket or AllInfoBucket
// ****************************************************************************
template <class TypedBucket>
class AdaptingTraverserBase :
    public MergeSource<TraverserValue<typename TypedBucket::SlotStorageType> >
{
 public:
  typedef typename TypedBucket::EntryType        EntryType;
  typedef typename TypedBucket::SlotStorageType  SlotStorageType;
  typedef TraverserValue<SlotStorageType> MergeEntry;

  AdaptingTraverserBase(const std::string &merge_source_name,
                    TypedEntry::TypeEnum type) :
      MergeSource<MergeEntry>(merge_source_name),
      type_(type), curr_merge_entry_() { }
  virtual ~AdaptingTraverserBase() {}
  virtual const MergeEntry &Current() const {
    return curr_merge_entry_;
  }
  virtual bool Advance() = 0;
  virtual void Close() = 0;

 protected:
  TypedEntry::TypeEnum type_;
  MergeEntry curr_merge_entry_;

  DISALLOW_COPY_AND_ASSIGN(AdaptingTraverserBase);
};

template <class TypedBucket, class Traverser>
class AdaptingTraverser : public AdaptingTraverserBase<TypedBucket> {
 public:
  typedef typename AdaptingTraverserBase<TypedBucket>::EntryType EntryType;
  typedef typename AdaptingTraverserBase<TypedBucket>::SlotStorageType SlotStorageType;
  typedef typename AdaptingTraverserBase<TypedBucket>::MergeEntry MergeEntry;
  typedef typename Traverser::SlotStorageType               SubSlotStorageType;

  // channel id only respected for raster project indexes, It's ignored
  // for everything else
  AdaptingTraverser(const std::string &merge_source_name,
                    TypedEntry::TypeEnum type,
                    const khTransferGuard<Traverser> &sub_traverser,
                    std::uint32_t channel_id,
                    std::uint32_t version) :
      AdaptingTraverserBase<TypedBucket>(merge_source_name, type),
      sub_traverser_(sub_traverser),
      channel_id_(channel_id),
      version_(version)
  {
    PopulateCurrent();
  }
  virtual ~AdaptingTraverser() {}
  virtual bool Advance() {
    if (sub_traverser_->Advance()) {
      PopulateCurrent();
      return true;
    }
    return false;
  }
  virtual void Close() {
    sub_traverser_->Close();
  }
  inline bool Finished() {
    return sub_traverser_->Finished();
  }
  inline bool Active() {
    return sub_traverser_->Active();
  }

 private:
  inline void PopulateCurrent(void) {
    this->curr_merge_entry_.set_qt_path(sub_traverser_->Current().qt_path());
    PopulateTypedSlot(
        static_cast<SlotStorageType*>(&this->curr_merge_entry_), this->type_,
        static_cast<const SubSlotStorageType&>(sub_traverser_->Current()),
        static_cast<Traverser*>(sub_traverser_),
        channel_id_, version_);
  }

  khDeleteGuard<Traverser> sub_traverser_;
  std::uint32_t channel_id_;
  std::uint32_t version_;

  DISALLOW_COPY_AND_ASSIGN(AdaptingTraverser);
};


template <class Traverser>
class UnifiedAdaptingTraverser :
      public AdaptingTraverser<UnifiedBucket, Traverser> {
 public:
  // channel id only respected for raster project indexes, It's ignored
  // for everything else
  UnifiedAdaptingTraverser(const std::string &merge_source_name,
                           TypedEntry::TypeEnum type,
                           const khTransferGuard<Traverser> &sub_traverser,
                           std::uint32_t channel_id,
                           std::uint32_t version) :
      AdaptingTraverser<UnifiedBucket, Traverser>(
          merge_source_name, type, sub_traverser, channel_id, version)
  { }
};


template <class Traverser>
class AllInfoAdaptingTraverser :
      public AdaptingTraverser<AllInfoBucket, Traverser> {
 public:
  // channel id only respected for raster project indexes, It's ignored
  // for everything else
  AllInfoAdaptingTraverser(const std::string &merge_source_name,
                           TypedEntry::TypeEnum type,
                           const khTransferGuard<Traverser> &sub_traverser,
                           std::uint32_t channel_id) :
      AdaptingTraverser<AllInfoBucket, Traverser>(
          merge_source_name, type, sub_traverser, channel_id,
          0 /* unused version override */)
  { }
};




} // namespace geindex


#endif // COMMON_GEINDEX_TRAVERSER_H__
