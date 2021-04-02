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


/******************************************************************************
File:        RasterGenerator.cpp
******************************************************************************/
#include "fusion/rasterfuse/RasterGenerator.h"

#include <assert.h>

#include "fusion/khgdal/khgdal.h"
#include "common/khTileAddr.h"
#include "common/khFileUtils.h"
#include "common/khGeomUtils.h"
#include "common/notify.h"
#include "common/khProgressMeter.h"
#include "common/khException.h"
#include "common/khGuard.h"
#include "common/khConstants.h"
#include "fusion/khgdal/RasterClusterAnalyzer.h"
#include "fusion/khgdal/khGDALReaderImpl.h"
#include "fusion/khraster/MinifyTile.h"


// ****************************************************************************
// ***  Helpers
// ****************************************************************************
const char *
GDALResampleAlgorithmName(GDALResampleAlg alg)
{
  switch (alg) {
    case GRA_NearestNeighbour:
      return "NearestNeighbor";
    case GRA_Bilinear:
      return "Bilinear";
    case GRA_Cubic:
      return "Cubic";
    case GRA_CubicSpline:
      return "CubicSpline";
    case GRA_Lanczos:
      return "LanczosInterpolation";
    case GRA_Average:
      return "Average";
    case GRA_Mode:
      return "Mode";
    case GRA_Max:
      return "Max";
    case GRA_Min:
      return "Min";
    case GRA_Med:
      return "Med";
    case GRA_Q1:
      return "Q1";
    case GRA_Q3:
      return "Q3";
  }
  return "Unknown"; // unreachable, but silences warnings
}


// ****************************************************************************
// ***  ZeroClamper - efficiently clamp pixel values at 0
// ***      Also avoids compiler warning
// ****************************************************************************
template <class T, bool isSigned>
class SmartZeroClamper { };

// specialized for unsigned types
template <class T>
class SmartZeroClamper<T, false> {
 public:
  inline static void Clamp(T *const, std::uint32_t) {
    // nothing to do - unsigned datatype can never be less than zero
  }
};

// specialized for signed types
template <class T>
class SmartZeroClamper<T, true>
{
 public:
  inline static void Clamp(T *const dest, std::uint32_t pos) {
    if (dest[pos] < 0) dest[pos] = 0;
  }
};

template <class T>
class ZeroClamper :
    public SmartZeroClamper<T, std::numeric_limits<T>::is_signed>
{ };


/******************************************************************************
 ***  ImageInfo - Holds khGDALDataset and some extra information
 ***  about the source image
 ******************************************************************************/

// constructor for Imagery sources
ImageInfo::ImageInfo(const std::string &infile,
                     const std::string &overridesrs,
                     khTilespace::ProjectionType aProjectionType)
    : rasterType(khRasterProduct::Imagery),
      srcDS(infile, overridesrs, khExtents<double>(), aProjectionType),
      componentType(khTypes::UInt64), // something illegal
      srcComponentType(khTypes::UInt64), // something illegal
      // In general, imagery is best resampled with the cubic kernel.
      resampleAlg(GRA_Cubic),
      numbands(srcDS->GetRasterCount()),
      noDataValue(0.0),                  // for Imagery, 0 is NoData
      finalNoDataValue(0.0),
      scale(0.0),                        // meaningless for Imagery
      clampNonnegative(false),           // meaningless for Imagery
      projectionType(aProjectionType)
{
  // We don't have to call this explicity. But we do so here because it
  // will throw exceptions that already include the filename. If it were
  // to throw from an implicit call below, our catch blocks at the end of
  // this function would add the filename again.
  srcDS.EnsureKeyholeNormalizedInfo();

  // catch any exceptions, prefix the string w/ the filename and rethrow
  try {
    // get the raster bands for use in validation
    if (numbands == 0) {
      throw khException(kh::tr("File has no bands"));
    }
    std::vector<GDALRasterBand*> rasterbands;
    for (unsigned int i = 0; i < numbands; i++) {
      rasterbands.push_back(srcDS->GetRasterBand(i+1));
    }

    if (!StorageEnumFromGDT(rasterbands[0]->GetRasterDataType(),
                            srcComponentType)) {
      throw khException
        (kh::tr("Unsupported source datatype: %1")
         .arg(GDALGetDataTypeName(rasterbands[0]->GetRasterDataType())));
    }

    // verify input and set values
    if (numbands == 1) {
      if (rasterbands[0]->GetColorInterpretation() ==
          GCI_PaletteIndex) {
        GDALColorTable *ctbl = rasterbands[0]->GetColorTable();
        if (!ctbl) {
          throw khException
            (kh::tr("Unable to get color table"));
        }
        if (ctbl->GetPaletteInterpretation() != GPI_RGB) {
          throw khException
            (kh::tr("Only RGB color tables supported for imagery"));
        }
        // palette images need NearestNeighbor sampling because
        // the pixel values cannot change
        resampleAlg = GRA_NearestNeighbour;
      }
    } else if (numbands == 3) {
      // nothing to do
    } else if (numbands == 4) {
      int num_alpha_bands = 0;
      for (unsigned int i = 0; i < numbands; ++i) {
        if (rasterbands[i]->GetColorInterpretation() == GCI_AlphaBand) {
          ++num_alpha_bands;
        }
      }
      switch (num_alpha_bands) {
        case 0: {
          std::string message = "Fusion has detected the imagery source has 4"
                                " bands.\n\tThe 4th band has type: ";
          message.append(GDALGetColorInterpretationName(
              rasterbands[3]->GetColorInterpretation()));
          message.append(".\n\tThis import will ignore the 4th band.");
          notify(NFY_WARN, "%s", message.c_str());
          break;
        }
        case 1: {
          notify(NFY_WARN,
                 "The image has an alpha band which will be ignored.");
          break;
        }
        default: {
          throw khException(kh::tr("The image has more than one alpha band."));
        }
      }
      numbands = 3;
    } else {  // Note: 2 band images, 5 or greater band images, are Unsupported.
      throw khException
        (kh::tr("Unsupported band count (%1)").arg(numbands));
    }
    if (rasterbands[0]->GetRasterDataType() != GDT_Byte) {
      notify(NFY_WARN,
             "Coercing imagery source to unsigned 8 bit.");
    }
    componentType = khTypes::UInt8;

    // fill in the rest of my members
    ComputeTileExtents();

  } catch (const std::exception &e) {
    throw khException(kh::tr("%1: %2").arg(infile.c_str()).arg(e.what()));
  } catch (...) {
    throw khException(kh::tr("%1: Unknown error processing file")
                      .arg(infile.c_str()));
  }
}


