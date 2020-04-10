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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSELECTOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSELECTOR_H_

#include <vector>
#include <list>

#include "fusion/gst/gstMemory.h"
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstRegistry.h"
#include "fusion/gst/gstFilter.h"
#include "fusion/autoingest/.idl/storage/QueryConfig.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstGeoIndex.h"

#include <qstringlist.h>

class LayerConfig;
class FuseConfig;

class gstLayer;
class gstSource;
class gstProgress;
class gstSiteSet;
class gstFeatureSet;
class gstDrawState;
class gstQuadAddress;
class JSDisplayBundle;

class khProgressMeter;

class gstSelector : public gstMemory {
 public:
  gstSelector(gstSource *src, int srcLayerNum,
              const LayerConfig& cfg,
              const QStringList &externalContextScripts_);
  gstSelector(gstSource *src, const QueryConfig& cfg);
  gstSelector(gstLayer *gst_layer, gstSource *src, const FuseConfig& cfg);

  ~gstSelector();

  // Called by GUI when LayerConfig changes
  void SetConfig(const LayerConfig &newlcfg, bool &keptQueries,
                 const QStringList &externalContextScripts_);

 public:
  gstSource* getSource() const;
  int getSourceID() const { return source_id_; }
  int layer() const { return layer_; }
  bool HasAttrib(void) const;

  gstRecordHandle getPickRecord(unsigned int id);
  gstGeodeHandle getPickGeode(unsigned int id);

  void deselect(int* rows, int count, bool selected);

  void ThrowingApplyQueries(khProgressMeter *progress);
  void ThrowingApplyQueries(gstProgress* guiProgress,
                            SoftErrorPolicy &soft_error_policy,
                            khProgressMeter *logProgress = 0);

  // CreateSelectionListFilesBatch creates the selection list files for
  // the given gst set in one pass while minimizing the memory footprint and
  // I/O calls.
  // Step through every geode of source file and apply filters
  // sequentially, writing out the selected feature ids as we go.
  void CreateSelectionListFilesBatch(
    const std::string& filter_selection_name_prefix,
    khProgressMeter *progress);
  void CreateSelectionListFilesBatch(
    const std::string& filter_selection_name_prefix,
    gstProgress* guiProgress, SoftErrorPolicy &soft_error_policy,
    khProgressMeter *logProgress);

  bool queryComplete(void) const { return query_complete_; }
  std::uint32_t NumSrcFeatures(void) const;
  double AverageSrcFeatureDiameter() const;

  void drawFeatures(int* max_count, const gstDrawState&);
  void drawSelectedFeatures(const gstDrawState&);

  enum {
    PICK_CLEAR_ADD,
    PICK_ADD,
    PICK_SUBTRACT
  };
  void applyBox(const gstDrawState&, int);

  void applyBoxCutter(const gstDrawState&, GeodeList&);

  // The geo_index - original build set GeoIndex. The coverage mask
  // that we keep in GeoIndex is used for optimization of completely
  // covered quads processing.
  bool PrepareFeatures(const gstQuadAddress &quad_address,
                       int filterId,
                       const gstGeoIndexHandle &geo_index,
                       const gstGeoIndexImpl::FeatureBucket &bucket,
                       const gstGeoIndexImpl::FeatureList &featureList,
                       gstFeatureSet *fset,
                       JSDisplayBundle &jsbundle);

  bool PrepareSites(const gstQuadAddress& quad_address,
                    int filter_id,
                    const gstGeoIndexImpl::FeatureBucket &bucket,
                    const gstGeoIndexImpl::FeatureList &featureList,
                    gstSiteSet& site_set,
                    JSDisplayBundle &jsbundle);

  bool PrepareSiteLabels(const gstBBox& box,
                         int filter_id,
                         gstSiteSet& site_set,
                         JSDisplayBundle &jsbundle,
                         bool is_mercator_preview);

  void ReduceRoads(const bool remove_overlapping_segments, std::uint32_t level,
                   gstFeatureSet *fset);
  void ReducePolylines(gstFeatureSet *fset);
  void ReducePolygons(gstFeatureSet *fset);

  // make static to allow outside objects to use this function
  static int RemoveOverlappingSegments(GeodeList* glist, int level);
  static unsigned int RemoveEmptyFeatures(GeodeList* geode_list,
                                  RecordList* record_list,
                                  unsigned int min_vertex_count);

  // Remove duplicate sites from the given site set.  Duplicate sites
  // have the same name and position and reside in the same layer.
  static int RemoveDuplicateSites(VertexList* vlist_ptr,
                                  RecordList* rlist_ptr);

  unsigned int NumFilters() const { return uint(filters_.size()); }
  gstFilter* GetFilter(unsigned int idx) const {
    return idx < static_cast< unsigned int> (filters_.size())
      ? filters_[idx] : NULL;
  }

  std::vector<SelectList> grabFilterQueries() const;
  bool stuffFilterQueries(const std::vector<SelectList>&);

  const SelectList& pickList() const { return pick_list_; }

  const SelectList& sortColumnOrThrow(int col, bool ascending);

  void DumpBuildStats();
  void UpdateSimplifyStats(unsigned int features_removed,
                           unsigned int vertices_removed,
                           double max_error);

 private:
  template <class T>
  inline gstFilter* CreateFilter(const T &t) {
    gstFilter *filter = new gstFilter(t, filters_.size());
    filters_.push_back(filter);
    return filter;
  }

  void RemoveAllFilters();

  bool convertFeatureType(gstGeodeHandle *geode,
                          const gstFeatureConfigs& feature_configs);

  int source_id_;
  int layer_;
  QueryConfig config;

  // Used while vector fusing for callback to collect statistics about
  // completely covered nodes.
  gstLayer* const gst_layer_;

  std::vector<gstFilter*> filters_;

  SelectList pick_list_;
  bool query_complete_;

  // collect some statistics when building
  struct BuildStats {
    std::uint64_t total_features;
    std::uint64_t total_vertexes;
    std::uint64_t join_count;
    std::uint64_t duplicate_features;
    std::uint64_t overlap_count;
    std::uint64_t reduced_features;
    std::uint64_t reduced_vertexes;
    std::uint64_t culled_features;
    std::uint64_t culled_vertexes;
    double max_simplify_error;
    std::uint64_t duplicate_sites;

    BuildStats() { Reset(); }
    void Reset() {
      memset(this, 0, sizeof(BuildStats));
      max_simplify_error = 0.5;
    }
  };

  BuildStats build_stats;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSELECTOR_H_
