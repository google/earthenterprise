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

// The class khCoverageMask provides functionality to build and keep coverage
// mask for vector dataset (SetCovered()) and get information about the features
// covering specific tile (GetCovered()).
// If specific tile is completely covered with specific polygonal feature, we
// set covered bit to true for that tile and store the feature ID to an
// associative container that maps tile address to feature ID (SetCovered()).
// To get information whether specific tile is completely covered with any
// polygonal feature, we traverse the quad tree levels of coverage mask from
// level of the requested tile to the begin level and collect all the feature
// IDs stored with corresponding quads.
// Note: We store only feature ID to address the feature then since
// a separate coverage mask is built for every build set.

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHCOVERAGEMASK_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHCOVERAGEMASK_H_

#include <limits>
#include <set>
#include <map>

#include "common/khLevelPresenceMask.h"
#include "common/khTileAddr.h"
#include "common/geFileUtils.h"
#include "common/khGuard.h"

class khInsetCoverage;

// khCoverageMask
class khCoverageMask {
 public:
  // for copying another
  khCoverageMask(const khCoverageMask &);
  khCoverageMask& operator=(const khCoverageMask &);

  // set_present - whether build levels with all "not present" or all "present".
  khCoverageMask(const khInsetCoverage &coverage, bool set_present = false);

  inline unsigned int NumLevels() const { return (end_level_ - begin_level_); }

  // Whether specific tile is completely covered by some polygon.
  // If asked for tileAddr outside the stored level range it will
  // always return false.
  // The feature_id_set - container for list of feature IDs covering
  // specific tile.
  bool GetCovered(const khTileAddr &addr,
                  std::set<int> *feature_id_set) const;

  // Sets covered for all elements of specified level.
  inline void SetCovered(unsigned int lev, bool set_covered) {
    assert(levels_[lev]);
    levels_[lev]->SetPresence(set_covered);
  }

  // Sets that specific tile (addr) is completely covered with specific feature
  // (feature_id).
  void SetCovered(const khTileAddr &addr, int feature_id);

  // Fill in coverage object with my coverage
  void PopulateCoverage(khInsetCoverage *cov) const;

  bool ValidLevel(unsigned int level) const {
    return ((level >= begin_level_) && (level < end_level_));
  }

  khExtents<std::uint32_t> LevelTileExtents(unsigned int level) const;

 private:
  unsigned int begin_level_;
  unsigned int end_level_;
  khDeleteGuard<khLevelPresenceMask> levels_[NumFusionLevels];
  typedef std::multimap<std::uint64_t, int> TileAddrToFeatureIdMap;
  TileAddrToFeatureIdMap tile_addr_to_feature_id_map;
};  // class khCoverageMask

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHCOVERAGEMASK_H_