// constructor for Heightmap sources
ImageInfo::ImageInfo(const std::string &infile,
                     const std::string &overridesrs,
                     float scale_,
                     bool clampnonneg)
    : rasterType(khRasterProduct::Heightmap),
      srcDS(infile, overridesrs),
      componentType(khTypes::UInt64), // something illegal
      srcComponentType(khTypes::UInt64), // something illegal
      // We resample heightmaps with Bilinear. NearestNeighbor terraces steep
      // mountains, and BiCubic smooths too much.
      resampleAlg(GRA_Bilinear),
      numbands(srcDS->GetRasterCount()),
      noDataValue(0.0),  // Set below if it is specified in source.
      finalNoDataValue(0.0),
      scale(scale_),
      clampNonnegative(clampnonneg),
      projectionType(srcDS.GetProjectionType())
{
  // We don't have to call this explicity. But we do so here because it
  // will throw exceptions that already include the filename. If it were
  // to throw from an implicit call below, our catch blocks at the end of
  // this function would add the filename again.
  srcDS.EnsureKeyholeNormalizedInfo();

  bool mustResample = (srcDS.needReproject() || srcDS.needSnapUp());

  // catch any exceptions, prefix the string w/ the filename and rethrow
  try {
    // get the raster bands for use in validation
    if (numbands != 1) {
      throw khException
        (kh::tr("Heightmap sources must have exactly one band"));
    }
    GDALRasterBand* rasterband = srcDS->GetRasterBand(1);

    if (!StorageEnumFromGDT(rasterband->GetRasterDataType(),
                            srcComponentType)) {
      throw khException
        (kh::tr("Unsupported source datatype: %1")
         .arg(GDALGetDataTypeName(rasterband->GetRasterDataType())));
    }

    // verify input and set values
    if (rasterband->GetColorInterpretation() == GCI_PaletteIndex) {
      // Despite the GDAL API, Grey color tables really aren't
      // supported in GDAL right now.
      throw khException
        (kh::tr("Color tables not supported for heightmaps"));
    } else if (rasterband->GetRasterDataType() == GDT_Byte) {
      if ((scale == 1.0) && !mustResample) {
        notify(NFY_WARN,
               "Coercing heightmap source to integer 16 bit.");
        componentType = khTypes::Int16;
      } else {
        notify(NFY_WARN,
               "Coercing heightmap source to float 32 bit.");
        componentType = khTypes::Float32;
      }
    } else if (rasterband->GetRasterDataType() == GDT_Int16) {
      if ((scale == 1.0) && !mustResample) {
        componentType = khTypes::Int16;
      } else {
        notify(NFY_WARN,
               "Coercing heightmap source to float 32 bit.");
        componentType = khTypes::Float32;
      }
    } else if (rasterband->GetRasterDataType() == GDT_Float32) {
      componentType = khTypes::Float32;
    } else if (rasterband->GetRasterDataType() == GDT_Float64) {
      notify(NFY_WARN,
             "Coercing heightmap source to float 32 bit.");
      componentType = khTypes::Float32;
    } else {
      notify(NFY_WARN,
             "Coercing heightmap source to float 32 bit.");
      componentType = khTypes::Float32;
    }

    // Try to get NoData value from source raster, and if so use it to fill in
    // empty tiles/pixels in the pyramid.  Otherwise use 0.
    int nodata_exists = 0;
    noDataValue = rasterband->GetNoDataValue(&nodata_exists);
    if (!nodata_exists)
      noDataValue = 0.0;
    finalNoDataValue = scale * noDataValue;
    if (clampNonnegative && finalNoDataValue < 0.0)
      finalNoDataValue = 0.0;

    // fill in the rest of my members
    ComputeTileExtents();

  } catch (const std::exception &e) {
    throw khException(kh::tr("%1: %2").arg(infile.c_str()).arg(e.what()));
  } catch (...) {
    throw khException(kh::tr("%1: Unknown error processing file")
                      .arg(infile.c_str()));
  }
}


