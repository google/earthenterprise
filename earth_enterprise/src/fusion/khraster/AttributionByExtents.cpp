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


#include "AttributionByExtents.h"
#include <khSimpleException.h>
#include <khGuard.h>
#include <khraster/khRasterProduct.h>
#include <khTileAddr.h>
#include <common/khConstants.h>

AttributionByExtents::Inset::Inset(const std::string &rppath, std::uint32_t id, const std::string &acquisitionDate) :
    inset_id_(id)
{
  khDeleteGuard<khRasterProduct> rp(khRasterProduct::Open(rppath));
  if (!rp) {
    throw khSimpleException("Unable to open raster product ") << rppath;
  }
  max_level_   = rp->maxLevel();
  deg_extents_ = rp->degOrMeterExtents();
  if (acquisitionDate != kUnknownDateTimeUTC && acquisitionDate != kUnknownDate) {
    acquisition_date_ = acquisitionDate;
  } else {
    acquisition_date_ = rp->GetAcquisitionDate();
  }
}

 std::uint32_t AttributionByExtents::GetInternalInsetIndex(const khTileAddr &addr)
const {
  std::uint32_t maxid = 0;
  std::uint32_t maxlevel = 0;
  double maxarea = 0.0;

  khExtents<double> tile_extents = addr.degExtents(ClientImageryTilespaceBase);
  double tile_area = tile_extents.width() * tile_extents.height();

  // Traverse the insets from highest resolution to lowest resolution
  for (unsigned int j = 0; j < insets.size(); ++j) {
    unsigned int idx = insets.size()-j-1;

    // If there is a higher res inset w/ at least 33% coverage
    // give it ownership of the tile
    if ((insets[idx].max_level_ < maxlevel) &&
        (maxarea / tile_area > 0.33)) {
      return maxid;
    }

    // calculate the intersection area of this inset
    khExtents<double> intersect =
      khExtents<double>::Intersection(tile_extents,
                                      insets[idx].deg_extents_);
    double area = intersect.width() * intersect.height();

    if (area / tile_area > 0.5) {
      // if this inset covers more than 50%, give it ownership
      return idx;
    } else if (area > maxarea) {
      // if this set covers more than any other yet, mark it as
      // a candidate for ownership
      maxid = idx;
      maxarea = area;
      maxlevel = insets[idx].max_level_;
    }
  }

  return maxid;
}
 std::uint32_t AttributionByExtents::GetInsetId(const khTileAddr &addr) const {
  std::uint32_t max_index = GetInternalInsetIndex(addr);
  if (max_index < insets.size()) {
    return insets[max_index].inset_id_;
  }
  return 0;
}

// Return the acquisition date for the most significant inset for the
// specified tile.
const std::string& AttributionByExtents::GetAcquisitionDate(
  const khTileAddr &addr) const {
  std::uint32_t max_index = GetInternalInsetIndex(addr);

  if (max_index < insets.size()) {
    return insets[max_index].acquisition_date_;
  }
  return kEmptyString;
}
