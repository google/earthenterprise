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

// Methods for managing quadtree partitioning.

#include "fusion/gst/QuadExporter.h"

#include "fusion/gst/gstQuadAddress.h"
#include "common/khProgressMeter.h"
#include "common/notify.h"


// ****************************************************************************
// ***  QuadExporterBase
// ****************************************************************************
QuadExporterBase::QuadExporterBase(
    gstLayer *l, khProgressMeter *p,
    std::vector<gstLayer::BuildSet> &build_sets,
    std::vector<gstGeoIndexHandle> *buildset_indexes,
    std::vector<bool> &use_sets,
    bool *need_lod,
    const khLevelCoverage &c)
    : layer(l),
      progress(p),
      numSets(build_sets.size()),
      buildSets(build_sets),
      buildset_indexes_(buildset_indexes),
      useSets(use_sets),
      needLOD(need_lod),
      clearedSet(numSets, false),
      somethingCleared(false),
      somethingLeft(false),
      cov(c) {
  notify(NFY_DEBUG, "Exporter lev:%u, row:%u->%u, col:%u->%u, tiles:%Zu",
         cov.level, cov.extents.beginRow(), cov.extents.endRow(),
         cov.extents.beginCol(), cov.extents.endCol(),
         cov.NumTiles());
}

void QuadExporterBase::RestoreCleared(void) {
  for (unsigned int i = 0; i < numSets; ++i) {
    if (clearedSet[i]) {
      clearedSet[i] = false;
      useSets[i] = true;
    }
  }
  somethingCleared = false;
}

// ****************************************************************************
// ***  MinifiedQuadExporter
// ****************************************************************************
bool MinifiedQuadExporter::ExportQuad(std::uint32_t row, std::uint32_t col) {
  assert(Contains(row, col));

  TryToClearSets(row, col);

  if (!somethingLeft) {
    // The index tells us we have nothing to contribute. But we have counted
    // the tiles below this as work that needs to be done. Calculate how
    // many tiles we're going to skip and mark them as skipped.

    // figure out this tile's coverage at the target level
    khLevelCoverage thisTargetCov =
      khTileAddr(cov.level, row, col).MagnifiedToLevel(targetCov.level);

    // intersect that with the targetCoverage (what we really want to do)
    khExtents<std::uint32_t> skipExtents =
      khExtents<std::uint32_t>::Intersection(thisTargetCov.extents,
                                      targetCov.extents);

    // now we know how many we're going to skip
    std::uint64_t numSkipped = static_cast<std::uint64_t>(skipExtents.numRows()) *
                        static_cast<std::uint64_t>(skipExtents.numCols());

    progress->incrementDone(numSkipped);
    layer->IncrementSkipped(numSkipped);
#if 0
    notify(NFY_VERBOSE, "MinifiedExporter pruning (lrc) %u,%u,%u",
           cov.level, row, col);
#endif
  } else {
    TryToSplitSets(row, col);

    // for each of the four source tiles / quadrants
    for (unsigned int quad = 0; quad < 4; ++quad) {
      // magnify the dest row/col/quad to get row/col for the next level
      std::uint32_t nextRow = 0;
      std::uint32_t nextCol = 0;
      QuadtreePath::MagnifyQuadAddr(row, col, quad, nextRow, nextCol);

      // check if quad exists the next row down
      if (next_->Contains(nextRow, nextCol)) {
        if (!next_->ExportQuad(nextRow, nextCol)) {
          return false;
        }
      }
    }

    if (somethingSplit) {
      RestoreSplit();
    }

    somethingLeft = false;
  }

  if (somethingCleared) {
    RestoreCleared();
  }

  return true;
}


MinifiedQuadExporter::MinifiedQuadExporter(QuadExporterBase *_next,
                                           const khLevelCoverage &c,
                                           const khLevelCoverage &target_cov)
    : QuadExporterBase(_next->layer, _next->progress,
                       _next->buildSets,
                       _next->buildset_indexes_,
                       _next->useSets,
                       _next->needLOD, c),
      splitSet(numSets),
      somethingSplit(false),
      targetCov(target_cov),
      next_(_next) {
}

void MinifiedQuadExporter::SplitSet(unsigned int i, std::uint32_t row, std::uint32_t col) {
  somethingSplit = true;
  splitSet[i] = buildSets[i].geoIndex;
  buildSets[i].geoIndex = buildSets[i].geoIndex->SplitCell(row, col,
                                                           targetCov);
}

void MinifiedQuadExporter::RestoreSplit(void) {
  for (unsigned int i = 0; i < numSets; ++i) {
    if (splitSet[i]) {
      // restore the one I split
      buildSets[i].geoIndex = splitSet[i];

      // clear my copy so I don't restore it next time
      splitSet[i] = gstGeoIndexHandle();
    }
  }
  somethingSplit = false;
}

// ****************************************************************************
// ***  FullResQuadExporter
// ****************************************************************************
bool FullResQuadExporter::ExportQuad(std::uint32_t row, std::uint32_t col) {
  assert(Contains(row, col));

  TryToClearSets(row, col);

  if (!somethingLeft) {
    layer->IncrementSkipped(1);
    progress->incrementDone(1);
#if 0
    notify(NFY_VERBOSE, "FullResExporter pruning (lrc) %u,%u,%u",
           cov.level, row, col);
#endif
  } else {
    // TODO: Rework gstLayer::ExportQuad so I can pass only those
    // filters that have something here.
    if (!layer->ExportQuad(gstQuadAddress(cov.level, row, col),
                           *this)) {
      // returning false will abort the entire export
      return false;
    }
    progress->incrementDone(1);
    somethingLeft = false;
  }

  if (somethingCleared) {
    RestoreCleared();
  }

  return true;
}

FullResQuadExporter::FullResQuadExporter(
    gstLayer *l, khProgressMeter *p,
    std::vector<gstLayer::BuildSet> &build_sets,
    std::vector<gstGeoIndexHandle> *buildset_indexes,
    bool *need_lod,
    const khLevelCoverage &c,
    JSDisplayBundle &jsbundle_)
    : FullResQuadExporterPreStorage(build_sets.size()),
      QuadExporterBase(
          l, p, build_sets, buildset_indexes, use_sets, need_lod, c),
      jsbundle(jsbundle_) {
  feature_sets.reserve(numSets);
  site_sets.reserve(numSets);
}
