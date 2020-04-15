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


#include <khFileUtils.h>
#include <khException.h>
#include <khProgressMeter.h>
#include "FilterGeoIndex.h"
#include "QuadSelector.h"


namespace vectorquery {


FilterGeoIndexManager::FilterGeoIndexManager(
    unsigned int level, const gstSharedSource &source,
    const std::vector<std::string> &select_files,
    const khTilespace &tilespace, double oversize_factor,
    const std::string& progress_meter_prefix) :
    tilespace_(tilespace),
    oversize_factor_(oversize_factor),
    level_(level),
    bbox_(),
    filter_indexes_(),
    source_(source),
    progress_meter_prefix_(progress_meter_prefix)
{

  for (unsigned int i = 0; i < select_files.size(); ++i) {
    std::uint64_t size;
    time_t mtime;
    if ((khGetFileInfo(select_files[i], size, mtime)) && (size > 0)) {
      gstGeoIndexHandle geo_index = gstGeoIndexImpl::Load(
          select_files[i], source_, tilespace_, oversize_factor_, level_);
      bbox_.Grow(geo_index->GetBoundingBox());
      filter_indexes_.push_back(FilterGeoIndex(i, geo_index));
    }
  }


  // Use the bbox and target level to build a full inset coverage
  // (e.g. coverage for each level)
  inset_coverage_ =
    khInsetCoverage(khExtents<double>(NSEWOrder, bbox_.n, bbox_.s,
                                      bbox_.e, bbox_.w),
                    tilespace_,
                    level_,    // full res level
                    0,         // begin coverage level
                    level_+1); // end coverage level
}


void FilterGeoIndexManager::DoSelection(void) {
  // add block to control lifetime of progress meter - and thereby the
  // final report
  {
    khLevelCoverage target_coverage = inset_coverage_.levelCoverage(level_);

    khProgressMeter progress(target_coverage.NumTiles(), "tiles",
                             progress_meter_prefix_);

    // build the full res selector at our target level
    // this is the one that eventually calls back to me to get the next
    // OutputTile and to handle the output tile
    khDeletingVector<QuadSelectorBase> selectors;
    selectors.reserve(level_ + 1);
    selectors.push_back(new FullResQuadSelector(&progress,
                                                *this,
                                                filter_indexes_,
                                                target_coverage));

    // build all the minified levels up to level 0
    // use int (instead of unsigned int) to avoid wrap at 0
    for (int lev = level_ - 1; lev >= 0; --lev) {
      selectors.push_back(new MinifiedQuadSelector
                          (selectors.back(),
                           inset_coverage_.levelCoverage(lev),
                           target_coverage));
    }

    // now just tell the top most selector (level 0) to select 0,0
    // everything else will fall out from there
    selectors.back()->SelectQuad(0, 0);
  }
}



} // namespace vectorquery
