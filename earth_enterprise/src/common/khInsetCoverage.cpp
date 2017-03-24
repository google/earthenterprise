// Copyright 2017 Google Inc.
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


#include "khInsetCoverage.h"
#include <khException.h>

khInsetCoverage
khInsetCoverage::GetSubset(uint subsetThis, uint subsetTotal) const
{
  // By virtue of the range of uint, this check also ensures that
  // subsetTotal != 0
  if (subsetThis >= subsetTotal) {
    notify(NFY_WARN, "Internal Error: Invalid subset specification");
    return khInsetCoverage();
  }

  std::vector<khExtents<uint32> > newExtents;
  newExtents.reserve(numLevels());

  for (uint level = beginLevel(); level < endLevel(); ++level) {
    newExtents.push_back(levelCoverage(level)
                         .GetSubset(subsetThis, subsetTotal)
                         .extents);
  }

  return khInsetCoverage(beginLevel(), endLevel(), newExtents);
}


// Build an inset coverage from a simple khLevelCoverage
khInsetCoverage::khInsetCoverage(const khLevelCoverage &levCov) :
    vec0Level(levCov.level),
    beginLevel_(levCov.level),
    endLevel_(levCov.level+1)
{
  extentsVec.push_back(levCov.extents);
}

// Build an inset coverage from a group of extents
khInsetCoverage::khInsetCoverage(uint beginCoverageLevel,
                                 uint endCoverageLevel,
                                 const std::vector<khExtents<uint32> > &extentsList)
    : vec0Level(beginCoverageLevel),
      beginLevel_(beginCoverageLevel),
      endLevel_(endCoverageLevel),
      extentsVec(extentsList)
{
  if (numLevels() != extentsVec.size()) {
    throw khException(kh::tr("Internal Error: Invalid inset coverage params"));
  }
}

// Build an inset coverage from degree extents
void
khInsetCoverage::PopulateLevels(const khTilespace &tilespace,
                                const khLevelCoverage &levCov,
                                uint stepOutSize,
                                uint paddingSize)
{
  if (beginLevel() >= endLevel()) {
    // we don't want any levels, so just return now
    return;
  }

  // get level coverage I can play with
  khLevelCoverage tmpCov(levCov);

  // scale it to the maximum level requested
  tmpCov.scaleToLevel(endLevel()-1);

  // make sure the extents don't step outside the valid range for this level
  tmpCov.cropToWorld(tilespace);

  extentsVec.resize(numLevels()); // presize so I can fill from the back

  // add it to my levels
  if (paddingSize > 0) {
    khLevelCoverage padCov(tmpCov);
    padCov.extents.expandBy(paddingSize);
    padCov.cropToWorld(tilespace);
    extentsRef(tmpCov.level) = padCov.extents;
  } else {
    extentsRef(tmpCov.level) = tmpCov.extents;
  }

  while (tmpCov.level > beginLevel()) {
    if (stepOutSize > 0) {
      // expand the previous extents (step out).
      tmpCov.extents.expandBy(stepOutSize); // protects against wrapping

      // make sure the expanded extents don't step outside the valid
      // range for this level
      tmpCov.cropToWorld(tilespace);
    }

    // minify the previous extents
    tmpCov.minifyBy(1);

    // add it to my levels
    if (paddingSize > 0) {
      khLevelCoverage padCov(tmpCov);
      padCov.extents.expandBy(paddingSize);
      padCov.cropToWorld(tilespace);
      extentsRef(tmpCov.level) = padCov.extents;
    } else {
      extentsRef(tmpCov.level) = tmpCov.extents;
    }
  }
}

void
khInsetCoverage::PopulateLevels(const khTilespace &tilespace,
                                uint fullresTileLevel,
                                uint stepOutSize,
                                uint paddingSize)
{
  if (beginLevel() >= endLevel()) {
    // we don't want any levels, so just return now
    return;
  }

  // Use the maximum of fullres and endLevel-1 as the fullres level.
  // That way we always minify (so we can do the appropriate step out)
  // Note: check above guarantees that endLevel > 0
  fullresTileLevel = std::max(fullresTileLevel, endLevel()-1);

  // get level coverage for highest res level that was requested
  khLevelCoverage maxCov(tilespace, degreeExtents(),
                         fullresTileLevel,
                         endLevel() - 1 /* targetlevel */);

  PopulateLevels(tilespace, maxCov, stepOutSize, paddingSize);
}


void
khInsetCoverage::PopulateLevels(const khExtents<double> &normExtents,
                                const khTilespace &tilespace,
                                uint fullresTileLevel,
                                uint stepOutSize,
                                uint paddingSize)
{
  if (beginLevel() >= endLevel()) {
    // we don't want any levels, so just return now
    return;
  }

  // Use the maximum of fullres and endLevel-1 as the fullres level.
  // That way we always minify (so we can do the appropriate step out)
  // Note: check above guarantees that endLevel > 0
  fullresTileLevel = std::max(fullresTileLevel, endLevel()-1);

  // get level coverage for highest res level that was requested
  khLevelCoverage maxCov
    = khLevelCoverage::FromNormExtents(tilespace, normExtents,
                                       fullresTileLevel,
                                       endLevel() - 1 /* targetlevel */);

  PopulateLevels(tilespace, maxCov, stepOutSize, paddingSize);
}


// Build an inset coverage from degree extents
khInsetCoverage::khInsetCoverage(const khTilespace &tilespace,
                                 const khExtents<double> &degExtents,
                                 uint fullresTileLevel,
                                 uint beginCoverageLevel,
                                 uint endCoverageLevel) :
    vec0Level(beginCoverageLevel),
    beginLevel_(beginCoverageLevel),
    endLevel_(endCoverageLevel),
    degree_extents_(degExtents)
{
  PopulateLevels(tilespace,
                 fullresTileLevel,
                 0 /* stepOutSize */,
                 0 /* paddingSize */);
}

khInsetCoverage::khInsetCoverage(const khExtents<double> &normExtents,
                                 const khTilespace &tilespace,
                                 uint fullresTileLevel,
                                 uint beginCoverageLevel,
                                 uint endCoverageLevel) :
    vec0Level(beginCoverageLevel),
    beginLevel_(beginCoverageLevel),
    endLevel_(endCoverageLevel)
{
  PopulateLevels(normExtents,
                 tilespace,
                 fullresTileLevel,
                 0 /* stepOutSize */,
                 0 /* paddingSize */);
}

// Build an inset coverage from degree extents (w/ stepout and padding)
khInsetCoverage::khInsetCoverage(const khTilespace &tilespace,
                                 const khExtents<double> &degExtents,
                                 uint fullresTileLevel,
                                 uint beginCoverageLevel,
                                 uint endCoverageLevel,
                                 uint stepOutSize,
                                 uint paddingSize) :
    vec0Level(beginCoverageLevel),
    beginLevel_(beginCoverageLevel),
    endLevel_(endCoverageLevel),
    degree_extents_(degExtents)

{
  PopulateLevels(tilespace,
                 fullresTileLevel,
                 stepOutSize,
                 paddingSize);
}

// Build an inset coverage from khLevelCoverage & level range
khInsetCoverage::khInsetCoverage(const khTilespace &tilespace,
                                 const khLevelCoverage &levCov,
                                 uint beginCoverageLevel,
                                 uint endCoverageLevel) :
    vec0Level(beginCoverageLevel),
    beginLevel_(beginCoverageLevel),
    endLevel_(endCoverageLevel)
{
  PopulateLevels(tilespace,
                 levCov,
                 0 /* stepOutSize */,
                 0 /* paddingSize */);
}