// constructor for AlphaMask
ImageInfo::ImageInfo(const std::string &maskfile,
                     const std::string &datafile,
                     const std::string &overridesrs,
                     khTilespace::ProjectionType aProjectionType)
    : rasterType(khRasterProduct::AlphaMask),
      srcDS(),
      componentType(khTypes::UInt8),
      srcComponentType(khTypes::UInt64), // something illegal
      resampleAlg(GRA_Bilinear), // good enough for alpha
      noDataValue(0.0),          // For alpha, NoData is 0
      finalNoDataValue(0.0),
      scale(0.0),                // meaningless for alpha
      clampNonnegative(false),   // meaningless for alpha
      projectionType(aProjectionType)
{
  // see if we can load the maskfile and get the
  // geoExtents from it. Ignore the exceptions, we'll check srcDS later
  try {
    srcDS = khGDALDataset(maskfile, overridesrs, khExtents<double>(),
                          projectionType);
  } catch (...) {
  }

  // extract the geo information from the datafile
  khExtents<double> dataExtents;
  std::string dataSRS;
  try {
    // check if the datafile is a raster product
    khDeleteGuard<khRasterProduct> dataRP(khRasterProduct::Open(datafile));
    if (dataRP) {
      // the datafile is a raster product
      dataExtents  = dataRP->degOrMeterExtents();
      dataSRS      = GetWGS84ProjString();
    } else {
      // Open the datafile via GDAL
      khGDALDataset dataDS(datafile, overridesrs);
      dataExtents  = dataDS.geoExtents().extents();
      dataSRS      = dataDS->GetProjectionRef();
    }
  } catch (...) {
    // Couldn't get geo info about the datafile
    if (!srcDS) {
      // I don't have any geo info myself, fatal combination
      // so rethrow
      throw;
    }
  }

  if (!srcDS) {
    // now make the srcDS, overriding the srs and extents
    srcDS = khGDALDataset(maskfile, dataSRS, dataExtents, projectionType);
  }

  // catch any exceptions, prefix the string w/ the filename and rethrow
  try {
    // get the raster bands for use in validation
    numbands = srcDS->GetRasterCount();
    if (numbands == 0) {
      throw khException(kh::tr("File has no bands"));
    } else if (numbands > 1) {
      notify(NFY_WARN,
             "Ignoring all but first band of alpha mask source.");
      numbands = 1;
    }
    GDALRasterBand* rasterband = srcDS->GetRasterBand(1);
    if (rasterband->GetRasterDataType() != GDT_Byte) {
      notify(NFY_WARN, "Coercing alpha mask source to unsigned 8 bit.");
    }

    if (!StorageEnumFromGDT(rasterband->GetRasterDataType(),
                            srcComponentType)) {
      throw khException
        (kh::tr("Unsupported source datatype: %1")
         .arg(GDALGetDataTypeName(rasterband->GetRasterDataType())));
    }


    // fill in the rest of my members
    ComputeTileExtents();

  } catch (const std::exception &e) {
    throw khException(kh::tr("%1: %2").arg(maskfile.c_str()).arg(e.what()));
  } catch (...) {
    throw khException(kh::tr("%1: Unknown error processing file")
                      .arg(maskfile.c_str()));
  }
}

void
ImageInfo::ComputeTileExtents(void)
{
  // level-wide tile extents
  tileExtents = PixelExtentsToTileExtents(srcDS.croppedPixelExtents(),
                                          RasterProductTilespaceBase.tileSize);
  notify(NFY_DEBUG, "tileExtents = (nsew) %u,%u,%u,%u (wh) %u,%u",
         tileExtents.north(),
         tileExtents.south(),
         tileExtents.east(),
         tileExtents.west(),
         tileExtents.width(),
         tileExtents.height());
}


// ****************************************************************************
// ***  Image Reader
// ****************************************************************************
class ImageReader
{
  const ImageInfo &imageInfo;

