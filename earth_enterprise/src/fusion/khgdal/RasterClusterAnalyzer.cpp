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


#include "fusion/khgdal/RasterClusterAnalyzer.h"

#include <khgdal/.idl/khVirtualRaster.h>
#include "fusion/khgdal/khGDALDataset.h"
#include "common/notify.h"


// Cluster.
void Cluster::Print() const {
  std::uint64_t area = Area();
  notify(NFY_NOTICE, "  Sum of inset areas: %zu", InsetsArea());
  notify(NFY_NOTICE, "  Cluster area: %zu", area);
  if (area != 0) {
    double area_ratio = static_cast<double>(InsetsArea())/area;
    notify(NFY_NOTICE, "  Ratio of sum of inset areas to cluster area: %.2f.",
           area_ratio);
  }

  notify(NFY_NOTICE, "  Insets:");
  for (size_t i = 0; i < file_list.size(); ++i) {
    notify(NFY_NOTICE, "    %zu: %s", i, file_list[i].c_str());
  }
}


// ClusterAnalyzer.
const double ClusterAnalyzer::area_ratio_threshold = 0.8;

// Compute clusters of insets to check if insets of a virtual raster
// intersect in a tilespace.
void ClusterAnalyzer::CalcAssetClusters(const std::string &infile,
                                        const std::string &override_srs,
                                        bool is_mercator) {
  khVirtualRaster virtraster;
  notify(NFY_NOTICE, "Loading virtual raster %s", infile.c_str());
  if (!virtraster.Load(infile)) {
    notify(NFY_FATAL, "Couldn't load virtual raster %s",
           infile.c_str());
  }

  const khGDALDataset srcDS(infile,
                            override_srs,
                            khExtents<double>(),
                            is_mercator ?
                            khTilespace::MERCATOR_PROJECTION :
                            khTilespace::FLAT_PROJECTION);
  ClusterAnalyzer::CalcAssetClusters(virtraster, srcDS.normalizedTopLevel(),
                                     srcDS.IsMercator());
}


// Compute clusters of insets to check if insets of a virtual raster
// intersect in a tilespace.
void ClusterAnalyzer::CalcAssetClusters(const khVirtualRaster &virtraster,
                                        const size_t toplevel,
                                        const bool is_mercator) {
  notify(NFY_NOTICE,
         "Find clusters of insets (insets intersected in tilespace)...");
  const size_t num_insets = virtraster.inputTiles.size();
  if (num_insets < 2) {
    notify(NFY_NOTICE, "%zu inset -> no check needed", num_insets);
    return;
  }
  double dX = virtraster.pixelsize.width;
  double dY = virtraster.pixelsize.height;
  const khTilespace& tilespace = RasterProductTilespace(is_mercator);
  for (size_t i = 0; i < num_insets; ++i) {
    const khVirtualRaster::InputTile& input_tile = virtraster.inputTiles[i];
    khOffset<double> offset = input_tile.origin;
    double degX = (input_tile.rastersize.width) * dX;
    double degY = (input_tile.rastersize.height) * dY;
    // Here khExtents expects lower left origin:
    const khExtents<double> deg_extent(XYOrder,
                                       std::min(offset.x(), offset.x() + degX),
                                       std::max(offset.x(), offset.x() + degX),
                                       std::min(offset.y(), offset.y() + degY),
                                       std::max(offset.y(), offset.y() + degY));
    const khLevelCoverage cov_toplevel(
        tilespace,
        is_mercator ?
        khCutExtent::ConvertFromFlatToMercator(deg_extent) : deg_extent,
        toplevel, toplevel);

    Cluster new_cluster(cov_toplevel.extents, input_tile.file);
    ClusterList::iterator it = clusters_.begin();
    for ( ; it != clusters_.end(); ++it) {
      Cluster& cur = *it;
      if (cur.Connects(new_cluster)) {
        cur.Join(new_cluster);
        break;
      }
    }
    if (it == clusters_.end()) {
      AddCluster(new_cluster);
    }
  }

  // Note: here, all clusters have a status: is_new=true, is_updated=true.
  // So, run status update to reset is_updated which will be used in below part
  // of algorithm.
  UpdateClustersStatus();

  do {
    for (ClusterList::iterator it_a = clusters_.begin();
         it_a != clusters_.end(); ) {
      Cluster& cluster_a = *it_a;
      ClusterList::iterator it_b = it_a;
      ++it_b;
      for ( ; it_b != clusters_.end(); ++it_b) {
        Cluster& cluster_b = *it_b;
        // Note: if cluster (cluster_a) is updated (means it has
        // been updated on the current iteration), then we have to re-check this
        // cluster for connecting with all clusters after this one in the list.
        //
        // If cluster (cluster_b) is a new cluster, it means this cluster has
        // been updated on previous iteration, and we need to re-check against
        // this cluster all clusters located before this one in the list. All
        // clusters that are after this one in the list have been checked
        // against this one on previous iteration.
        if (cluster_a.is_updated || cluster_b.is_new) {
          if (cluster_b.Connects(cluster_a)) {
            cluster_b.Join(cluster_a);
            ClusterList::iterator to_delete = it_a;
            ++it_a;
            clusters_.erase(to_delete);
            break;
          }
        }
      }

      if (it_b == clusters_.end()) {
        // There was no cluster joining/deleting.
        ++it_a;
      }
    }
  } while (UpdateClustersStatus());
}

