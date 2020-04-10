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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTLAYER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTLAYER_H_

#include <pthread.h>
#include <semaphore.h>

#include <string>
#include <vector>
#include <algorithm>

#include <qstringlist.h>

#include "common/khExtents.h"
#include "fusion/gst/gstMemory.h"
#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstRegistry.h"
#include "fusion/gst/gstLimits.h"
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstSite.h"
#include "fusion/gst/gstGeoIndex.h"
#include "fusion/gst/gstSourceManager.h"
#include "fusion/gst/gstSimplifier.h"

class gstFilter;
class gstSelector;
class gstSource;
class gstVectorProject;
class gstDrawState;
class gstExporter;
class gstProgress;
class gstFeatureSet;
class khProgressMeter;
class gstSimplifier;
class gstQuadAddress;
class gstIcon;
class FullResQuadExporter;
class JSDisplayBundle;
class geFilePool;
class FuseConfig;

// Normalized lat/long (0.0 -> 1.0)
inline void
LatLonToRowCol(unsigned int lev,
               double lat, double lon,
               std::uint32_t& row, std::uint32_t& col) {
  // clamp lat
  if (lat <= 0.25) {
    if (lev < 2) {
      row = 0;
    } else {
      row = 0x1U << (lev - 2);
    }
  } else if (lat >= 0.75) {
    if (lev < 2) {
      row = lev;
    } else {
      row = (0x1U << lev) - (0x1U << (lev - 2)) - 1;
    }
  } else {
    row = static_cast<std::uint32_t>(lat * static_cast<double>(0x1U << lev));
  }

  // clamp lon
  if (lon <= 0.0) {
    col = 0;
  } else if (lon >= 1.0) {
    col = (0x1U << lev) - 1;
  } else {
    col = static_cast<std::uint32_t>(lon * static_cast<double>(0x1U << lev));
  }
}

inline khExtents<std::uint32_t> ExtentsForBoundingBox(gstBBox box, int lev) {
  std::uint32_t min_row, max_row, min_col, max_col;
  LatLonToRowCol(lev, box.s, box.w, min_row, min_col);
  LatLonToRowCol(lev, box.n, box.e, max_row, max_col);

  return khExtents<std::uint32_t>(RowColOrder,
                           min_row, max_row + 1,
                           min_col, max_col + 1);
}

////////////////////////////////////////////////////////////////////////////////

class gstLevelState {
 public:
  gstLevelState();

  gstLevelState& operator=(const gstLevelState& that);

  void reset();
  bool isEmpty() const;

  void set(int l, int state = 1) {
    _states[ l ] = state;
  }
  int get(int l) const {
    return _states[l];
  }
  int operator[](int l) const {
    return _states[l];
  }

  const int* lods() {
    return &_states[0];
  }

  unsigned int size() const {
    return NUM_LEVELS;
  }

 private:
  int _states[NUM_LEVELS];
};

////////////////////////////////////////////////////////////////////////////////

class gstLayer : public gstMemory {
 public:
  class BuildSet {
   public:
    bool Valid(void) const { return (filterId != -1); }

    BuildSet(void) : filterId(-1) { }
    BuildSet(int f, gstGeoIndexHandle idx,
             gstPrimType t, unsigned int e, unsigned int max_build_level_in)
        : filterId(f), geoIndex(idx), type(t), endlevel(e),
          max_build_level(max_build_level_in) {
      endlevel = std::min(endlevel, MaxClientLevel);
      max_build_level = std::min(max_build_level, MaxClientLevel);
    }

    int filterId;
    gstGeoIndexHandle geoIndex;

    // NOTE: BuildSet type can be gstPoint/gstPolyLine/gstStreet/
    // gstPolygon.
    gstPrimType type;
    unsigned int endlevel;
    unsigned int max_build_level;
  };

  class BuildSetMgr {
   public:
    explicit BuildSetMgr(gstSelector* sel);

    bool IsLevelValid(int lev) { return build_area_[lev].Valid(); }
    gstBBox GetBBox(int lev) { return build_area_[lev]; }
    std::vector<BuildSet>& GetBoxList(int lev) { return box_list_[lev]; }
    bool DumpLODTable(const QString &);
    void Update(unsigned int lev, bool need_lod);

   private:
    void AddFilter(int fid);

    gstBBox build_area_[MAX_CLIENT_LEVEL + 1];
    std::vector<BuildSet> box_list_[MAX_CLIENT_LEVEL + 1];

    gstSelector* selector_;
    gstLevelState road_levels_;
    gstLevelState fadeout_levels_;
    //  gstLevelState lod_state_;
  };

  gstLayer(gstVectorProject* p, gstSource *src, int srcLayerNum,
           const LayerConfig& cfg);
  gstLayer(gstVectorProject* p, gstSource *src, const FuseConfig& cfg);

  ~gstLayer();

  void SetConfig(const LayerConfig &);
  inline const LayerConfig& GetConfig() const { return config; }

  const char* GetAssetName() const;

  void QueryThreadPrep(gstProgress* progress);
  void QueryThread();

 private:
  void Init();
  void ResetNodeStatistics();
  gstProgress* query_progress_;