  khDeleteGuard<khGDALReader>  reader;
 public:
  explicit ImageReader(const ImageInfo &imageInfo_) : imageInfo(imageInfo_)
  {
    // Refer GDALCreateApproxTransformer documentation in gdal_warp.c
    // The maximum cartesian error in the "output" space that is to be accepted
    // in the linear approximation.
    // The value 0.125 is as default in khGDALWarpingReader.
    const double max_error = 0.125;

    if (imageInfo.srcDS.needReproject()) {
      notify(NFY_NOTICE, "Reprojecting source using %s",
             GDALResampleAlgorithmName(imageInfo.resampleAlg));
      const double* noData = NULL;
      if (imageInfo.rasterType == khRasterProduct::Heightmap)
        noData = &imageInfo.noDataValue;
      reader = TransferOwnership(
          new khGDALWarpingReader(
              imageInfo.srcDS, imageInfo.resampleAlg,
              imageInfo.numbands, max_error,
              imageInfo.projectionType, noData));
    } else if (imageInfo.srcDS.needSnapUp()) {
      // we snap with the warper, it knows to simplify the
      // reproject logic when it only needs a snapup
      notify(NFY_NOTICE, "Snapping source using %s",
             GDALResampleAlgorithmName(imageInfo.resampleAlg));
      reader = TransferOwnership(
          new khGDALWarpingReader(imageInfo.srcDS,
                                  imageInfo.resampleAlg,
                                  imageInfo.numbands,
                                  max_error,
                                  imageInfo.projectionType,
      imageInfo.rasterType == khRasterProduct::Heightmap ? &imageInfo.noDataValue
                                                         : NULL));
    } else {
      // image that exactly matches our projection and pixel size
      // requirements
      notify(NFY_NOTICE, "Simple read from source");
      reader = TransferOwnership(
          new khGDALSimpleReader(imageInfo.srcDS, imageInfo.numbands));
    }
  }

  // Read tile from source.
  // Do all coordinate calculations, filling, warping, palette lookup,
  // datatype conversion, band replication, etc.
  // Will throw exception if unable to complete read
  template <class TileType>
  void ReadTile(std::uint32_t row, std::uint32_t col, TileType &tile);
};


template <class T>
khExtents<T>
InvertRows(const khExtents<T> &sub, const khExtents<T> &all)
{
  return khExtents<std::uint32_t>(RowColOrder,
                           all.endRow() - sub.endRow(),
                           all.endRow() - sub.beginRow(),
                           sub.beginCol(),
                           sub.endCol());
}


// Read tile from source. Do all filling, warping, palette lookup, datatype
// conversion, band replication, etc.
// Will throw exception if unable to complete read
template <class TileType>
void
ImageReader::ReadTile(std::uint32_t row, std::uint32_t col, TileType &tile)
{
  if (!imageInfo.tileExtents.ContainsRowCol(row, col)) {
    throw khException
      (kh::tr("Internal error: Read requested for invalid tile: (rc) %1,%2")
       .arg(row).arg(col));
  }

  // pixel extents for this tile
  khExtents<std::int64_t> myTilePixelExtents
    (TileExtentsToPixelExtents
     (khExtents<std::uint32_t>(RowColOrder, row, row + 1, col, col + 1),
      RasterProductTilespaceBase.tileSize));
  notify(NFY_VERBOSE,
         "myTilePixelExtents = (nsew) %lld,%lld,%lld,%lld (wh) %lld,%lld",
         static_cast<long long>(myTilePixelExtents.north()),
         static_cast<long long>(myTilePixelExtents.south()),
         static_cast<long long>(myTilePixelExtents.east()),
         static_cast<long long>(myTilePixelExtents.west()),
         static_cast<long long>(myTilePixelExtents.width()),
         static_cast<long long>(myTilePixelExtents.height()));

  // pixels extents to be read
  khExtents<std::int64_t>
    myReadPixelExtents(khExtents<std::int64_t>::Intersection
                       (imageInfo.croppedPixelExtents(),
                        myTilePixelExtents));
  notify(NFY_VERBOSE,
         "myReadPixelExtents = (nsew) %lld,%lld,%lld,%lld (wh) %lld,%lld",
         static_cast<long long>(myReadPixelExtents.north()),
         static_cast<long long>(myReadPixelExtents.south()),
         static_cast<long long>(myReadPixelExtents.east()),
         static_cast<long long>(myReadPixelExtents.west()),
         static_cast<long long>(myReadPixelExtents.width()),
         static_cast<long long>(myReadPixelExtents.height()));

  // If there is no intersection with the source just fill the tile with NoData
  if ((myReadPixelExtents.width() == 0) ||
      (myReadPixelExtents.height() == 0)) {
    notify(NFY_DEBUG, "     no src intersection, fill with NoData %.3g",
           imageInfo.noDataValue);
    tile.Fill(static_cast<typename TileType::PixelType>(imageInfo.noDataValue));
    return;
  }

  khSize<std::uint32_t> readSize(myReadPixelExtents.width(),
                          myReadPixelExtents.height());

  // pixel offset where the pixels should go in this tile
  khOffset<std::uint32_t>
    tilePixelOffset(RowColOrder,
                    myReadPixelExtents.beginRow() %
                    RasterProductTilespaceBase.tileSize,
                    myReadPixelExtents.beginCol() %
                    RasterProductTilespaceBase.tileSize);

  // pixel offset where src pixels should be fetched
  khOffset<std::uint32_t>
    srcPixelOrigin(RowColOrder,
                   myReadPixelExtents.beginRow()  -
                   imageInfo.alignedPixelExtents().beginRow(),
                   myReadPixelExtents.beginCol()  -
                   imageInfo.alignedPixelExtents().beginCol());

  // pixel extents (src pixel space) to be read
  bool topToBottom = imageInfo.srcDS.alignedGeoExtents().topToBottom();
  khExtents<std::uint32_t> srcReadPixelExtents(srcPixelOrigin,
                                        readSize);
  if (topToBottom) {
    khExtents<std::uint32_t> srcExtents(RowColOrder,
                                 0,
                                 imageInfo.alignedPixelExtents().height(),
                                 0,
                                 imageInfo.alignedPixelExtents().width());
    // top -> bottom image
    // invert begin and end row
    srcReadPixelExtents = InvertRows(srcReadPixelExtents, srcExtents);
  }

  notify(NFY_VERBOSE, "srcReadPixelExtents = (nsew) %u,%u,%u,%u (wh) %u,%u",
         srcReadPixelExtents.north(),
         srcReadPixelExtents.south(),
         srcReadPixelExtents.east(),
         srcReadPixelExtents.west(),
         srcReadPixelExtents.width(),
         srcReadPixelExtents.height());

  // If we're not going to read the entire tile, fill it first
  if ((srcReadPixelExtents.width() < RasterProductTileResolution) ||
      (srcReadPixelExtents.height() < RasterProductTileResolution)) {
    notify(NFY_DEBUG, "     prefill with NoData %.3g", imageInfo.noDataValue);
    tile.Fill(static_cast<typename TileType::PixelType>(imageInfo.noDataValue));
  }


  // will warp/snap, do palette lookup, and do datatype conversion
  reader->Read(srcReadPixelExtents, topToBottom, tile, tilePixelOffset);
}


