/*
 * Copyright 2017 Google Inc.
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


#ifndef __khInsetCoverage_h
#define __khInsetCoverage_h

#include "khTileAddr.h"

// ****************************************************************************
// ***  khInsetCoverage
// ***
// ***  Represents tile coverage across multiple levels. This class provides
// ***  no interpretation of the level numbers, tile sizes, etc. It just
// ***  holds the tile extents per level. Some code may use this for pyramid
// ***  tile coverage, others for ff tile coverage, etc.
// ***
// ***  Some constructors take a khTilespace argument. They use it to help with
// ***  the initial determination of the inset coverage, but the class DOES NOT
// ***  remember the tilespace. Almost all code that uses khInsetCoverage will
// ***  be explicitly build to deal with just one tilespace. To store it would
// ***  be both a waste and a source of confusion.
// ****************************************************************************
class khInsetCoverage {
 public:
  inline uint beginLevel(void) const { return beginLevel_; }
  inline uint endLevel(void)   const { return endLevel_; }
  inline uint numLevels(void)  const { return endLevel() - beginLevel(); }
  inline bool hasLevel(uint lev) const {
    return ((lev >= beginLevel()) && (lev < endLevel()));
  }
  inline bool intersectsLevels(uint bLevel, uint eLevel) const {
    return (std::min(eLevel, endLevel()) > std::max(bLevel, beginLevel()));
  }
  inline bool hasTile(const khTileAddr &addr) const {
    return (hasLevel(addr.level) &&
            extentsRef(addr.level).ContainsRowCol(addr.row, addr.col));
  }
  inline const khExtents<uint32>& levelExtents(uint lev) const {
    return extentsRef(lev);
  }
  inline khLevelCoverage levelCoverage(uint lev) const {
    return khLevelCoverage(lev, extentsRef(lev));
  }

  khInsetCoverage GetSubset(uint subsetThis, uint subsetTotal) const;

  // Narrow this coverage based on the range of other coverages supplied.
  // This is used to reduce work that we know will be redone later.
  template <class CovIter>
  bool Narrow(CovIter begin, CovIter end);

  // used only for serializing to XML
  const std::vector<khExtents<uint32> >
  RawExtentsVec(void) const {
    if (numLevels()) {
      khInsetCoverage *self = const_cast<khInsetCoverage*>(this);
      return std::vector<khExtents<uint32> >
        (&self->extentsVec[vecIndex(beginLevel())],
         &self->extentsVec[vecIndex(endLevel())]);
    } else {
      return std::vector<khExtents<uint32> >();
    }
  }

  bool operator==(const khInsetCoverage& o) const {
    if ((beginLevel() != o.beginLevel()) ||
        (endLevel()   != o.endLevel())) {
      return false;
    }
    for (uint lev = beginLevel(); lev < endLevel(); ++lev) {
      if (levelExtents(lev) != o.levelExtents(lev)) {
        return false;
      }
    }
    return true;
  }

 private:
  uint   vec0Level;   /* level of extents in first vector slot */

  uint   beginLevel_; /* first valid level */
  /* This will be the same as vec0Level unless the
     coverage has been narrowed with a call to Narrow */

  uint   endLevel_;   /* one beyond last valid level */

  khExtents<double> degree_extents_;

  std::vector<khExtents<uint32> > extentsVec;

  inline uint vecIndex(uint lev) const { return lev - vec0Level; }
  inline khExtents<uint32>& extentsRef(uint lev) {
    return extentsVec[vecIndex(lev)];
  }
  inline const khExtents<uint32>& extentsRef(uint lev) const {
    return extentsVec[vecIndex(lev)];
  }

  // called from constructors to populate levels
  void PopulateLevels(const khTilespace &tilespace,
                      const khLevelCoverage &maxCov,
                      uint stepOutSize,
                      uint paddingSize);
  void PopulateLevels(const khTilespace &tilespace,
                      uint fullresTileLevel,
                      uint stepOutSize,
                      uint paddingSize);
  void PopulateLevels(const khExtents<double> &normExtents,
                      const khTilespace &tilespace,
                      uint fullresTileLevel,
                      uint stepOutSize,
                      uint paddingSize);

 public:
  inline const khExtents<double>& degreeExtents() const {
    return degree_extents_;
  }
  inline khInsetCoverage(void) : vec0Level(0), beginLevel_(0), endLevel_(0){}

  // Build an inset coverage from a simple khLevelCoverage
  khInsetCoverage(const khLevelCoverage &levCov);

  // Build an inset coverage from a group of extents
  khInsetCoverage(uint beginCoverageLevel,
                  uint endCoverageLevel,
                  const std::vector<khExtents<uint32> > &extentsList);

  // Build an inset coverage from degree extents
  khInsetCoverage(const khTilespace &tilespace,
                  const khExtents<double> &degExtents,
                  uint fullresTileLevel,
                  uint beginCoverageLevel,
                  uint endCoverageLevel);

  // Build an inset coverage from nomalized extents
  // NOTE: order of extents and tilespace are reversed to distinguish
  // from constructor with degExtents
  khInsetCoverage(const khExtents<double> &normExtents,
                  const khTilespace &tilespace,
                  uint fullresTileLevel,
                  uint beginCoverageLevel,
                  uint endCoverageLevel);

  // Build an inset coverage from khLevelCoverage & level range
  khInsetCoverage(const khTilespace &tilespace,
                  const khLevelCoverage &levCov,
                  uint beginCoverageLevel,
                  uint endCoverageLevel /* one beyond */);

  // Build an inset coverage from degree extents
  // stepOutSize is cummulative. Subsequent layer minifications
  // will be done on the stepped out extents. (used for terrain)
  // paddingSize is NOT cummulative. Subsequent layer minifications
  // will be done on the "pre padded" extents. (used for imagery)
  // Note: It probably doesn't make sense to specify both a stepout and
  // a padding, but the code doesn't prevent it.
  khInsetCoverage(const khTilespace &tilespace,
                  const khExtents<double> &degExtents,
                  uint fullresTileLevel,
                  uint beginCoverageLevel,
                  uint endCoverageLevel /* one beyond */,
                  uint stepOutSize,
                  uint paddingSize);

};


