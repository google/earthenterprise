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

#ifndef FUSION_GST_VECTORQUERY_PREVIEWSELECTOR_H__
#define FUSION_GST_VECTORQUERY_PREVIEWSELECTOR_H__

#include <vector>
#include <common/base/macros.h>
#include <quadtreepath.h>
#include <khTileAddr.h>
#include <workflow/PreviewStep.h>
#include <autoingest/.idl/storage/QueryConfig.h>
#include <gstGeoIndex.h>
#include <gstRecordJSContext.h>
#include <gstBBox.h>
#include <gst/gstSourceManager.h>
#include <gst/SoftErrorPolicy.h>
#include <geMultiRange.h>
#include "Tiles.h"

class UniqueFeatureId;
class gstSelectRule;
class gstProgress;

namespace vectorquery {


class RecordFilter {
 public:
  RecordFilter(const FilterConfig &cfg);
  ~RecordFilter(void);

  void Reset(void);
  // true on match, false on no match, exception on error
  bool TryApply(const gstRecordHandle &rec,
                const UniqueFeatureId &featureId,
                const gstRecordJSContext &cx,
                const KHJSScript &matchScript,
                SoftErrorPolicy &soft_errors);
  void Finalize(void);

  bool ReadQueryResultsFile(const std::string& path,
                            const gstSharedSource &source);

  std::uint32_t Intersect(const gstBBox& b, std::vector<int>* match_list);
 private:
  DISALLOW_COPY_AND_ASSIGN(RecordFilter);

  FilterConfig config;
  gstGeoIndexHandle geoIndex;
  std::uint32_t currentCount;
  khDeletingVector<gstSelectRule> selectRules;
};



class PreviewSelector :
      public workflow::PreviewStep<WorkflowOutputTile> {
 public:
  PreviewSelector(const gstSharedSource &source,
                  const QueryConfig &config_,
                  const std::vector<geMultiRange<std::uint32_t> > &validLevels_,
                  const khTilespace &tilspace_, double oversizeFactor);
  virtual ~PreviewSelector(void) { }

  void ApplyQueries(gstProgress &guiProgress,
                    SoftErrorPolicy &soft_errors);


  virtual bool Fetch(WorkflowOutputTile *out);

  bool CanReuse(const gstSharedSource &source_,
                const QueryConfig &config_,
                const std::vector<geMultiRange<std::uint32_t> > &validLevels_) const {
    return ((source == source_) &&
            (config == config_) &&
            (validLevels == validLevels_));
  }

 private:
  const QueryConfig config;
  const std::vector<geMultiRange<std::uint32_t> > validLevels;
  gstSharedSource source;
  const khTilespace& tilespace;
  double oversizeFactor;
  khDeletingVector<RecordFilter> filters;
};




} // namespace vectorquery

#endif // FUSION_GST_VECTORQUERY_PREVIEWSELECTOR_H__
