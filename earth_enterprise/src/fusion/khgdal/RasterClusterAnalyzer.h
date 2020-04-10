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

//  Class for determination of connectedness of groups of assets.
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_RASTERCLUSTERANALYZER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_RASTERCLUSTERANALYZER_H_

#include <string>
#include <vector>
#include <list>
#include "common/khExtents.h"

class khGDALDataset;
class khVirtualRaster;

//   Cluster
//   A Cluster is a set of connected insets in tile space.
//   Note: Connected here means that the bounding boxes(BB) which encompasses
//   each inset touch or intersect. Multiple insets may combine to produce a
//   cluster in such a way that the aggregate cluster BB contains empty tiles.
//   These empty tiles could intersect a new insets BB making the new inset
//   part of the cluster without actually touching or intersecting any other
//   individual inset BB in the cluster.
class Cluster {
 public:
  Cluster(const khExtents<std::uint32_t> &extent, const std::string &file)
      : extents(extent),
        file_list(1, file),
        is_new(true),
        // Set is_updated to true for unification - allows updating of a flag
        // is_updated in Cluster::Join().
        is_updated(true) {
    insets_area = Area();
  }

  void Print() const;

  bool Connects(const Cluster& other) const {
    // Note: Connected here means that the bounding box(BB) which encompasses
    // the new(other) inset touches or intersects the cluster BB.
    // The new(other) inset may connect with the cluster without actually
    // touching or intersecting any other individual inset BB in the cluster.
    return extents.connects(other.extents);
  }

  std::uint64_t Area() const {
    return extents.width() * extents.height();
  }

  std::uint64_t InsetsArea() const {
    return insets_area;
  }

  void Join(const Cluster& tojoin) {
    is_updated = true;
    extents.grow(tojoin.extents);
    insets_area += tojoin.insets_area;
    file_list.insert(file_list.end(), tojoin.file_list.begin(),
                     tojoin.file_list.end());
  }

  // Updates status - marks cluster new if cluster has been updated, and
  // resets is_updated flag.
  // Returns whether cluster's status has been updated (whether cluster is new).
  // Note: it is used in ClusterAnalyzer for algorithm optimization.
  bool UpdateStatus() {
    is_new = is_updated;
    is_updated = false;
    return is_new;
  }

  khExtents<std::uint32_t> extents;
  std::vector<std::string> file_list;
  // Note: the is_new, is_updated flags are used in ClusterAnalyzer for
  // algorithm optimization.
  bool is_new;
  bool is_updated;
  std::uint64_t insets_area;
};

// Class ClusterAnalyzer analyzes virtual raster for:
// - insets clustering in raster product tile space;
// - insets coverage in mosaiced area;
class ClusterAnalyzer {
 public:
  // Compute clusters of insets to check if insets of a virtual raster
  // intersect in a tilespace.
  // The infile is a *.khvr file, override_srs - use given SRS,
  // is_mercator - whether projection is Mercator.
  void CalcAssetClusters(const std::string &infile,
                         const std::string &override_srs,
                         const bool is_mercator);

  // The virtraster is a virtual raster, toplevel - normalized top resolution
  // level, is_mercator - whether projection is Mercator.
  void CalcAssetClusters(const khVirtualRaster &virtraster,
                         const size_t toplevel,
                         const bool is_mercator);
  // Print inset clusters.
  void PrintClusters() const;

  // Calculates sum of inset areas, virtual raster area in tile space and
  // their ratio.
  // The srcDS is a GDAL dataset built based on a virtual raster.
  void CalcAssetAreas(const khGDALDataset &srsDS);

  // The infile is a *.khvr file, is_mercator - whether projection is Mercator.
  void CalcAssetAreas(const std::string &infile,
                      const bool is_mercator);

  // The infile is a *.khvr file, override_srs - use given SRS,
  // override_srs - use given SRS, is_mercator - whether projection is Mercator.
  void CalcAssetAreas(const std::string &infile,
                      const std::string &override_srs,
                      const bool is_mercator);

  // The virtraster is a virtual raster, toplevel - normalized top resolution
  // level, is_mercator - whether projection is Mercator.
  void CalcAssetAreas(const khVirtualRaster &virtraster,
                      const size_t toplevel,
                      const bool is_mercator);

 private:
  // Area ratio threshold specifes a threshold when report warning.
  static const double area_ratio_threshold;
  typedef std::list<Cluster> ClusterList;
  ClusterList clusters_;

  // Triggers update of status for all clusters.
  // Returns whether any of clusters has been updated.
  bool UpdateClustersStatus();

  void AddCluster(const Cluster& cluster) {
    clusters_.push_back(cluster);
  }

  size_t NumClusters() const {
    return clusters_.size();
  }

  // Calculates sum of insets areas based on information in clusters.
  std::uint64_t CalcRasterInsetsArea() const;

  // Calculates virtual raster (mosaic) area based on information in clusters.
  std::uint64_t CalcRasterArea() const;

  // Prints virtual raster info: sum of inset areas, raster area, their ratio.
  void PrintRasterInfo(const std::uint64_t raster_inset_area,
                       const std::uint64_t raster_area,
                       const bool is_area_check) const;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_RASTERCLUSTERANALYZER_H_
