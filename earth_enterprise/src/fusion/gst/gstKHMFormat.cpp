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


#include <gstKHMFormat.h>
#include <gstRecord.h>
#include <gstBBox.h>
#include <khgdal/.idl/khVirtualRaster.h>
#include <khFileUtils.h>

gstKHMFormat::gstKHMFormat(const char* n)
    : gstFormat(n) {
}

gstStatus gstKHMFormat::OpenFile() {
  // load the XML - these are by definition quite small, slurp the whole thing
  khVirtualRaster virtraster;
  if (khHasExtension(name(), ".khm")) {
    khMosaic mosaic;
    if (!mosaic.Load(name())) {
      notify(NFY_WARN, "Error opening %s: %s", FormatName(), name());
      return GST_OPEN_FAIL;
    }
    virtraster = khVirtualRaster(mosaic);
  } else {
    if (!virtraster.Load(name())) {
      notify(NFY_WARN, "Error opening %s: %s", FormatName(), name());
      return GST_OPEN_FAIL;
    }
  }

  // Setup our Spatial Reference System
  OGRSpatialReference srs(virtraster.srs.c_str());
  SetSpatialRef(new gstSpatialRef(&srs));


  // add the extents polygon to its own layer
  gstHeaderHandle extHeader = gstHeaderImpl::Create();
  extHeader->addSpec("filename", gstTagString);
  extHeader->addSpec("numtiles", gstTagInt);
  gstRecordHandle attrib = extHeader->NewRecord();
  extents_attribs_.push_back(attrib);
  attrib->Field(0)->set(name());
  attrib->Field(1)->set(virtraster.inputTiles.size());
  AddLayer(gstPolyLine, 1, extHeader);

  double originX = virtraster.cropOriginX();
  double originY = virtraster.cropOriginY();
  gstBBox extents(originX,
                  originX +
                  virtraster.cropExtents.width()*virtraster.pixelsize.width,
                  originY,
                  originY +
                  virtraster.cropExtents.height()*virtraster.pixelsize.height);
  extents_geometry_.push_back(extents);

  // add the tiles to their own layer
  gstHeaderHandle tileHeader = gstHeaderImpl::Create();
  tileHeader->addSpec("filename", gstTagString);
  PopulateTiles(virtraster, tileHeader);
  AddLayer(gstPolyLine, tiles_geometry_.size(), tileHeader);

  return GST_OKAY;
}

gstStatus gstKHMFormat::CloseFile() {
  extents_geometry_.clear();
  extents_attribs_.clear();
  tiles_geometry_.clear();
  tiles_attribs_.clear();

  return GST_OKAY;
}

gstGeodeHandle gstKHMFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(fidx < NumFeatures(layer));

  switch (layer) {
    case 0:
      return gstGeodeImpl::Create(extents_geometry_[fidx]);
    case 1:
      return gstGeodeImpl::Create(tiles_geometry_[fidx]);
  }

  // unreached, but silences warnings
  return gstGeodeHandle();
}

gstRecordHandle gstKHMFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(fidx < NumFeatures(layer));

  switch (layer) {
    case 0:
      return extents_attribs_[ fidx ];
    case 1:
      return tiles_attribs_[fidx];
  }

  // unreached, but silences warnings
  return gstRecordHandle();
}

void gstKHMFormat::PopulateTiles(const khVirtualRaster &virtraster,
                                 gstHeaderHandle &header) {
  // Process the tiles adding geodes for each of them
  for (std::vector<khVirtualRaster::InputTile>::const_iterator iter =
         virtraster.inputTiles.begin();
       iter < virtraster.inputTiles.end();
       ++iter) {
    gstRecordHandle attrib = header->NewRecord();
    tiles_attribs_.push_back(attrib);
    attrib->Field(0)->set(iter->file.c_str());

    gstBBox extents(iter->origin.x(),
                    iter->origin.x() +
                    iter->rastersize.width*virtraster.pixelsize.width,
                    iter->origin.y(),
                    iter->origin.y() +
                    iter->rastersize.height*virtraster.pixelsize.height);
    tiles_geometry_.push_back(extents);
  }
}