// ****************************************************************************
// ***  LevelGenerator
// ****************************************************************************
template <class TileType>
class LevelGeneratorBase
{
 protected:
  TileType tile;
  khProgressMeter &progress;
  const ImageInfo &imageInfo;
 public:
  khRasterProductLevel &prodLevel;

  virtual ~LevelGeneratorBase(void) { }
  LevelGeneratorBase(khRasterProductLevel &pl,
                     khProgressMeter &prog,
                     const ImageInfo &imageInfo_) :
      tile(imageInfo_.numbands == 1), progress(prog), imageInfo(imageInfo_),
      prodLevel(pl) {
  }

  virtual const TileType& MakeTile(std::uint32_t row, std::uint32_t col) = 0;
};

template <class TileType>
class SrcLevelGenerator : public LevelGeneratorBase<TileType>
{
  ImageReader reader;
 public:
  SrcLevelGenerator(khRasterProductLevel &pl, khProgressMeter &prog,
                    const ImageInfo &imageInfo_)
      : LevelGeneratorBase<TileType>(pl, prog, imageInfo_),
        reader(imageInfo_) { }

  virtual const TileType& MakeTile(std::uint32_t row, std::uint32_t col) {
    if (!this->prodLevel.tileExtents().ContainsRowCol(row, col)) {
      throw khException
        (kh::tr
         ("Internal Error: request for invalid src tile: (lrc) %1,%2,%3")
         .arg(this->prodLevel.levelnum()).arg(row).arg(col));
    }

    // Read tile from source. Do all filling, warping, palette
    // lookup, datatype conversion, band replication, etc.  Will
    // throw exception if unable to complete read
    reader.ReadTile(row, col, this->tile);

    // If it's a heightmap, scale and clamp up to 0 if needed.
    if (this->imageInfo.rasterType == khRasterProduct::Heightmap) {
      if (this->imageInfo.scale != 1.0) {
        assert(!std::numeric_limits<typename TileType::PixelType>
               ::is_integer);
        for (unsigned int i = 0; i < TileType::BandPixelCount; ++i) {
          // scale the pixel
          this->tile.bufs[0][i] = static_cast<typename TileType::PixelType>(
              this->tile.bufs[0][i] * this->imageInfo.scale);
        }
      }
      if (this->imageInfo.clampNonnegative &&
          std::numeric_limits<typename TileType::PixelType>::is_signed) {
        for (unsigned int i = 0; i < TileType::BandPixelCount; ++i) {
          ZeroClamper<typename TileType::PixelType>
            ::Clamp(this->tile.bufs[0], i);
        }
      }
    }

    if (!this->prodLevel.WriteTile(row, col, this->tile)) {
      throw
        khException(kh::tr
                    ("Unable to write tile: (lrc) %1,%2,%3")
                    .arg(this->prodLevel.levelnum()).arg(row).arg(col));
    }
    this->progress.increment();
    notify(NFY_DEBUG, "Source Tile: (lrc) %u,%u,%u",
           this->prodLevel.levelnum(), row, col);

    return this->tile;
  }
};


template <class TileType>
class MinifiedLevelGenerator : public LevelGeneratorBase<TileType>
{
  typedef typename TileType::PixelType CalcType;

