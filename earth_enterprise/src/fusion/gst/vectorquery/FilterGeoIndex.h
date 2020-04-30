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

#ifndef FUSION_GST_VECTORQUERY_FILTERGEOINDEX_H__
#define FUSION_GST_VECTORQUERY_FILTERGEOINDEX_H__

#include <vector>
#include "Tiles.h"
#include "fusion/gst/gstGeoIndex.h"
#include "fusion/gst/gstSourceManager.h"
#include "common/khInsetCoverage.h"


namespace vectorquery {


class FilterGeoIndex {
 public:
  FilterGeoIndex(unsigned int filter_id, gstGeoIndexHandle geo_index) :
      filter_id_(filter_id),
      geo_index_(geo_index)
  { }

  unsigned int filter_id_;
  gstGeoIndexHandle geo_index_;
};

class FilterGeoIndexManager {
 public:
  FilterGeoIndexManager(unsigned int level, const gstSharedSource &source,
                        const std::vector<std::string> &select_files,
                        const khTilespace &tilespace, double oversize_factor,
                        const std::string& progress_meter_prefix);
  virtual ~FilterGeoIndexManager(void) { }

  void DoSelection(void);
  virtual WorkflowOutputTile* GetEmptyOutputTile(void) = 0;
  virtual void HandleOutputTile(WorkflowOutputTile *tile) = 0;
  virtual void ReturnEmptyOutputTile(WorkflowOutputTile *tile) = 0;
  inline khLevelCoverage LevelCoverage(void) const {
    return inset_coverage_.levelCoverage(level_);
  }
  const gstSharedSource &GetSource(void) const { return source_; }
  const khTilespace     &GetTilespace(void) const { return tilespace_; }
  double GetOversizeFactor(void) const { return oversize_factor_; }

 private:
  const khTilespace&          tilespace_;
  const double                oversize_factor_;
  unsigned int                        level_;
  gstBBox                     bbox_;
  std::vector<FilterGeoIndex> filter_indexes_;
  gstSharedSource             source_;
  khInsetCoverage             inset_coverage_;
  const std::string           progress_meter_prefix_;

};


} // namespace vectorquery



#endif // FUSION_GST_VECTORQUERY_FILTERGEOINDEX_H__