bool ClusterAnalyzer::UpdateClustersStatus() {
  bool updated = false;
  for (ClusterList::iterator it = clusters_.begin();
         it != clusters_.end(); ++it) {
    updated |= it->UpdateStatus();
  }
  return updated;
}

void ClusterAnalyzer::PrintClusters() const {
  const bool is_area_check = false;
  size_t num_clusters = NumClusters();
  if (num_clusters == 0) {
    notify(NFY_NOTICE, "No clusters of insets.");
  } else if (num_clusters == 1) {
    notify(NFY_NOTICE, "Insets of virtual raster form single cluster.");
    const Cluster &cluster = clusters_.front();
    PrintRasterInfo(cluster.InsetsArea(), cluster.Area(), is_area_check);
  } else {
    size_t idx = 0;
    for (ClusterList::const_iterator it = clusters_.begin();
         it != clusters_.end(); ++it, ++idx) {
      const Cluster& cur = *it;
      notify(NFY_NOTICE, "Cluster %zu info:", idx);
      cur.Print();
    }
    notify(NFY_WARN,
           "Insets of virtual raster form non-intersecting clusters.\n");
    PrintRasterInfo(CalcRasterInsetsArea(), CalcRasterArea(), is_area_check);
  }
}

void ClusterAnalyzer::CalcAssetAreas(const std::string &infile,
                                     const std::string &override_srs,
                                     bool is_mercator) {
  khVirtualRaster virtraster;
  notify(NFY_NOTICE, "Loading virtual raster %s", infile.c_str());
  if (!virtraster.Load(infile)) {
    notify(NFY_FATAL, "Couldn't load virtual raster %s",
           infile.c_str());
  }

  const khGDALDataset srcDS(infile,
                            override_srs,
                            khExtents<double>(),
                            is_mercator ?
                            khTilespace::MERCATOR_PROJECTION :
                            khTilespace::FLAT_PROJECTION);
  CalcAssetAreas(virtraster, srcDS.normalizedTopLevel(), srcDS.IsMercator());
}

void ClusterAnalyzer::CalcAssetAreas(const khGDALDataset &srcDS) {
  khVirtualRaster virtraster;
  notify(NFY_NOTICE, "Loading virtual raster %s", srcDS.GetFilename().c_str());
  if (!virtraster.Load(srcDS.GetFilename())) {
    notify(NFY_FATAL, "Couldn't load virtual raster %s",
           srcDS.GetFilename().c_str());
  }
  CalcAssetAreas(virtraster, srcDS.normalizedTopLevel(), srcDS.IsMercator());
}

