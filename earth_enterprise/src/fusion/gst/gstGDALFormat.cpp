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


#include <string>

#include <khFileUtils.h>
#include <khException.h>

#include <gstGDALFormat.h>
#include <gstRecord.h>
#include <gstSpatialRef.h>

#include <khgdal/khGDALDataset.h>

gstGDALFormat::gstGDALFormat(const char *n)
    : gstFormat(n) {
}

gstGDALFormat::~gstGDALFormat() {
}

gstStatus gstGDALFormat::OpenFile() {
  try {
    khGDALDataset fileData(name());
    khExtents<double> degExtents =
      fileData.normalizedGeoExtents().extents();
    notify(NFY_DEBUG,
           "  normalized extents (nswe) %.8lf,%.8lf,%.8lf,%.8lf",
           degExtents.north(), degExtents.south(),
           degExtents.west(),  degExtents.east());

    // GetKeyholeNormalizeGeoExtents will always return extents
    // in WGS84
    OGRSpatialReference srs;
    srs.SetWellKnownGeogCS("WGS84");
    SetSpatialRef(new gstSpatialRef(&srs));

    coord_box_.init(degExtents.west(),  degExtents.east(),
                    degExtents.south(), degExtents.north());
    if (!coord_box_.Valid()) {
      throw khSoftException(kh::tr("Invalid bounding box: %1")
                            .arg(name()));
    }

    AddLayer(gstPolyLine, 1, gstHeaderImpl::Create());

    return GST_OKAY;
  } catch(const std::exception &e) {
    // exception msg will contain filename and error description
    notify(NFY_WARN, "%s", e.what());
  } catch(...) {
    notify(NFY_WARN, "Unknown error opening %s", name());
  }
  return GST_OPEN_FAIL;
}

gstGeodeHandle gstGDALFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fidx < NumFeatures(layer));

  return gstGeodeImpl::Create(coord_box_);
}

gstRecordHandle gstGDALFormat::GetAttributeImpl(std::uint32_t, std::uint32_t) {
  throw khException(kh::tr("No attributes available"));
}

