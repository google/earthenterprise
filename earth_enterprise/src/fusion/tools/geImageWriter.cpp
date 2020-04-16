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


// Write images and terrains to disk.
// Ported from gemaskgen.cpp.

#include "fusion/tools/geImageWriter.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// This is a GDAL header file which in some ancient version (1.4.2) didn't get
// installed. With GDAL 1.10 it does get installed.
#include <vrtdataset.h>

#include <algorithm>
#include <ios>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "common/notify.h"
#include "common/khFileUtils.h"
#include "common/khGetopt.h"
#include "common/khProgressMeter.h"
#include "common/khStringUtils.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/khgdal/khgdal.h"
#include "fusion/khgdal/khGeoExtents.h"
#include "fusion/khgdal/khGDALDataset.h"
#include "khraster/khRasterProduct.h"
#include "khraster/LevelRasterBand.h"

BandInfo::BandInfo(GDALRasterBand *band,
                   const khExtents<std::uint32_t> &extents,
                   khTypes::StorageEnum type)
    : hostBand(band),
      extract_extents_(extents),
      datatype(type) {
}

void geImageWriter::CopyAndWriteImage(const khSize<std::uint32_t>& rasterSize,
                                      const std::vector<BandInfo> &bands,
                                      GDALDriver *driver,
                                      const std::string &output,
                                      const khGeoExtents &outputGeoExtents,
                                      const bool is_mercator,
                                      char **output_options) {
  const char* projection = is_mercator ? GetMercatorProjString().c_str()
                                       : GetWGS84ProjString().c_str();
  CopyAndWriteImage(rasterSize, bands, driver, output, outputGeoExtents,
                    projection, output_options);
}

void geImageWriter::CopyAndWriteImage(const khSize<std::uint32_t>& rasterSize,
                                      const std::vector<BandInfo> &bands,
                                      GDALDriver *driver,
                                      const std::string &output,
                                      const khGeoExtents &outputGeoExtents,
                                      const char* projection,
                                      char **output_options) {
  if (!khEnsureParentDir(output)) {
    notify(NFY_FATAL, "Unable to mkdir %s", khDirname(output).c_str());
  }

  // build a virtual dataset - will be copied to output
  khDeleteGuard<GDALDataset>
    virtDataset(TransferOwnership(
                    new VRTDataset(rasterSize.width, rasterSize.height)));
  if (projection) {
    virtDataset->SetProjection(projection);
  }
  virtDataset->SetGeoTransform(const_cast<double*>(
      outputGeoExtents.geoTransform()));

  for (unsigned int b = 0; b < bands.size(); ++b) {
    GDALDataType gdalDatatype = GDTFromStorageEnum(bands[b].datatype);
    virtDataset->AddBand(gdalDatatype, 0);
    VRTSourcedRasterBand* vrtBand =
      reinterpret_cast<VRTSourcedRasterBand*>(
          virtDataset->GetRasterBand(virtDataset->GetRasterCount()));
    vrtBand->AddSimpleSource(bands[b].hostBand,
                             bands[b].extract_extents_.beginX(),
                             bands[b].extract_extents_.beginY(),
                             bands[b].extract_extents_.width(),
                             bands[b].extract_extents_.height());
  }

  // write the output file
  khDeleteGuard<GDALDataset> outDS(
      TransferOwnership(
          reinterpret_cast<GDALDataset*>(
              GDALCreateCopy(driver, output.c_str(),
                             reinterpret_cast<GDALDataset*>(&*virtDataset),
                             0, output_options,
                             0, 0))));
  if (!outDS) {
    notify(NFY_FATAL, "Unable to write %s", output.c_str());
  }
}


void geImageWriter::ExtractLevelImage(const khRasterProductLevel &prodLevel,
                                      const khExtents<std::uint32_t> &extract_extents,
                                      GDALDriver *driver,
                                      const std::string &output,
                                      const khGeoExtents &outputGeoExtents,
                                      const bool is_mercator,
                                      char **output_options) {
  notify(NFY_NOTICE, "Writing extract file %s", output.c_str());

  // Build the BandInfo structures
  khDeletingVector<GDALRasterBand> rasterBands;
  std::vector<BandInfo> bands;
  for (unsigned int b = 0; b < prodLevel.numComponents(); ++b) {
    // Set GDAL band number to 0 since it is free-standing raster band
    // (there is no owner, referenced dataset is set to NULL).
    rasterBands.push_back(new LevelRasterBand(NULL, /* dataset */
                                              0, /* gdalBand */
                                              b, /* extract band */
                                              prodLevel,
                                              prodLevel.componentType()));
    bands.push_back
      (BandInfo(rasterBands[b],
                khExtents<std::uint32_t>
                (khOffset<std::uint32_t>(XYOrder,
                                  extract_extents.beginX(),
                                  (prodLevel.tileExtents().numRows() *
                                   RasterProductTileResolution) -
                                  extract_extents.endY()),
                 extract_extents.size()),
                prodLevel.componentType()));
  }

  geImageWriter::CopyAndWriteImage(
      extract_extents.size(), bands, driver, output, outputGeoExtents,
      is_mercator, output_options);
}