  LevelGeneratorBase<TileType> *src;
  CalcType (*averager)(CalcType, CalcType, CalcType, CalcType);
 public:
  MinifiedLevelGenerator(khRasterProductLevel &pl,
                         khProgressMeter &prog,
                         const ImageInfo &imageInfo_,
                         CalcType (*averager_)(CalcType, CalcType,
                                               CalcType, CalcType),
                         LevelGeneratorBase<TileType> *s)
      : LevelGeneratorBase<TileType>(pl, prog, imageInfo_), src(s),
        averager(averager_) { }

  virtual const TileType& MakeTile(std::uint32_t row, std::uint32_t col) {
    if (!this->prodLevel.tileExtents().ContainsRowCol(row, col)) {
      throw khException
        (kh::tr
         ("Internal Error: request for invalid minified tile: (lrc) %1,%2,%3")
         .arg(this->prodLevel.levelnum()).arg(row).arg(col));
    }

    // for each of the four source tiles / quadrants
    for (unsigned int quad = 0; quad < 4; ++quad) {
      // magnify the dest row/col/quad to get a src row/col
      std::uint32_t srcRow = 0;
      std::uint32_t srcCol = 0;
      QuadtreePath::MagnifyQuadAddr(row, col, quad, srcRow, srcCol);

      // read high res tile to temporary buffers
      if (src->prodLevel.tileExtents().ContainsRowCol(srcRow, srcCol)) {
        const TileType &srcTile = src->MakeTile(srcRow, srcCol);

        MinifyTile(this->tile, quad, srcTile, averager);
      } else {
        // No source tile, so fill this quad with NoData in all bands. Source
        // data has been scaled/clamped already, so we use finalNoDataValue.
        this->tile.FillQuad(quad,
                            static_cast<typename TileType::PixelType>(
                                this->imageInfo.finalNoDataValue));
      }
    }

    if (!this->prodLevel.WriteTile(row, col, this->tile)) {
      throw
        khException(kh::tr
                    ("Unable to write tile: (lrc) %1,%2,%3")
                    .arg(this->prodLevel.levelnum()).arg(row).arg(col));
    }
    this->progress.increment();
    notify(NFY_DEBUG, "Minified Tile: (lrc) %u,%u,%u",
           this->prodLevel.levelnum(), row, col);

    return this->tile;
  }
};










/******************************************************************************
 ***  RasterGenerator
 ******************************************************************************/
// used to generate a new Raster Product from source
RasterGenerator::RasterGenerator(const ImageInfo &imageInfo,
                                 const std::string &outfile)
    : outRP(0)
{
  unsigned int defaultStartLevel =
    khRasterProduct::DefaultStartLevel(imageInfo.rasterType);
  if (imageInfo.toplevel() < defaultStartLevel) {
    throw
      khException(kh::tr("Source raster doesn't have enough precision"));
  }

  outRP = khRasterProduct::New(imageInfo.rasterType,
                               imageInfo.componentType,
                               outfile,
                               imageInfo.projectionType,
                               defaultStartLevel,
                               imageInfo.toplevel(),
                               imageInfo.degExtents());
  if (!outRP) {
    std::string msg { kh::tr("Unable to create output rasterproduct ").toUtf8().constData() };
    msg += outfile;
    throw khException(msg.c_str());
  }

  notify(NFY_DEBUG, "GLOBAL PIXEL LOD: Upsample = %u",
         imageInfo.toplevel());
  notify(NFY_DEBUG, "GLOBAL PIXEL SIZE: Upsample = %.8f\n",
         outRP->IsMercator() ?
         RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(
             imageInfo.toplevel()) :
         RasterProductTilespaceFlat.DegPixelSize(imageInfo.toplevel()));
}

// used to generate a new Mask Raster Product from source
RasterGenerator::RasterGenerator(const ImageInfo &imageInfo,
                                 const std::string &outfile,
                                 khRasterProduct::RasterType dataRasterType)
    : outRP(0)
{
  unsigned int defaultStartLevel =
    khRasterProduct::DefaultStartLevel(dataRasterType);
  unsigned int toplevel = imageInfo.toplevel();
  if (toplevel < defaultStartLevel) {
    // It's OK if alpha's have less resolution, we're expecting to
    // magnify them anyway
    defaultStartLevel = toplevel;
  }

  outRP = khRasterProduct::New(imageInfo.rasterType,
                               imageInfo.componentType,
                               outfile,
                               imageInfo.projectionType,
                               defaultStartLevel,
                               toplevel,
                               imageInfo.degExtents());
  if (!outRP) {
    std::string msg { kh::tr("Unable to create mask output rasterproduct ").toUtf8().constData() };
    msg += outfile;
    throw khException(msg.c_str());
  }

  notify(NFY_DEBUG, "GLOBAL PIXEL LOD: Upsample = %u", toplevel);
  const bool is_mercator =
      (imageInfo.projectionType == khTilespace::MERCATOR_PROJECTION);
  notify(NFY_DEBUG, "GLOBAL PIXEL SIZE: Upsample = %.8f %s\n", is_mercator ?
      RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(toplevel)
      : RasterProductTilespaceFlat.DegPixelSize(toplevel),
      is_mercator ? "Meter" : "degree");
}

