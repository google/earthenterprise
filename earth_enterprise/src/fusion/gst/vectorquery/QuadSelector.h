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

#ifndef FUSION_GST_QUADSELECTOR_H__
#define FUSION_GST_QUADSELECTOR_H__


#include "FilterGeoIndex.h"
class khProgressMeter;


namespace vectorquery {


class QuadSelectorBase
{
  friend class MinifiedQuadSelector;
 protected:
  khProgressMeter *progress_;

  // all the parallel arrays below have this same size
  const uint num_filters_;

  // holds information about whcih filters contribute to the target level
  // NOTE: this is a ref to a single vector that get's modified/restored
  // Copying would be too expensive for the dense data case
  // All Selectors in the heirarchy point to the same vector
  // This could be a parameter to SelectQuad, but storing it here reduces
  // the call cost of SelectQuad
  std::vector<FilterGeoIndex> &filters_;

  // which indexes of filters still have useful information
  // NOTE: this is a ref to a single vector that get's modified/restored
  // Copying would be too expensive for the dense data case
  // All Selectors in the heirarchy point to the same vector
  // This could be a parameter to SelectQuad, but storing it here reduces
  // the call cost of SelectQuad
  std::vector<bool> &use_set_;

  // Which indexes in use_filters_ have I cleared while processing a tile
  // This could be a local variable in SelectQuad, but the cost of allocating
  // and initializing the array each time gets expensive.
  std::vector<bool> cleared_set_;
  bool something_cleared_;
  bool something_left_;

  // wanted coverage for my level
  khLevelCoverage cov_;

  // Try to clear some sets out of the use_set_ list
  // This is called from inside tight loops. Mark it as inline and leave it
  // in the header with the hopes that the compiler is realy smart enough to
  // inline it.
  inline void TryToClearSets(uint32 row, uint32 col) {
    for (uint i = 0; i < num_filters_; ++i) {
      if (use_set_[i]) {
        if (filters_[i].geo_index_->
            GetEstimatedPresence(khTileAddr(cov_.level, row, col))) {
          something_left_ = true;
        } else {
          something_cleared_ = true;
          cleared_set_[i] = true;
          use_set_[i] = false;
        }
      }
    }
  }

  // Restore the sets that I cleared
  void RestoreCleared(void);

 public:
  inline bool Contains(uint32 row, uint32 col) {
    return cov_.extents.ContainsRowCol(row, col);
  }
  virtual ~QuadSelectorBase(void) { }
  QuadSelectorBase(khProgressMeter *progress,
                   std::vector<FilterGeoIndex> &filters,
                   std::vector<bool> &use_set,
                   const khLevelCoverage &cov);

  virtual void SelectQuad(uint32 row, uint32 col) = 0;
};


class MinifiedQuadSelector : public QuadSelectorBase
{
 protected:
  // Holds original BuildSets when I split one. When I'm done processing
  // I restore these to their original locations
  // This could be a local variable in SelectQuad, but the cost of allocating
  // and initializing the array each time gets expensive.
  std::vector<gstGeoIndexHandle> split_set_;
  bool something_split_;

  // wanted coverage at target level
  khLevelCoverage target_cov_;

  QuadSelectorBase *next_;

  // Split all BuildSets that have reach the max level of their index
  // This is called from inside tight loops. Mark it as inline and leave it
  // in the header with the hopes that the compiler is realy smart enough to
  // inline it.
  inline void TryToSplitSets(uint32 row, uint32 col) {
    for (uint i = 0; i < num_filters_; ++i) {
      if (use_set_[i]) {
        if (cov_.level == filters_[i].geo_index_->MaxLevel()) {
          SplitSet(i, row, col);
        }
      }
    }
  }

  void SplitSet(uint i, uint32 row, uint32 col);
  void RestoreSplit(void);

 public:
  MinifiedQuadSelector(QuadSelectorBase *next,
                       const khLevelCoverage &cov,
                       const khLevelCoverage &target_cov);

  virtual void SelectQuad(uint32 row, uint32 col);
};


class FilterGeoIndexManager;

class FullResQuadSelectorPreStorage {
 public:
  std::vector<bool> use_set_storage_;

  FullResQuadSelectorPreStorage(uint num_filters)
      : use_set_storage_(num_filters, true) { }
};
class FullResQuadSelector : private FullResQuadSelectorPreStorage,
                            public QuadSelectorBase
{
 public:
  FullResQuadSelector(khProgressMeter *progress,
                      FilterGeoIndexManager &manager,
                      std::vector<FilterGeoIndex> &filters,
                      const khLevelCoverage &cov);

  virtual void SelectQuad(uint32 row, uint32 col);

 private:
  FilterGeoIndexManager &manager_;
};


} // namespace vector query


#endif // FUSION_GST_QUADSELECTOR_H__