void geImageWriter::WriteImageryPreview(
    const khRasterProductLevel &prodLevel,
    const khExtents<std::uint32_t> &extract_extents,
    unsigned char *alphaBuf,
    GDALDriver *driver,
    const std::string &output,
    const khGeoExtents &outputGeoExtents,
    const bool is_mercator,
    char **output_options) {
  notify(NFY_NOTICE, "Writing imagery preview %s", output.c_str());

  // Build the BandInfo structures
  khDeletingVector<GDALRasterBand> rasterBands;
  std::vector<BandInfo> bands;
  for (unsigned int b = 0; b < prodLevel.numComponents(); ++b) {
    // Set GDAL band number to 0 since it is free-standing raster band
    // (there is no owner, referenced dataset is set to NULL).
    rasterBands.push_back(new LevelRasterBand(NULL, /* dataset */
                                              0, /* gdalBand */
                                              b, /* extract band */
                                              prodLevel,
                                              khTypes::UInt8));
    bands.push_back
      (BandInfo(rasterBands.back(),
                khExtents<std::uint32_t>
                (khOffset<std::uint32_t>(XYOrder,
                                  extract_extents.beginX(),
                                  (prodLevel.tileExtents().numRows() *
                                   RasterProductTileResolution) -
                                  extract_extents.endY()),
                 extract_extents.size()),
                khTypes::UInt8));
  }

  // build the BandInfo for the alpha band
  // Set GDAL band number to 0 since it is free-standing raster band
  // (there is no owner, referenced dataset is set to NULL).
  rasterBands.push_back(new BufferRasterBand(NULL, /* dataset */
                                             0, /* gdalBand */
                                             alphaBuf,
                                             extract_extents.size(),
                                             khTypes::UInt8));
  bands.push_back(BandInfo(rasterBands.back(),
                           khExtents<std::uint32_t>(khOffset<std::uint32_t>(XYOrder, 0, 0),
                                             extract_extents.size()),
                           khTypes::UInt8));

  geImageWriter::CopyAndWriteImage(
      extract_extents.size(), bands, driver, output, outputGeoExtents,
      is_mercator, output_options);
}


void geImageWriter::WriteHeightmapPreview(
    const khRasterProductLevel &prodLevel,
    const khExtents<std::uint32_t> &extract_extents,
    unsigned char *alphaBuf,
    GDALDriver *driver,
    const std::string &output,
    const khGeoExtents &outputGeoExtents,
    const bool is_mercator,
    char **output_options) {
  notify(NFY_NOTICE, "Writing heightmap preview %s", output.c_str());

  // Build the BandInfo structures
  khDeletingVector<GDALRasterBand> rasterBands;
  std::vector<BandInfo> bands;
  for (unsigned int b = 0; b < 3; ++b) {
    // Set GDAL band number to 0 since it is free-standing raster band
    // (there is no owner, referenced dataset is set to NULL).
    rasterBands.push_back(new LevelRasterBand(NULL, /* dataset */
                                              0, /* gdalBand */
                                              0, /* extract band */
                                              prodLevel,
                                              khTypes::UInt8));
    bands.push_back
      (BandInfo(rasterBands.back(),
                khExtents<std::uint32_t>
                (khOffset<std::uint32_t>(XYOrder,
                                  extract_extents.beginX(),
                                  (prodLevel.tileExtents().numRows() *
                                   RasterProductTileResolution) -
                                  extract_extents.endY()),
                 extract_extents.size()),
                khTypes::UInt8));
  }

  // build the BandInfo for the alpha band
  // Set GDAL band number to 0 since it is free-standing raster band
  // (there is no owner, referenced dataset is set to NULL).
  rasterBands.push_back(new BufferRasterBand(NULL, /* dataset */
                                             0, /* gdalBand */
                                             alphaBuf,
                                             extract_extents.size(),
                                             khTypes::UInt8));
  bands.push_back(BandInfo(rasterBands.back(),
                           khExtents<std::uint32_t>(khOffset<std::uint32_t>(XYOrder, 0, 0),
                                             extract_extents.size()),
                           khTypes::UInt8));

  geImageWriter::CopyAndWriteImage(
      extract_extents.size(), bands, driver, output, outputGeoExtents,
      is_mercator, output_options);
}

void geImageWriter::WriteAlphaImage(const khSize<std::uint32_t> &extract_size,
                                    unsigned char *alphaBuf,
                                    GDALDriver *driver,
                                    const std::string &output,
                                    const khGeoExtents &outputGeoExtents,
                                    const bool is_mercator,
                                    char **output_options) {
  const char* projection = is_mercator ? GetMercatorProjString().c_str()
                                       : GetWGS84ProjString().c_str();
  WriteAlphaImage(extract_size, alphaBuf, driver, output, outputGeoExtents,
                  projection, output_options);
}

void geImageWriter::WriteAlphaImage(const khSize<std::uint32_t> &extract_size,
                                    unsigned char *alphaBuf,
                                    GDALDriver *driver,
                                    const std::string &output,
                                    const khGeoExtents &outputGeoExtents,
                                    const char* projection,
                                    char **output_options) {
  notify(NFY_NOTICE, "Writing alpha file %s", output.c_str());


  // build the BandInfo for the alpha band
  khDeletingVector<GDALRasterBand> rasterBands;
  std::vector<BandInfo> bands;
  // Set band number to 0 since it is free-standing raster band (there is no
  // owner, referenced dataset is set to NULL).
  rasterBands.push_back(new BufferRasterBand(NULL, /* dataset */
                                             0, /* gdalBand */
                                             alphaBuf,
                                             extract_size,
                                             khTypes::UInt8));

  bands.push_back(BandInfo(rasterBands.back(),
                           khExtents<std::uint32_t>(khOffset<std::uint32_t>(XYOrder, 0, 0),
                                             extract_size),
                           khTypes::UInt8));

  geImageWriter::CopyAndWriteImage(
      extract_size, bands, driver, output, outputGeoExtents, projection,
      output_options);
}
