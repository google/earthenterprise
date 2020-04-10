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

// Refactored gstGeoIndex internal data representation (optimization of memory
// usage): store reference to full FeatureHandle array in all geoindex-es and
// use index of array to address FeatureHandle-s.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOINDEX_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOINDEX_H_

#include <vector>
#include <deque>
#include <set>
#include <string>

#include "common/base/macros.h"
#include "fusion/gst/gstBBox.h"
#include "common/khRefCounter.h"
#include "common/khPresenceMask.h"
#include "common/khCoverageMask.h"

class gstSharedSourceImpl;
typedef khRefGuard<gstSharedSourceImpl> gstSharedSource;

class gstGeoIndexImpl;
typedef khRefGuard<gstGeoIndexImpl> gstGeoIndexHandle;

class gstGeoIndexImpl : public khRefCounter {
 public:
  static khRefGuard<gstGeoIndexImpl> Create() {
    return khRefGuardFromNew(new gstGeoIndexImpl());
  }
  static gstGeoIndexHandle Load(const std::string &select_file,
                                const gstSharedSource &source,
                                const khTilespace &tilespace,
                                const double oversize_factor,
                                const unsigned int target_level);

  void Reset();

  // Adds a feature id and it's corresponding bounding box.
  void Insert(int feature_id, const gstBBox& bounding_box);
  void Finalize();

  void Intersect(const gstBBox& b, std::vector<int>* match_list,
                 std::vector<gstBBox>* index_boxes = NULL);
  void SelectAll(std::vector<int>* list);


  bool ReadFile(const std::string& path, int source_id);
  bool WriteFile(const std::string& path);

  const gstBBox& GetBoundingBox() const { return bounding_box_; }
  int GetCount() const { return box_index_list_.size(); }
  const khLevelCoverage& GetCoverage(void) const { return coverage_; }

  // false return is definitive
  // true return may just be an estimate
  inline bool GetEstimatedPresence(const khTileAddr &addr) const {
    // Note: if it's not true then it means that we traverse odd
    // tiles (our quadtree partitioning is not effective).
    assert(presence_mask_);
    return presence_mask_->GetEstimatedPresence(addr);
  }

  inline void SetPresence(khTileAddr addr, bool present) {
    assert(presence_mask_);
    // Note: The vector data is fused starting from level 0.
    // The presence mask created here depends on feature distribution
    // statistics and it can happen that a start level of the presence mask
    // is not 0. Currently, the function is only called in 3d fuse pipeline,
    // and seems it never happens, but I encountered it trying to integrate
    // the qt partitioning optimization into 2d fuse pipeline. So we check
    // whether addr.level is valid.
    if (presence_mask_->ValidLevel(addr.level)) {
      presence_mask_->SetPresence(addr, present);
    }
  }

  // Sets that specific tile (addr) is completely covered by specific feature
  // (feature_id).
  inline void SetCovered(const khTileAddr &addr,
                          int feature_id) {
    assert(coverage_mask_);
    if (coverage_mask_->ValidLevel(addr.level)) {
      coverage_mask_->SetCovered(addr, feature_id);
    }
  }

  // Returns whether specific tile is completely covered with any polygonal
  // feature and a list of feature IDs covering that tile.
  inline bool GetCovered(const khTileAddr &addr,
                         std::set<int> *feature_id_set) const {
    assert(coverage_mask_);
    return coverage_mask_->GetCovered(addr, feature_id_set);
  }


  inline unsigned int MaxLevel(void) const { return coverage_.level; }
  gstGeoIndexHandle SplitCell(std::uint32_t row, std::uint32_t col,
                              const khLevelCoverage &targetCov);

  typedef std::deque< unsigned int>  FeatureBucket;
  typedef FeatureBucket::iterator FeatureBucketIterator;
  typedef FeatureBucket::const_iterator FeatureBucketConstIterator;

  const FeatureBucket* GetBucket(std::uint32_t row, std::uint32_t col) const;
  void PopulateBucket(const khExtents<std::uint32_t> &extents,
                      FeatureBucket *bucket) const;

  void GetFeatureIdsFromBucket(std::uint32_t row, std::uint32_t col,
                               std::vector<int> &ids) const;
  void GetFeatureIdsFromBuckets(const khExtents<std::uint32_t> &extents,
                                std::vector<int> &ids) const;

  struct FeatureHandle {
    FeatureHandle(int fid, const gstBBox& b)
        : feature_id(fid), bounding_box(b) {}
    bool operator<(const FeatureHandle& fh) const {
      return (feature_id < fh.feature_id);
    }
    int feature_id;
    gstBBox bounding_box;
  };

  struct FeatureList;
  typedef khRefGuard<FeatureList> FeatureListHandle;

  class FeatureList : public khRefCounter {
   public:
    static khRefGuard<FeatureList> Create() {
      return khRefGuardFromNew(new FeatureList());
    }

    inline size_t size() const {
      return feature_handles_.size();
    }

    inline void push_back(const FeatureHandle& x) {
      feature_handles_.push_back(x);
    }

    inline const FeatureHandle& operator[](unsigned int n) const {
      return feature_handles_[n];
    }

    inline FeatureHandle& operator[](unsigned int n) {
      return feature_handles_[n];
    }

   private:
    FeatureList() : feature_handles_() {}

    std::deque<FeatureHandle> feature_handles_;
    DISALLOW_COPY_AND_ASSIGN(FeatureList);
  };

  const FeatureList& GetFeatureList(void) const {
    assert(box_list_);
    return (*box_list_);
  }

 private:
  friend class khRefGuard<gstGeoIndexImpl>;

  typedef std::vector<FeatureBucket> FeatureGrid;
  typedef FeatureGrid::iterator FeatureGridIterator;

  gstGeoIndexImpl();
  gstGeoIndexImpl(const khLevelCoverage &cov,
                  const khTilespace &tilespace,
                  double oversize_factor);
  gstGeoIndexImpl(const std::string &select_file,
                  const gstSharedSource &source,
                  const khTilespace &tilespace,
                  const double oversize_factor,
                  const unsigned int target_level);

  void Init();

  // will throw - use ReadFile above if you don't want exceptions
  void ThrowingReadFile(const std::string& path, int source_id);

  // Adds a feature index and it's corresponding bounding box.
  // It's used in SplitCell() to create a new Index and fill it with features
  // based on an existing Index.
  void InsertIndex(unsigned int idx);

  const khTilespace& tilespace_;
  const double oversize_factor_;

  FeatureGrid grid_;
  FeatureListHandle box_list_;
  typedef std::deque<std::uint32_t> BoxIndexList;
  BoxIndexList box_index_list_;

  gstBBox bounding_box_;
  khLevelCoverage coverage_;
  khDeleteGuard<khPresenceMask> presence_mask_;
  // The coverage mask keeps information about completely covered tiles:
  // whether tile is completely covered and list of features covering it.
  // It is built/updated while fusing every level and used for optimization
  // of processing of completely covered tiles.
  khDeleteGuard<khCoverageMask> coverage_mask_;
  const unsigned int target_level_;

  DISALLOW_COPY_AND_ASSIGN(gstGeoIndexImpl);
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOINDEX_H_
