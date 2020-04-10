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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_KHGDALDATASET_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_KHGDALDATASET_H_

#include <cstdint>
#include <khGeomUtils.h>
#include <khGuard.h>
#include <string>
#include <gdal_priv.h>
#include "khGeoExtents.h"
#include <khRefCounter.h>
#include <ogr_spatialref.h>
#include <notify.h>
#include <khTileAddr.h>

// ****************************************************************************
// ***  khGDALDataset
// ***
// ***  Smart pointer around GDALDataset
// ***    - constructor & extended keyhole routines throw exceptions
// ***    - can be used almost like a GDALDataset*
// ***      - normal GDALDataset routines available through '->'
// ***      - extended keyhole routines available through '.'
// ***    - properly handles .geo files and  SRS/extents overrides
// ***      - wraps raw GDALDataset in a khAddGeoDataset so ALL other GDAL
// ***        routines can use the overridden geo spacial references
// ***    - remembers filename :-)
// ***
// ***  khGDALDataset srcDS(filename);
// ***  if (srcDS) {
// ***     notify(NFY_NOTICE, "numBands = %d", srcDS->GetRasterBandCount());
// ***     const khExtents<double> &extents(srcDS.geoExtents().extents());
// ***     notify(NFY_NOTICE, "extents = (nsew) %f,%f,%f,%f",
// ***            extents.north(), extents.south(),
// ***            extents.east(),  extents.west());
// ***  }
// ****************************************************************************

// implementation class - never used directly
class khGDALDatasetImpl : public khRefCounter
{
 public:
  friend class khGDALDataset;
  bool IsMercator() const {
    return projectionType == khTilespace::MERCATOR_PROJECTION;
  }

  khTilespace::ProjectionType GetProjectionType() const {
    return projectionType;
  }

  inline const std::string& GetFilename() const {
    return filename;
  }

 private:
  std::string filename;
  khDeleteGuard<GDALDataset> dataset;
  khDeleteGuard<GDALDataset> maskDS;
  bool checkedForMask;
  khGeoExtents               geoExtents;
  OGRSpatialReference        ogrSRS;

  bool haveNormalized;
  unsigned int             normTopLevel;

  // snapped to level and aligned with that level's pixels
  khExtents<std::int64_t> alignedPixelExtents;
  khGeoExtents     alignedGeoExtents;

  // aligned extents cropped to world - when the outside world
  // sees as my normalized extents
  khExtents<std::int64_t> croppedPixelExtents;
  khGeoExtents     croppedGeoExtents;

  bool           needReproject;
  bool           needSnapUp;

  khTilespace::ProjectionType projectionType;

  inline khSize<std::uint32_t> rasterSize(void) const {
    return khSize<std::uint32_t>(dataset->GetRasterXSize(),
                          dataset->GetRasterYSize());
  }

  // ALL THESE WILL THROW ON ERROR
  khGDALDatasetImpl(const std::string &filename_,
                    const std::string &overrideSRS,
                    const khExtents<double> &overrideExtents,
                    khTilespace::ProjectionType aProjectionType);

  void EnsureKeyholeNormalizedInfo(void);


  // When we reproject an image we also need to scale to ceil of nearest zoom
  // level. i.e if we get zoom level of 3.4 we need to snap up to zoom level 4.
  // So this method will find where the current target projection leads to in
  // zoom level and then snap it up to nearest zoom level.
  static void
  ComputeReprojectSnapupExtents(GDALDataset *srcDS,
                                const std::string &srcSRS,
                                const std::string &dstSRS,
                                khSize<std::uint32_t> &rasterSize,
                                khGeoExtents &geoExtents,
                                unsigned int         &level,
                                const bool   is_mercator);

  void
  AlignAndCropExtents(const unsigned int normTopLevel,
                      const khSize<std::uint32_t> &inRasterSize,
                      const khGeoExtents   &inGeoExtents,
                      bool is_mercator);

