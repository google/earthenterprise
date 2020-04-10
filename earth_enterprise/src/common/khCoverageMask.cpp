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

#include "common/khCoverageMask.h"

#include <string.h>

#include <algorithm>
#include <iterator>
#include <utility>
#include <functional>

#include "common/khInsetCoverage.h"
#include <cstdint>


namespace {
inline std::uint64_t TILEADDRKEY(std::uint32_t lev, std::uint32_t row, std::uint32_t col) {
  return ( (static_cast<std::uint64_t>(row) & 0xffffff)         |
           ((static_cast<std::uint64_t>(col) & 0xffffff) << 24) |
           ((static_cast<std::uint64_t>(lev) & 0x00001f) << 48));
}
}

// khCoverageMask
khCoverageMask::khCoverageMask(const khInsetCoverage &coverage,
                               bool set_covered)
    : begin_level_(coverage.beginLevel()),
      end_level_(coverage.endLevel()) {
  for (unsigned int i = begin_level_; i < end_level_; ++i) {
    levels_[i] = TransferOwnership(
        new khLevelPresenceMask(i, coverage.levelExtents(i), set_covered));
  }
}

khCoverageMask::khCoverageMask(const khCoverageMask &o)
    : begin_level_(o.begin_level_),
      end_level_(o.end_level_) {
  for (unsigned int i = begin_level_; i < end_level_; ++i) {
    levels_[i] = TransferOwnership(new khLevelPresenceMask(*o.levels_[i]));
  }
}

khCoverageMask& khCoverageMask::operator=(const khCoverageMask &o) {
  if (&o != this) {
    for (unsigned int i = begin_level_; i < end_level_; ++i) {
      levels_[i].clear();
    }
    begin_level_ = o.begin_level_;
    end_level_   = o.end_level_;
    for (unsigned int i = begin_level_; i < end_level_; ++i) {
      levels_[i] = TransferOwnership(new khLevelPresenceMask(*o.levels_[i]));
    }
  }
  return *this;
}

void khCoverageMask::PopulateCoverage(khInsetCoverage *cov) const {
  assert(cov != NULL);
  if (NumLevels() == 0)
    return;

  std::vector<khExtents<std::uint32_t> > extentsList;
  extentsList.reserve(NumLevels());
  for (unsigned int i = begin_level_; i < end_level_; ++i) {
    extentsList.push_back(levels_[i]->extents);
  }

  *cov = khInsetCoverage(begin_level_, end_level_, extentsList);
}

khExtents<std::uint32_t> khCoverageMask::LevelTileExtents(unsigned int lev) const {
  assert(lev < NumFusionLevels);
  if (levels_[lev]) {
    return levels_[lev]->extents;
  } else {
    return khExtents<std::uint32_t>();
  }
}

bool khCoverageMask::GetCovered(
    const khTileAddr &addr,
    std::set<int> *feature_id_set) const {
  assert(addr.level < NumFusionLevels);
  assert(feature_id_set != NULL);
  khTileAddr cur_addr = addr;

  if (cur_addr.level >= end_level_) {
    // reach up to maxLevel (end_level_-1) and start checking from there.
    cur_addr = cur_addr.MinifiedToLevel(end_level_ - 1);
  }

  // Check all the levels from current to begin_level_.
  while (levels_[cur_addr.level]) {
    if (levels_[cur_addr.level]->GetPresence(cur_addr.row, cur_addr.col)) {
      std::pair<TileAddrToFeatureIdMap::const_iterator,
                TileAddrToFeatureIdMap::const_iterator> range =
          tile_addr_to_feature_id_map.equal_range(TILEADDRKEY(
              cur_addr.level, cur_addr.row, cur_addr.col));
      for (TileAddrToFeatureIdMap::const_iterator it = range.first;
          it != range.second; ++it) {
        feature_id_set->insert(it->second);
      }
    }

    if (cur_addr.level <= begin_level_) {
      break;
    }

    cur_addr.minifyBy(1);
  }
  return (!feature_id_set->empty());
}

void khCoverageMask::SetCovered(const khTileAddr &addr,
                                int feature_id) {
  assert(addr.level < NumFusionLevels);
  assert(levels_[addr.level]);

  levels_[addr.level]->SetPresence(
      addr.row, addr.col, true /* covered */,
      Int2Type<true>() /* specialization for coverage mask */);

  tile_addr_to_feature_id_map.insert(
      std::make_pair(TILEADDRKEY(addr.level, addr.row, addr.col), feature_id));
}
