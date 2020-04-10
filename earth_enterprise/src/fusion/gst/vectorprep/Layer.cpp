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


#include "Layer.h"
#include "Binders.h"
#include <gstSourceManager.h>
#include <gstSource.h>
#include <khException.h>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <khTileAddr.h>

namespace vectorprep {

template <class DisplayRuleConfig>
Layer<DisplayRuleConfig>::Layer
(const gstSharedSource &source,
 const khTilespace &tilespace_, double oversizeFactor_,
 const std::vector<DisplayRuleConfig> &drConfigs,
 const QStringList &jsContextScripts) :
    tilespace(tilespace_),
    oversizeFactor(oversizeFactor_)
{
  // get the srcHeader
  srcHeader = source->GetAttrDefs(0 /* vector products only have one layer */);
  if (!srcHeader) {
    throw khException("Unable to retrieve layer source header");
  }

  // make a JS context (only if we really need it)
  bool hasPrepJS = false;
  for (unsigned int i = 0; i < drConfigs.size(); ++i) {
    if (drConfigs[i].HasPrepJS()) {
      hasPrepJS = true;
      break;
    }
  }
  if (hasPrepJS) {
    jsCX = gstRecordJSContextImpl::Create(srcHeader, jsContextScripts);
  }

  // make my child DisplayRules
  displayRules.reserve(drConfigs.size());
  for (unsigned int i = 0; i < drConfigs.size(); ++i) {
    displayRules.push_back(new DisplayRuleType(source,
                                               srcHeader, jsCX,
                                               drConfigs[i]));
  }
}

template <class DisplayRuleConfig>
Layer<DisplayRuleConfig>::~Layer(void)
{
}



template <class DisplayRuleConfig>
bool
Layer<DisplayRuleConfig>::Prepare
(const QuadtreePath &path,
 LayerTileType *out,
 const vectorquery::LayerTile &in)
{
  assert(in.displayRules.size() == displayRules.size());

  // compute my box for cutting features
  // khLevelCoverage cov(khTileAddr(path));
  khTileAddr addr(path);
  khLevelCoverage cov(addr);

  khExtents<std::uint64_t> pixels(RowColOrder,
                           cov.extents.beginRow() * tilespace.tileSize,
                           cov.extents.endRow() * tilespace.tileSize,
                           cov.extents.beginCol() * tilespace.tileSize,
                           cov.extents.endCol() * tilespace.tileSize);

  // Stretch in width, delta_width = width * oversizeFactor
  // Stretch in width in each dir = delta_width / 2.0
  pixels.expandBy(static_cast<std::uint32_t>(tilespace.tileSize * oversizeFactor / 2));

  double north, south, east, west;

  if (tilespace.projection_type == khTilespace::MERCATOR_PROJECTION) {
    Projection::LatLng lower_left = tilespace.projection->FromPixelToLatLng(
        Projection::Point(pixels.beginX(), pixels.beginY()), path.Level());
    Projection::LatLng upper_right = tilespace.projection->FromPixelToLatLng(
        Projection::Point(pixels.endX(), pixels.endY()), path.Level());

    west = khTilespace::Normalize(lower_left.Lng());
    east = khTilespace::Normalize(upper_right.Lng());
    south = khTilespace::Normalize(lower_left.Lat());
    north = khTilespace::Normalize(upper_right.Lat());
  } else {
    double norm_pixel_size = tilespace.NormPixelSize(path.Level());
    west = static_cast<double>(pixels.beginCol() * norm_pixel_size),
    east = static_cast<double>(pixels.endCol() * norm_pixel_size),
    south = static_cast<double>(pixels.beginRow() * norm_pixel_size),
    north = static_cast<double>(pixels.endRow() * norm_pixel_size);
  }

  gstBBox cut_box(west, east, south, north);

  // have all display rules Prepare themselves
  bool found = false;
  for (unsigned int i = 0; i < displayRules.size(); ++i) {
    if (displayRules[i]->Prepare(cut_box, path.Level(),
                                 out->displayRules[i],
                                 in.displayRules[i])) {
      found = true;
    }
  }

  //  notify(NFY_VERBOSE, "LayerPrep lrc(%u,%u,%u): found = %s",
  //     level, row, col, found ? "true" : "false");
  return found;
}


} // namespace vectorprep


// ****************************************************************************
// ***  Explicit instantiations of Layer<DisplayRuleConfig>
// ****************************************************************************
#include "DisplayRuleTmpl.h"

template class vectorprep::Layer<MapDisplayRuleConfig>;
