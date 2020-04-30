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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHPRESENCEMASK_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHPRESENCEMASK_H_

#include "common/khLevelPresenceMask.h"
#include "common/khTileAddr.h"
#include "common/geFileUtils.h"
#include "common/khGuard.h"
//#include "common/khTypes.h"
#include <cstdint>

class khInsetCoverage;

// ****************************************************************************
// ***  khPresenceMask
// ****************************************************************************
class khPresenceMask {
 public:
  // for copying another
  khPresenceMask(const khPresenceMask &);
  khPresenceMask& operator=(const khPresenceMask &);

  // load mask from file - will throw exception
  explicit khPresenceMask(const std::string &filename);

  // set_present - whether build levels with all "not present" or all "present".
  khPresenceMask(const khInsetCoverage &coverage, bool set_present = false);

  // if asked for tileAddr outside the stored level range it will
  // always return false
  inline bool GetPresence(const khTileAddr &addr) const {
    assert(addr.level < NumFusionLevels);
    if (levels[addr.level]) {
      return levels[addr.level]->GetPresence(addr.row, addr.col);
    } else {
      return false;
    }
  }

  // if asked to check a tile outside the stored level range it will
  // estimate the presence based on the levels it does have
  // false return is definitive
  // true return may just be an estimate
  bool GetEstimatedPresence(const khTileAddr &addr) const;

  // Set presence for all elements of specified level.
  void SetPresence(unsigned int lev, bool set_present) {
    assert(levels[lev]);
    levels[lev]->SetPresence(set_present);
  }

  inline void SetPresence(const khTileAddr &addr, bool present) {
    assert(addr.level < NumFusionLevels);
    assert(levels[addr.level]);
    levels[addr.level]->SetPresence(
        addr.row, addr.col, present, Int2Type<false>());
  }

  // Set present for tile specified by addr and for tiles of higher levels,
  // that contains this tile, cascading up.
  // addr not passed by ref, we're going to play with it.
  void SetPresenceCascade(khTileAddr addr);

  // fill in coverage object with my coverage
  void PopulateCoverage(khInsetCoverage &cov) const;

  bool ValidLevel(unsigned int level) const {
    return ((level >= beginLevel) && (level < endLevel));
  }
  khExtents<std::uint32_t> levelTileExtents(unsigned int level) const;


  unsigned int numLevels;
  unsigned int beginLevel;
  unsigned int endLevel;
  khDeleteGuard<khLevelPresenceMask> levels[NumFusionLevels];
};  // class PresenceMask



// ****************************************************************************
// ***  khPresenceMaskWriter
// ****************************************************************************
class khPresenceMaskWriter {
 private:
  khPresenceMask  presence;
  // Proxy file which is copied to the requested file on destruction.
  geLocalBufferWriteFile proxyFile;
  khMunmapGuard munmapper;

  std::uint32_t DataOffset(void);
  std::uint32_t TotalFileSize(void);

 public:
  khPresenceMaskWriter(const std::string &filename,
                     const khInsetCoverage &coverage);
  ~khPresenceMaskWriter(void);

  inline void SetPresence(const khTileAddr &addr, bool present) {
    assert(addr.level < NumFusionLevels);
    assert(presence.levels[addr.level]);
    presence.levels[addr.level]->SetPresence(
        addr.row, addr.col, present, Int2Type<false>());
  }
  inline bool GetPresence(const khTileAddr &addr) const {
    return presence.GetPresence(addr);
  }
  inline const khPresenceMask& GetPresenceMask(void) const {
    return presence;
  }
};





#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHPRESENCEMASK_H_