void ClusterAnalyzer::CalcAssetAreas(const khVirtualRaster &virtraster,
                                     const size_t toplevel,
                                     const bool is_mercator) {
  notify(NFY_NOTICE, "Analyze virtual raster and insets areas...");
  const size_t num_insets = virtraster.inputTiles.size();
  if (num_insets < 2) {
    notify(NFY_NOTICE, "Number of insets: %zu - no check needed", num_insets);
    return;
  }
  double dX = virtraster.pixelsize.width;
  double dY = virtraster.pixelsize.height;
  const khTilespace& tilespace = RasterProductTilespace(is_mercator);
  std::uint64_t sum_inset_area = 0;
  khExtents<std::uint32_t> mosaic_extent;
  for (size_t i = 0; i < num_insets; ++i) {
    const khVirtualRaster::InputTile& input_tile = virtraster.inputTiles[i];
    khOffset<double> offset = input_tile.origin;
    double degX = (input_tile.rastersize.width) * dX;
    double degY = (input_tile.rastersize.height) * dY;
    // Here khExtents expects lower left origin:
    const khExtents<double> deg_extent(XYOrder,
                                       std::min(offset.x(), offset.x() + degX),
                                       std::max(offset.x(), offset.x() + degX),
                                       std::min(offset.y(), offset.y() + degY),
                                       std::max(offset.y(), offset.y() + degY));
    const khLevelCoverage
        cov_toplevel(tilespace,
                     is_mercator ?
                     khCutExtent::ConvertFromFlatToMercator(deg_extent) :
                     deg_extent,
                     toplevel, toplevel);
    const khExtents<std::uint32_t>& extent = cov_toplevel.extents;
    mosaic_extent.grow(extent);
    sum_inset_area += extent.width() * extent.height();
  }

  std::uint64_t mosaic_area = mosaic_extent.width() * mosaic_extent.height();
  const bool is_area_check = true;
  PrintRasterInfo(sum_inset_area, mosaic_area, is_area_check);
}

 std::uint64_t ClusterAnalyzer::CalcRasterInsetsArea() const {
  std::uint64_t sum_insets_area = 0;
  for (ClusterList::const_iterator it = clusters_.begin();
       it != clusters_.end(); ++it) {
    sum_insets_area += it->InsetsArea();
  }
  return sum_insets_area;
}

 std::uint64_t ClusterAnalyzer::CalcRasterArea() const {
  khExtents<std::uint32_t> extents;
  for (ClusterList::const_iterator it = clusters_.begin();
       it != clusters_.end(); ++it) {
    extents.grow(it->extents);
  }
  return extents.width()*extents.height();
}

void ClusterAnalyzer::PrintRasterInfo(const std::uint64_t raster_inset_area,
                                      const std::uint64_t raster_area,
                                      const bool is_area_check) const {
  notify(NFY_NOTICE, "Sum of inset areas: %zu", raster_inset_area);
  notify(NFY_NOTICE, "Virtual raster area: %zu", raster_area);

  if (raster_area != 0) {
    double area_ratio = static_cast<double>(raster_inset_area) /
        static_cast<double>(raster_area);
    if (area_ratio < ClusterAnalyzer::area_ratio_threshold) {
      notify(NFY_WARN,
             "Ratio of sum of inset areas to virtual raster area: %.2f.",
             area_ratio);
      notify(NFY_WARN, "The virtual raster may contain inefficient gaps.");
      if (is_area_check) {
        notify(NFY_NOTICE,
               "Use --check_clusters for more information on how virtual raster"
               " might be\n better subdivided.");
      }
    }
  }
  notify(NFY_NOTICE,
         "*.khvr file can be opened in Fusion to visually analyze virtual"
         " raster topology.");
}