// used for extracting from another Raster Product
RasterGenerator::RasterGenerator(const khRasterProduct *inRP,
                                 const khLevelCoverage &extractCov,
                                 const std::string &outfile)
    : outRP(0)
{
  // We will extract entire tiles from the inRP. Even if those tiles
  // are only partially covered with valid data, we will count the entire
  // tile extents as valid
  outRP = khRasterProduct::New(
      inRP->type(),
      inRP->componentType(),
      outfile,
      inRP->projectionType(),
      inRP->minLevel(),                                   /* min level */
      extractCov.level,                                   /* max level */
      (inRP->IsMercator() ?
       extractCov.meterExtents(RasterProductTilespaceBase) :
       extractCov.degExtents(RasterProductTilespaceBase)));  /* data extents */
  if (!outRP) {
    std::string msg { kh::tr("Unable to create output rasterproduct ").toUtf8().constData() };
    msg+= outfile;
    throw khException(msg.c_str());
  }
}


RasterGenerator::~RasterGenerator(void)
{
  delete outRP;
}


template <class TileType>
void
RasterGenerator::MakeLevels(const ImageInfo &imageInfo)
{
  typedef typename TileType::PixelType PixelType;
  static_assert(TileType::TileWidth == RasterProductTileResolution,
                      "Incompatible Tile Width");
  static_assert(TileType::TileHeight == RasterProductTileResolution,
                      "Incompatible Tile Height");
  assert(outRP->numComponents() == TileType::NumComp);
  assert(outRP->componentType() == TileType::Storage);

  khProgressMeter progress(outRP->CalcNumTiles());

  if (imageInfo.clampNonnegative) {
    if (std::numeric_limits<PixelType>::is_signed) {
      notify(NFY_NOTICE, "Clamping negative elevations to 0");
    } else {
      notify(NFY_NOTICE, "Unsigned source elevations cannot be negative");
    }
  }

  khDeletingVector<LevelGeneratorBase<TileType> > levels;
  levels.reserve(outRP->maxLevel() - outRP->minLevel() + 1);

  // make the source level
  khRasterProductLevel &maxLevel(outRP->level(outRP->maxLevel()));
  notify(NFY_DEBUG, "Making SrcLevel for %d", outRP->maxLevel());

  if (maxLevel.levelnum() != imageInfo.srcDS.normalizedTopLevel()) {
    throw khException
      (kh::tr("Internal Error: toplevel mismatch"));
  }

  if (maxLevel.tileExtents() != imageInfo.tileExtents) {
    throw khException
      (kh::tr("Internal Error: tileExtents mismatch"));
  }

  levels.push_back
    (new SrcLevelGenerator<TileType>(maxLevel, progress, imageInfo));



  typedef typename TileType::PixelType CalcType;
  CalcType (*averager)(CalcType, CalcType, CalcType, CalcType);
  if (outRP->type() == khRasterProduct::AlphaMask) {
    // we erode the alpha masks
    averager = &khCalcHelper<PixelType>::ZeroOrAverage;
  } else if (outRP->type() == khRasterProduct::Heightmap) {
    // heightmaps just take the lower left pixel
    averager = &khCalcHelper<PixelType>::TakeFirst;
  } else {
    // imagery averages the four pixels
    averager = &khCalcHelper<PixelType>::AverageOf4;
  }

  // make the minified levels
  // use int instead of unsigned int to avoid wrap at 0 in case of minLevel equals 0.
  for (int i = static_cast<int>(outRP->maxLevel()) - 1;
       i >= static_cast<int>(outRP->minLevel()); --i) {
    notify(NFY_DEBUG, "Making MinifiedLevel for %d", i);
    levels.push_back
      (new MinifiedLevelGenerator<TileType>
       (outRP->level(i), progress, imageInfo,
        averager, levels.back()));
  }

  // ask the lowest resolution level to make its tiles
  const khRasterProductLevel &minLevel(outRP->level(outRP->minLevel()));
  khExtents<std::uint32_t> tileExtents = minLevel.tileExtents();
  for (std::uint32_t row = tileExtents.beginRow();
       row < tileExtents.endRow(); row++) {
    for (std::uint32_t col = tileExtents.beginCol();
         col < tileExtents.endCol(); col++) {
      (void) levels.back()->MakeTile(row, col);
    }
  }
}