 public:
  void DrawFeatures(int* max_count, const gstDrawState &, bool enhanced);
  void GetSiteLabels(const gstDrawState& state,
                     std::vector<gstSiteSet>* site_sets);

  bool ExportStreamPackets(geFilePool &file_pool, const std::string& path);
  QStringList GetExternalContextScripts(void) const;

 private:
  bool BuildRegion(const khExtents<std::uint32_t>& tiles, int level,
                   std::vector<BuildSet> &list,
                   bool* need_lod,
                   JSDisplayBundle &jsbundle);

  friend class FullResQuadExporter;
  bool ExportQuad(const gstQuadAddress& quad_address,
                  FullResQuadExporter &exporter);

  // SimplifyFeatureSet takes a set of features and simplifies them for
  // the specified level of detail (level) and loop count (the number of
  // simplification passes we're on).
  // Each loop doubles the allowable error.
  // Return the maximum simplification error (i.e., the maximum error between
  // the original features and the simplified features).
  double SimplifyFeatureSet(std::vector<gstFeatureSet> *featureSets,
                            int level, int loop);
  // SimplifySiteSet takes a set of site features (POIs) and simplifies them for
  // the specified level of detail (level) and loop count (the number of
  // simplification passes we're on).
  // Simplification essentially reduces the number of POIs per quadtree node
  // to a specified number by clustering nearby POIs into a single POI.
  // Each loop doubles the allowable amount of simplification.
  // Return true if any simplification occurred (i.e., the number of sites
  // is reduced).
  bool SimplifySiteSet(std::vector<gstSiteSet> *siteSets, int level, int loop);

  void simplify25DPolygons(GeodeList *glist, int loopcount,
                           int *geodes_removed, int *vertices_removed);

  void simplify3DPolygons(GeodeList *glist, int loopcount,
                          int *geodes_removed, int *vertices_removed);

  bool TryPopulateJSDisplayBundle(JSDisplayBundle *, QString *error);

 public:
  bool isGroup() const { return (the_selector_ == NULL); }
  bool isTopLevel() const { return Legend().isEmpty(); }
  gstSelector* GetSelector() const { return the_selector_; }

  //////////////////////////////
  // new API which allows gstSelector to be hidden
  gstSource* GetSource() const;
  gstSharedSource GetSharedSource(void) const;
  int       GetSourceLayerNum(void) const;
  gstHeaderHandle GetSourceAttr();
  gstFilter* GetFilterById(unsigned int id) const;
  unsigned int NumFilters(void) const;
  void ApplyBoxCutter(const gstDrawState&, GeodeList&);
  void ApplyBox(const gstDrawState&, int);
  bool QueryComplete() const;


  //////////////////////////////

  int Version() const { return version_; }
  void SetVersion(int v);

  unsigned int Id() const { return config.channelId; }

  unsigned int SortId() const { return sort_id_; }
  void SetSortId(unsigned int i);

  bool InitialState() const {
    return config.defaultLocale.is_checked_.GetValue();
  }

  QString Icon() const { return config.defaultLocale.icon_.GetValue().href;}
  IconReference::Type IconType() const {
    return config.defaultLocale.icon_.GetValue().type;
  }

  bool Enabled() const { return enabled_; }
  void SetEnabled(bool s);

  // control types (main channel list, or subordinate to client's Map Controls)
  enum {
    DEFAULT = 0,
    BORDERS = 1,
    ROADS   = 2
  };

  const QString& Legend() const { return config.legend; }
  void SetLegend(const QString& l);

  void SetName(const QString& n);
  QString GetShortName() const { return config.defaultLocale.name_.GetValue();}
  std::string GetUuid() const { return config.asset_uuid_;}
  std::string GetAssetRef() const { return config.assetRef; }
  QString GetPath() const { return config.DefaultNameWithPath(); }
  inline void IncrementSkipped(std::uint64_t count) { node_count_skipped_ += count; }
  inline void IncrementCovered(std::uint64_t count) { node_count_covered_ += count; }

 private:
  friend class gstVectorProject;

  LayerConfig config;

  gstVectorProject* parent_;
  gstSelector* the_selector_;
  gstSimplifier simplifier_;
  gstFeatureCuller culler_;

  unsigned int sort_id_;

  int version_;         // last build version number

  bool enabled_;
  gstExporter* exporter_;

  // The Efficient LOD. Depends on the Efficient LOD we apply different
  // simplification procedure to build LOD for 2.5D polygon.
  // For levels that are less than Efficient LOD we at first apply feature
  // culling by diameter to reject features that are too small to affect the
  // display and then feature culling by Volume to fit packet size.
  // Feature culling by diameter is applied with the same threshold for all of
  // quads of level.
  // For levels from Efficient LOD to Max Resolution Level we only apply
  // feature culling by Volume to fit packet size.
  int efficient_lod_;

  std::uint64_t node_count_total_;
  std::uint64_t node_count_done_;
  std::uint64_t node_count_covered_;
  std::uint64_t node_count_empty_;
  std::uint64_t node_count_skipped_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTLAYER_H_
