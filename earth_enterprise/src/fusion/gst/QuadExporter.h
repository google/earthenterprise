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

// Classes for managing quadtree partitioning.
// Optimized quad partitioning by skipping empty quads at early stage.
// In SplitSet() (SplitCell() in GeoIndex) we fill grid with features IDs
// based on intersection of feature bounding boxes with grid cells. So for
// complex features that have large bounding boxes but cover only small area of
// their bounding boxes we get a lot of really empty grid cells for that we
// apply clipping with corresponding features. And we do it for all levels.
// On higher LODs number of really empty quads grow significantly.
// Levels are partitioned from lower to higher LODs.
// Now during processing quads of current level if we get quad that is not
// intersecting with features of some build set we store the information
// about quad state for this build set (update presence mask) and then during
// partitioning subsequent levels we analyze quad state at lower levels
// (in TryToClearSets()) and skip empty quads.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_QUADEXPORTER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_QUADEXPORTER_H_


#include "fusion/gst/gstLayer.h"
#include "fusion/gst/gstFilter.h"

class JSDisplayBundle;

class QuadExporterBase {
  friend class gstLayer;
  friend class MinifiedQuadExporter;
 protected:
  gstLayer *layer;
  khProgressMeter *progress;

  // all the parallel arrays below have this same size
  const unsigned int numSets;

  // Holds information about which filters contribute to the target level
  // Note: this is a ref to a single vector that get's modified/restored
  // Copying would be too expensive for the dense data case
  // All Exporters in the heirarchy point to the same vector
  // This could be a parameter to ExportQuad, but storing it here reduces
  // the call cost of ExportQuad
  std::vector<gstLayer::BuildSet> &buildSets;

  // Holds original BuildSets GeoIndexes.
  // Note: Used to update PresenceMask during building current
  // level and then analyze during building subsequent levels.
  // we build PresenceMask for levels from 0 to kMaxLevelForBuildPresenceMask.
  std::vector<gstGeoIndexHandle> *buildset_indexes_;

  // which indexes of buildsets still have useful information
  // Note: this is a ref to a single vector that get's modified/restored
  // Copying would be too expensive for the dense data case
  // All Exporters in the heirarchy point to the same vector
  // This could be a parameter to ExportQuad, but storing it here reduces
  // the call cost of ExportQuad
  std::vector<bool> &useSets;

  // Flag to set if I find out we need an LOD
  // This could be a parameter to ExportQuad, but storing it here reduces
  // the call cost of ExportQuad
  bool *needLOD;

  // Which indexes in useSets have I cleared while processing a single tile
  // This could be a local variable in ExportQuad, but the cost of allocating
  // and initializing the array each time gets expensive.
  std::vector<bool> clearedSet;
  bool somethingCleared;
  bool somethingLeft;

  // wanted coverage for my level
  khLevelCoverage cov;

  // Try to clear some sets out of the useSets list
  // This is called from inside tight loops. Mark it as inline and leave it
  // in the header with the hopes that the compiler is realy smart enough to
  // inline it.
  inline void TryToClearSets(std::uint32_t row, std::uint32_t col) {
    for (unsigned int i = 0; i < numSets; ++i) {
      if (useSets[i]) {
        if ((cov.level > kMaxLevelForBuildPresenceMask ||
             (*buildset_indexes_)[i]->GetEstimatedPresence(
                 khTileAddr(cov.level, row, col))) &&
            buildSets[i].geoIndex->GetEstimatedPresence(
                khTileAddr(cov.level, row, col))) {
          somethingLeft = true;
        } else {
          somethingCleared = true;
          clearedSet[i] = true;
          useSets[i] = false;
          // Update presence mask for current build set.
          if (cov.level <= kMaxLevelForBuildPresenceMask) {
            (*buildset_indexes_)[i]->SetPresence(
                khTileAddr(cov.level, row, col), false);
          }
        }
      }
    }
  }

  // Restore the sets that I cleared
  void RestoreCleared(void);

 public:
  inline bool Contains(std::uint32_t row, std::uint32_t col) {
    return cov.extents.ContainsRowCol(row, col);
  }
  virtual ~QuadExporterBase(void) { }
  QuadExporterBase(gstLayer *l, khProgressMeter *p,
                   std::vector<gstLayer::BuildSet> &build_sets,
                   std::vector<gstGeoIndexHandle> *buildset_indexes,
                   std::vector<bool> &use_sets,
                   bool *need_lod,
                   const khLevelCoverage &c);

  virtual bool ExportQuad(std::uint32_t row, std::uint32_t col) = 0;
};


class MinifiedQuadExporter : public QuadExporterBase {
 protected:
  // Holds original BuildSets when I split one. When I'm done processing
  // I restore these to their original locations
  // This could be a local variable in ExportQuad, but the cost of allocating
  // and initializing the array each time gets expensive.
  std::vector<gstGeoIndexHandle> splitSet;
  bool somethingSplit;

  // wanted coverage at target level
  khLevelCoverage targetCov;

  QuadExporterBase *next_;

  // Split all BuildSets that have reach the max level of their index
  // This is called from inside tight loops. Mark it as inline and leave it
  // in the header with the hopes that the compiler is realy smart enough to
  // inline it.
  inline void TryToSplitSets(std::uint32_t row, std::uint32_t col) {
    for (unsigned int i = 0; i < numSets; ++i) {
      if (useSets[i]) {
        if (cov.level == buildSets[i].geoIndex->MaxLevel()) {
          SplitSet(i, row, col);
        }
      }
    }
  }

  void SplitSet(unsigned int i, std::uint32_t row, std::uint32_t col);
  void RestoreSplit(void);

 public:
  MinifiedQuadExporter(QuadExporterBase *_next,
                       const khLevelCoverage &c,
                       const khLevelCoverage &target_cov);

  virtual bool ExportQuad(std::uint32_t row, std::uint32_t col);
};


class FullResQuadExporterPreStorage {
 public:
  std::vector<bool> use_sets;

  explicit FullResQuadExporterPreStorage(unsigned int numSets)
      : use_sets(numSets, true) { }
};
class FullResQuadExporter : private FullResQuadExporterPreStorage,
                            public QuadExporterBase {
  friend class gstLayer;
  std::vector<gstFeatureSet> feature_sets;
  std::vector<gstSiteSet>    site_sets;
  JSDisplayBundle &jsbundle;

 public:
  inline const std::vector<bool> &UsedSets(void) const { return use_sets; }
  inline JSDisplayBundle &JSBundle(void) { return jsbundle; }

  FullResQuadExporter(gstLayer *l, khProgressMeter *p,
                      std::vector<gstLayer::BuildSet> &build_sets,
                      std::vector<gstGeoIndexHandle> *buildset_indexes,
                      bool *need_lod,
                      const khLevelCoverage &c,
                      JSDisplayBundle &jsbundle_);

  virtual bool ExportQuad(std::uint32_t row, std::uint32_t col);
};




#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_QUADEXPORTER_H_