// Iterator to expose the others' level extents w/o copying
template <class CovIter>
class ExtentsIterator {
  CovIter cIter;  // current
  CovIter eIter;  // end
  uint level;
 public:
  inline ExtentsIterator(CovIter c, CovIter e, uint l) :
      cIter(c), eIter(e), level(l) {

    // make sure our first one is valid
    while ((cIter != eIter) && !cIter->hasLevel(level)) {
      ++cIter;
    }
  }

  inline const khExtents<double>* degreeExtents() const {
    return &cIter->degreeExtents();
  }

  inline const khExtents<uint32>* operator->(void) const {
    return &cIter->levelExtents(level);
  }
  inline const khExtents<uint32>& operator*(void) const {
    return *operator->();
  }

  inline ExtentsIterator operator++(void) {
    ++cIter;
    while ((cIter != eIter) && !cIter->hasLevel(level)) {
      ++cIter;
    }
    return *this;
  }
  inline bool operator==(const ExtentsIterator &o) const {
    return ((cIter == o.cIter) && (level == o.level));
  }
  inline bool operator!=(const ExtentsIterator &o) const {
    return !operator==(o);
  }
};


// Narrow this coverage based on the range of other coverages supplied.
// This is used to reduce work that we know will be redone later.
template <class CovIter>
bool
khInsetCoverage::Narrow(CovIter beginOthers, CovIter endOthers)
{
  // For now we only try to narrow coverage by removing the lower resolution
  // levels that will be completely re-done by the others. Eventually, we
  // could also reduce the remaining levels if we know that part of them
  // are going to be redone. If we ever make those changes here, we'll need
  // to also mosify the FindNeeded*Insets to look at all levels and not
  // just the min-resolution level.

  if (beginOthers == endOthers) {
    // there are no others - there's no way to narrow
    return false;
  }


  // check each level (from lowres to highres) - stop when the others
  // don't completely cover this one.
  uint level = beginLevel();
  while (level < endLevel()) {
    if (levelExtents(level).CoveredBy(ExtentsIterator<CovIter>
                                      (beginOthers, endOthers, level),
                                      ExtentsIterator<CovIter>
                                      (endOthers,   endOthers, level),
                                      degreeExtents())) {
      ++level;
    } else {
      break;
    }
  }
  // we've eliminated some low res levels, modify our beginLevel_
  if (level != beginLevel()) {
    beginLevel_ = level;
    return true;
  }

  return false;
}

#endif /* __khInsetCoverage_h */
