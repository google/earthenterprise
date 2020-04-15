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

#include "QuadSelector.h"
#include <khProgressMeter.h>
#include <notify.h>

namespace vectorquery {

// ****************************************************************************
// ***  QuadSelectorBase
// ****************************************************************************
QuadSelectorBase::QuadSelectorBase(khProgressMeter *progress,
                                   std::vector<FilterGeoIndex> &filters,
                                   std::vector<bool> &use_set,
                                   const khLevelCoverage &cov) :
    progress_(progress),
    num_filters_(filters.size()),
    filters_(filters),
    use_set_(use_set),
    cleared_set_(num_filters_, false),
    something_cleared_(false),
    something_left_(false),
    cov_(cov)
{
  notify(NFY_DEBUG, "Selector lev:%u, row:%u->%u, col:%u->%u, tiles:%llu",
         cov_.level, cov_.extents.beginRow(), cov_.extents.endRow(),
         cov_.extents.beginCol(), cov_.extents.endCol(),
         static_cast<long long unsigned>(cov_.NumTiles()));
}

void QuadSelectorBase::RestoreCleared(void) {
  for (unsigned int i = 0; i < num_filters_; ++i) {
    if (cleared_set_[i]) {
      cleared_set_[i] = false;
      use_set_[i] = true;
    }
  }
  something_cleared_ = false;
}

// ****************************************************************************
// ***  MinifiedQuadSelector
// ****************************************************************************
void MinifiedQuadSelector::SelectQuad(std::uint32_t row, std::uint32_t col) {
  assert(Contains(row, col));

  TryToClearSets(row, col);

  if (!something_left_) {
    // The index tells us we have nothing to contribute. But we have counted
    // the tiles below this as work that needs to be done. Calculate how
    // many tiles we're going to skip and mark them as skipped.
#if 0
    if ((cov_.level == 5) && (row == 20) && (col == 16)) {
      notify(NFY_WARN, "========== (lrc) 5,20,16:  Nothing Left");
    }
#endif

    // figure out this tile's coverage at the target level
    khLevelCoverage this_target_cov =
      khTileAddr(cov_.level, row, col).MagnifiedToLevel(target_cov_.level);

    // interset that with the targetCoverage (what we really want to do)
    khExtents<std::uint32_t> skip_extents =
      khExtents<std::uint32_t>::Intersection(this_target_cov.extents,
                                      target_cov_.extents);

    // now we know how many we're going to skip
    std::uint64_t numSkipped = std::uint64_t(skip_extents.numRows()) *
                        std::uint64_t(skip_extents.numCols());

    progress_->incrementDone(numSkipped);
#if 0
    notify(NFY_VERBOSE, "MinifiedSelector pruning (lrc) %u,%u,%u",
           cov_.level, row, col);
#endif
  } else {
#if 0
    if ((cov_.level == 5) && (row == 20) && (col == 16)) {
      notify(NFY_WARN, "========== (lrc) 5,20,16:  Calling TryToSplitSets");
    }
#endif
    TryToSplitSets(row, col);

    // for each of the four source tiles / quadrants
    for(unsigned int quad = 0; quad < 4; ++quad) {
      // magnify the dest row/col/quad to get row/col for the next level
      std::uint32_t nextRow = 0;
      std::uint32_t nextCol = 0;
      QuadtreePath::MagnifyQuadAddr(row, col, quad, nextRow, nextCol);

      // check if quad exists the next row down
      if (next_->Contains(nextRow, nextCol)) {
        next_->SelectQuad(nextRow, nextCol);
#if 0
      } else if ((cov_.level+1 == 5) && (nextRow == 20) &&
                 (nextCol == 16)) {
        notify(NFY_WARN, "========== (lrc) 5,20,16:  Not in coverage");
#endif
      }
    }

    if (something_split_) {
      RestoreSplit();
    }

    something_left_ = false;
  }

  if (something_cleared_) {
    RestoreCleared();
  }
}


MinifiedQuadSelector::MinifiedQuadSelector(QuadSelectorBase *next,
                                           const khLevelCoverage &cov,
                                           const khLevelCoverage &target_cov) :
    QuadSelectorBase(next->progress_, next->filters_, next->use_set_, cov),
    split_set_(num_filters_),
    something_split_(false),
    target_cov_(target_cov),
    next_(next)
{
}

void MinifiedQuadSelector::SplitSet(unsigned int i, std::uint32_t row, std::uint32_t col) {
  something_split_ = true;
  split_set_[i] = filters_[i].geo_index_;
  filters_[i].geo_index_ = filters_[i].geo_index_->SplitCell(row, col,
                                                             target_cov_);
}

void MinifiedQuadSelector::RestoreSplit(void) {
  for (unsigned int i = 0; i < num_filters_; ++i) {
    if (split_set_[i]) {
      // restore the one I split
      filters_[i].geo_index_ = split_set_[i];

      // clear my copy so I don't restore it next time
      split_set_[i] = gstGeoIndexHandle();
    }
  }
  something_split_ = false;
}

// ****************************************************************************
// ***  FullResQuadSelector
// ****************************************************************************
void FullResQuadSelector::SelectQuad(std::uint32_t row, std::uint32_t col) {
  assert(Contains(row, col));

  TryToClearSets(row, col);

  if (!something_left_) {
    progress_->incrementDone(1);
#if 0
    notify(NFY_VERBOSE, "FullResSelector pruning (lrc) %u,%u,%u",
           cov_.level, row, col);
#endif
  } else {
    QuadtreePath qpath(cov_.level, row, col);

    // get an empty output tile that we can reuse for this one
    WorkflowOutputTile *out_tile = manager_.GetEmptyOutputTile();
    out_tile->Reset();
    out_tile->path = qpath;

    // populate this output tile
    std::uint32_t numids = 0;
    for (unsigned int i = 0; i < num_filters_; ++i) {
      if (use_set_[i]) {
        FilterGeoIndex &filter = filters_[i];

        khDeleteGuard<gstGeoIndexImpl::FeatureBucket> tmp_bucket;
        if (cov_.level == filter.geo_index_->GetCoverage().level) {
          // We are selecting from a single cell from the index
          filter.geo_index_->GetFeatureIdsFromBucket(
              row, col, out_tile->displayRules[i].selectedIds);
        } else if (cov_.level < filter.geo_index_->GetCoverage().level) {
          // this index is deeper than we are exporting, we need to merge
          // from several FeatureBuckets
          // NOTE: This should be very rare
          khLevelCoverage export_cov =
            khTileAddr(cov_.level, row, col).
            MagnifiedToLevel(filter.geo_index_->GetCoverage().level);

          khExtents<std::uint32_t> to_check =
            khExtents<std::uint32_t>::Intersection(
                export_cov.extents, filter.geo_index_->GetCoverage().extents);

          filter.geo_index_->GetFeatureIdsFromBuckets(
              to_check, out_tile->displayRules[i].selectedIds);
        } else {
          notify(NFY_FATAL,
                 "Internal Error: SelectQuad called on un-split index");
        }

        // increment numids
        numids += out_tile->displayRules[i].selectedIds.size();
      }
    }

    if (numids) {
      manager_.HandleOutputTile(out_tile);
    } else {
      manager_.ReturnEmptyOutputTile(out_tile);
    }

    progress_->incrementDone(1);
    something_left_ = false;
  }

  if (something_cleared_) {
    RestoreCleared();
  }
}

FullResQuadSelector::FullResQuadSelector(
    khProgressMeter *progress,
    FilterGeoIndexManager &manager,
    std::vector<FilterGeoIndex> &filters,
    const khLevelCoverage &cov) :
    FullResQuadSelectorPreStorage(filters.size()),
    QuadSelectorBase(progress, filters, use_set_storage_, cov),
    manager_(manager)
{
}


} // namespace vectorquery
