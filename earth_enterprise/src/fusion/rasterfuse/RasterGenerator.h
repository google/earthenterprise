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

/******************************************************************************
File:        RasterGenerator.h
******************************************************************************/
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERGENERATOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERGENERATOR_H_

#include <string>
#include <khraster/khRasterProduct.h>
#include <khgdal/khGDALDataset.h>
#include <gdalwarper.h>

class khProgressMeter;

class ImageInfo
{
 public:
  khRasterProduct::RasterType rasterType;
  khGDALDataset               srcDS;
  khTypes::StorageEnum        componentType;
  khTypes::StorageEnum        srcComponentType;
  GDALResampleAlg             resampleAlg;
  unsigned int                        numbands;
  // NoData value is taken from srcDS for heightmaps if present, otherwise 0.
  // Final value is still 0 for imagery and masks; for heightmaps it is scaled
  // and clamped as the data will be.
  double                      noDataValue;
  double                      finalNoDataValue;
  // Following are only valid for heightmap source images.
  float                       scale;
  bool                        clampNonnegative;  // input values clamped >=0
  const khTilespace::ProjectionType projectionType;

  // my tile extents. row/col are level wide
  khExtents<std::uint32_t> tileExtents;
 private:
  // private and unimplemented, it's illegal to copy an ImageInfo
  ImageInfo(const ImageInfo&);
  ImageInfo& operator=(const ImageInfo&);

  void ComputeTileExtents(void);
 public:
  inline unsigned int toplevel(void) const { return srcDS.normalizedTopLevel(); }
  inline unsigned int numComponents(void) const { return numbands; }
  inline khExtents<double> degExtents(void) const {
    return srcDS.normalizedGeoExtents().extents();
  }
  inline const khExtents<std::int64_t> alignedPixelExtents(void) const {
    return srcDS.alignedPixelExtents();
  }
  inline const khExtents<std::int64_t> croppedPixelExtents(void) const {
    return srcDS.croppedPixelExtents();
  }

  // for imagery source images
  ImageInfo(const std::string &infile,
            const std::string &overridesrs,
            khTilespace::ProjectionType aProjectionType);

  // for heightmap source images
  ImageInfo(const std::string &infile,
            const std::string &overridesrs,
            float scale,
	    bool clampNonneg);

  // for alpha source images
  ImageInfo(const std::string &maskfile,
            const std::string &datafile,
            const std::string &overridesrs,
            khTilespace::ProjectionType aProjectionType);

};



/******************************************************************************
 ***  RasterGenerator
 ***
 ***  Someday this should be split in two (data & alpha)?
 ******************************************************************************/
class RasterGenerator {
  khRasterProduct  *outRP;

 public:
  static void MakeImageryProduct(
      const std::string &infile,
      const std::string &outfile,
      khTilespace::ProjectionType projectionType,
      const std::string& overridesrs = std::string());
  static void MakeHeightmapProduct(const std::string &infile,
                                   const std::string &outfile,
                                   const std::string &overridesrs,
                                   float scale,
                                   bool clamp_nonnegative);
  static void MakeAlphaProduct(const std::string &infile,
                               const std::string &outfile,
                               const std::string &datafile,
                               khRasterProduct::RasterType dataRasterType,
                               khTilespace::ProjectionType projectionType,
                               const std::string &overridesrs = std::string());
  static void ExtractDataProduct(const khRasterProduct *inRP,
                                 const khLevelCoverage &extractCov,
                                 const std::string &outfile);
 private:
  // used to generate a new Raster Product from source
  RasterGenerator(const ImageInfo &imageInfo,
                  const std::string &outfile);
  // used to generate an Alpha Raster Product from source
  RasterGenerator(const ImageInfo &imageInfo,
                  const std::string &outfile,
                  khRasterProduct::RasterType dataRasterType);
  // used for extracting from another Raster Product
  RasterGenerator(const khRasterProduct *inRP,
                  const khLevelCoverage &extractCov,
                  const std::string &outfile);
  ~RasterGenerator(void);

  template <class TileType>
  void MakeLevels(const ImageInfo &imageInfo);

  template <class TileType>
  void Extract(const khRasterProduct *inRP,
               const khLevelCoverage &extractCov);
};


#endif /* GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERGENERATOR_H_ */