template <class TileType>
void
RasterGenerator::Extract(const khRasterProduct *inRP,
                         const khLevelCoverage &extractCov)
{
  static_assert(TileType::TileWidth == RasterProductTileResolution,
                      "Incompatible Tile Width");
  static_assert(TileType::TileHeight == RasterProductTileResolution,
                      "Incompatible Tile Height");
  assert(inRP->numComponents() == TileType::NumComp);
  assert(inRP->numComponents() == outRP->numComponents());
  assert(inRP->componentType() == TileType::Storage);
  assert(inRP->componentType() == outRP->componentType());

  khProgressMeter progress(outRP->CalcNumTiles());


  // preallocate the buffers
  TileType tile;

  // extract/write the highest resolution level
  const khRasterProduct::Level &inLevel(inRP->level(extractCov.level));
  khRasterProduct::Level &outLevel(outRP->level(extractCov.level));
  if (! outLevel.OpenWriter(inLevel.IsMonoChromatic()))
    throw khException(kh::tr("Unable to open level %1 for writing")
                      .arg(extractCov.level));
  for (unsigned int y = extractCov.extents.beginY();
       y < extractCov.extents.endY(); ++y) {
    for (unsigned int x = extractCov.extents.beginX();
         x < extractCov.extents.endX(); ++x) {
      if (!inLevel.ReadTile(y, x, tile)) {
        throw
          khException(kh::tr
                      ("Unable to read tile lev %1, row %2, col %3")
                      .arg(extractCov.level).arg(y).arg(x));
      }
      if (!outLevel.WriteTile(y, x, tile)) {
        throw
          khException(kh::tr
                      ("Unable to write tile lev %1, row %2, col %3")
                      .arg(extractCov.level).arg(y).arg(x));
      }

      progress.increment();
    }
  }
  outLevel.CloseWriter();

  // do 4:1 minification on remaining levels
  // ->template syntax is required to disambiguate the calling of
  // a template member function whose template type must be explicitly
  // specified
  outRP->template MinifyRemainingLevels<TileType>(outRP->maxLevel()-1,
                                                  &progress);
}


void
RasterGenerator::MakeImageryProduct(const std::string &infile,
                                    const std::string &outfile,
                                    khTilespace::ProjectionType projectionType,
                                    const std::string &overridesrs)
{
  // constructor for Imagery
  ImageInfo imageInfo(infile, overridesrs, projectionType);
  if (khExtension(imageInfo.srcDS.GetFilename()) == kVirtualRasterFileExtension) {
    notify(NFY_NOTICE, "Compare area of mosaic to its insets...");
    ClusterAnalyzer analyzer;
    analyzer.CalcAssetAreas(imageInfo.srcDS);
  }
  RasterGenerator(imageInfo, outfile).
    MakeLevels<ImageryProductTile>(imageInfo);
}

void
RasterGenerator::MakeHeightmapProduct(const std::string &infile,
                                      const std::string &outfile,
                                      const std::string &overridesrs,
                                      float scale,
                                      bool clampNonnegative)
{
  // constructor for Heightmap
  ImageInfo imageInfo(infile, overridesrs, scale, clampNonnegative);
  if (khExtension(imageInfo.srcDS.GetFilename()) == kVirtualRasterFileExtension) {
    notify(NFY_NOTICE, "Compare area of mosaic to its insets...");
    ClusterAnalyzer analyzer;
    analyzer.CalcAssetAreas(imageInfo.srcDS);
  }
  switch (imageInfo.componentType) {
    case khTypes::Int16:
      RasterGenerator(imageInfo, outfile).
        MakeLevels<HeightmapInt16ProductTile>(imageInfo);
      break;
    case khTypes::Float32:
      RasterGenerator(imageInfo, outfile).
        MakeLevels<HeightmapFloat32ProductTile>(imageInfo);
      break;
    default:
      // ImageInfo already took care of checking types, and converting or
      // throwing an exception. It should be impossible to reach this
      // case.
      assert(0);
  }
}

void
RasterGenerator::MakeAlphaProduct(const std::string &infile,
                                  const std::string &outfile,
                                  const std::string &datafile,
                                  khRasterProduct::RasterType dataRasterType,
                                  khTilespace::ProjectionType projectionType,
                                  const std::string &overridesrs
                                  ) {
  // constructor for Alpha
  ImageInfo imageInfo(infile, datafile, overridesrs, projectionType);

  RasterGenerator(imageInfo, outfile, dataRasterType).
    MakeLevels<AlphaProductTile>(imageInfo);
}

void
RasterGenerator::ExtractDataProduct(const khRasterProduct *inRP,
                                    const khLevelCoverage &extractCov,
                                    const std::string &outfile) {
  switch (inRP->type()) {
    case khRasterProduct::Imagery:
      RasterGenerator(inRP, extractCov, outfile)
        .Extract<ImageryProductTile>(inRP, extractCov);
      break;
    case khRasterProduct::Heightmap:
      switch (inRP->componentType()) {
        case khTypes::Int16:
          RasterGenerator(inRP, extractCov, outfile)
            .Extract<HeightmapInt16ProductTile>(inRP, extractCov);
          break;
        case khTypes::Float32:
          RasterGenerator(inRP, extractCov, outfile)
            .Extract<HeightmapFloat32ProductTile>(inRP, extractCov);
          break;
        default:
          throw khException
            (kh::tr("Unsupported heightmap product pixel type: %1")
             .arg(khTypes::StorageName(inRP->componentType())));
      }
      break;
    case khRasterProduct::AlphaMask:
      RasterGenerator(inRP, extractCov, outfile)
        .Extract<AlphaProductTile>(inRP, extractCov);
      break;
  }
}