  GDALDataset* getMaskDS() const;
 public:
  // This method finds the pixel size (width & height) in degrees after
  // reprojecting srcDataSet from its projectionReference to
  // dstProjectionReference. The output is returned in the reference parameters
  // pixelWidthInDegrees and pixelHeightInDegrees. These values are >= 0 always.
  // Note: In case of errors it may throw exceptions.
  static void GetReprojectedPixelSizeInDegrees(
    GDALDataset* srcDataSet,
    double* pixelWidthInDegrees   /* OUT */,
    double* pixelHeightInDegrees  /* OUT */   );
};


class khGDALDataset
{
 private:
  khRefGuard<khGDALDatasetImpl> impl;

 public:
  bool IsMercator() const { return impl->IsMercator(); }

  void release(void) { impl = khRefGuard<khGDALDatasetImpl>(); }

  inline GDALDataset* operator->(void) const { return impl->dataset; }
  inline operator GDALDataset*(void) const {
    return impl ? (GDALDataset*)impl->dataset : 0;
  }
  inline GDALDataset* get() const { return impl->dataset; }

  // extended keyhole routines
  inline const std::string& filename(void) const { return impl->filename; }
  inline const khGeoExtents& geoExtents(void) const {
    return impl->geoExtents;
  }
  inline khSize<std::uint32_t> rasterSize(void) const {
    return impl->rasterSize();
  }
  inline const OGRSpatialReference& ogrSRS(void) const {
    return impl->ogrSRS;
  }

  // ALL THESE WILL THROW ON ERROR
  inline const khExtents<std::int64_t>& alignedPixelExtents(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->alignedPixelExtents;
  }
  inline const khExtents<std::int64_t>& croppedPixelExtents(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->croppedPixelExtents;
  }
  inline khSize<std::uint32_t> alignedRasterSize(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return khSize<std::uint32_t>(impl->alignedPixelExtents.width(),
                          impl->alignedPixelExtents.height());
  }
  inline khSize<std::uint32_t> croppedRasterSize(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return khSize<std::uint32_t>(impl->croppedPixelExtents.width(),
                          impl->croppedPixelExtents.height());
  }

  inline const khGeoExtents&   alignedGeoExtents(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->alignedGeoExtents;
  }
  inline const khGeoExtents&   croppedGeoExtents(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->croppedGeoExtents;
  }

  inline unsigned int normalizedTopLevel(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->normTopLevel;
  }
  inline khSize<std::uint32_t> normalizedRasterSize(void) const {
    return croppedRasterSize();
  }
  inline const khGeoExtents&   normalizedGeoExtents(void) const {
    return croppedGeoExtents();
  }


  inline bool needReproject(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->needReproject;
  }
  inline bool needSnapUp(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->needSnapUp;
  }
  inline bool normIsCropped(void) const {
    impl->EnsureKeyholeNormalizedInfo();
    return impl->alignedPixelExtents != impl->croppedPixelExtents;
  }

  inline GDALDataset* getMaskDS() const { return impl->getMaskDS(); }

  // you don't normally need to call this directly. The routines above
  // will call it as needed
  inline void EnsureKeyholeNormalizedInfo(void) {
    impl->EnsureKeyholeNormalizedInfo();
  }

  khTilespace::ProjectionType GetProjectionType() const {
    return impl->GetProjectionType();
  }

  inline const std::string& GetFilename() const {
    return impl->GetFilename();
  }

  inline khGDALDataset(void) : impl() { }

  // WILL THROW ON ERROR
  // empty override args mean don't override
  khGDALDataset(const std::string &filename,
                const std::string &overrideSRS = std::string(),
                const khExtents<double> &overrideExtents =
                khExtents<double>(),
                khTilespace::ProjectionType aProjectionType =
                  khTilespace::FLAT_PROJECTION);
};

#endif /* GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_KHGDALDATASET_H_ */
