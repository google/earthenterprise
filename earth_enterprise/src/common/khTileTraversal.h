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

#ifndef __khTileTraversal_h
#define __khTileTraversal_h


// ****************************************************************************
// ***  Helper function - use public API below
// ****************************************************************************
template <class Operation>
void TileAlignedTraversalImpl_(const khLevelCoverage &currLevelCoverage,
                               const khInsetCoverage &insetCoverage,
                               Operation &op)
{
  unsigned int currLevel = currLevelCoverage.level;
  assert(insetCoverage.hasLevel(currLevel));
  assert(insetCoverage.levelExtents(currLevel)
         .contains(currLevelCoverage.extents));

  if (currLevel + 1 == insetCoverage.endLevel()) {
    // We're at the bottom level, process the intersected tiles
    for (std::uint32_t row = currLevelCoverage.extents.beginRow();
         row < currLevelCoverage.extents.endRow(); ++row) {
      for (std::uint32_t col = currLevelCoverage.extents.beginCol();
           col < currLevelCoverage.extents.endCol(); ++col) {
        op(khTileAddr(currLevel, row, col));
      }
    }
  } else {
    // We're not at the bottom yet, recurse until we are
    for (std::uint32_t row = currLevelCoverage.extents.beginRow();
         row < currLevelCoverage.extents.endRow(); ++row) {
      for (std::uint32_t col = currLevelCoverage.extents.beginCol();
           col < currLevelCoverage.extents.endCol(); ++col) {
        khTileAddr thisAddr(currLevel, row, col);
        TileAlignedTraversalImpl_
          (khLevelCoverage(currLevel+1,
                           khExtents<std::uint32_t>::Intersection
                           (insetCoverage.levelExtents(currLevel+1),
                            thisAddr.MagnifiedBy(1).extents)),
           insetCoverage,
           op);
      }
    }
  }
}

// ****************************************************************************
// ***  Traverse the targetCoverage calling op(addr) for each addr. The
// ***  traversal order is aligned on tile boundaries. beginAlignLevel is the
// ***  lowest resolution level where you care to preserve alignment.
// ****************************************************************************
template <class Operation>
void TileAlignedTraversal(const khTilespace &tilespace,
                          const khLevelCoverage &targetCoverage,
                          unsigned int beginAlignLevel,
                          Operation op)
{
  khInsetCoverage insetCoverage(tilespace,
                                targetCoverage,
                                beginAlignLevel,
                                targetCoverage.level+1);

  TileAlignedTraversalImpl_(insetCoverage.levelCoverage(beginAlignLevel),
                            insetCoverage, op);
}


// ****************************************************************************
// ***  Traverse targetCoverage calling op(addr) for each addr. The traversal
// ***  order is a diagonal from top right to bottom left
// ***
// *** 22 16 11 07 04 02 01
// *** 28 23 17 12 08 05 03
// *** 33 29 24 18 13 09 06
// *** 37 34 30 25 19 14 10
// *** 40 38 35 31 26 20 15
// *** 42 41 39 36 32 27 21
// ****************************************************************************
template <class Operation>
void TopRightToBottomLeftTraversal(const khLevelCoverage &targetCoverage,
                                   Operation op)
{
  const khExtents<std::uint32_t> &extents(targetCoverage.extents);
  std::uint32_t w = extents.width();
  std::uint32_t h = extents.height();
  for (unsigned int i = 0; i < w + h - 1; ++i) {
    std::uint32_t row, col, num;
    if (i < w) {
      col = extents.endCol() - (i + 1);
      row = extents.endRow() - 1;
      num = std::min(h, i + 1);
    } else {
      col = extents.beginCol();
      row = (extents.endRow() - 1) - (i - w + 1);
      num = std::min(w, row - extents.beginRow() + 1);
    }
    for (unsigned int j = 0; j < num; ++j) {
      op(khTileAddr(targetCoverage.level, row, col));
      ++col;
      --row;
    }
  }
}

#endif /* __khTileTraversal_h */
