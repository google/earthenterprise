/*
 * Copyright 2017 Google Inc.
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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_KHGDALREADER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_KHGDALREADER_H_

#include <khException.h>
#include "khgdal.h"
#include "khGDALDataset.h"
#include <gdalwarper.h>
#include <khTileAddr.h>


// ****************************************************************************
// ***  khGDALReader
// ***
// ***  Wrap GDAL's read API
// ***    - expose only options Keyhole needs (default others as appropriate)
// ***    - can throw exceptions
// ***    - read directly into khRasterTile
// ****************************************************************************
class khGDALReader
{
 private:
  // read readExtents from src, place results in tile at the offset
  // specified. If src doesn't match TileType::PixelType then
  // convert/clamp as necessary
  // Will throw if can't read
  template <class SrcPixelType, class TileType>
  void TypedRead(const khExtents<uint32> &readExtents, bool topToBottom,
                 TileType &tile, const khOffset<uint32> &tileOffset);
 protected:
  khGDALDataset     srcDS;
  uint              numBands;
  std::vector<int>  gdalBandIndexes;
  GDALDataType      gdalDatatype;
  khTypes::StorageEnum storage;
  bool              topToBottom;

  uint paletteSize;
  const GDALColorEntry *palette;

  std::vector<uchar> rawReadBuf;

  // reads numBands worth of gdalDatatype pixels into rawReadBuf
  // assumes rawReadBuf has already been sized appropriately
  // will succeed or throw
  virtual void FetchPixels(const khExtents<uint32> &srcExtents) = 0;

 public:
  inline khTypes::StorageEnum componentType(void) const { return storage; }

  virtual ~khGDALReader(void) { }

  // empty bandIndexes means take numbands in order 0,1,...
  // Note: these band numbers are 0 based, not 1 based like that HORRID
  // gdal API.
  khGDALReader(const khGDALDataset &srcDS, uint numbands);

  // read readExtents from src, place results in tile at the offset
  // specified. If src doesn't match TileType::PixelType then
  // convert/clamp as necessary
  // Will throw if can't read
  template <class TileType>
  inline void Read(const khExtents<uint32> &readExtents, bool topToBottom,
                   TileType &tile, const khOffset<uint32> &tileOffset)
  {
    switch (storage) {
      case khTypes::UInt8:
        TypedRead<uint8, TileType>(readExtents, topToBottom,
                                   tile, tileOffset);
        break;
      case khTypes::Int8:
        TypedRead<int8, TileType>(readExtents, topToBottom,
                                  tile, tileOffset);
        break;
      case khTypes::UInt16:
        TypedRead<uint16, TileType>(readExtents, topToBottom,
                                    tile, tileOffset);
        break;
      case khTypes::Int16:
        TypedRead<int16, TileType>(readExtents, topToBottom,
                                   tile, tileOffset);
        break;
      case khTypes::UInt32:
        TypedRead<uint32, TileType>(readExtents, topToBottom,
                                    tile, tileOffset);
        break;
      case khTypes::Int32:
        TypedRead<int32, TileType>(readExtents, topToBottom,
                                   tile, tileOffset);
        break;
      case khTypes::Float32:
        TypedRead<float32, TileType>(readExtents, topToBottom,
                                     tile, tileOffset);
        break;
      case khTypes::Float64:
        TypedRead<float64, TileType>(readExtents, topToBottom,
                                     tile, tileOffset);
        break;
      default:
        throw khException
          (kh::tr("Internal Error: Bad storage type for khGDALReader"));
    }
  }
};


// ****************************************************************************
// ***  Simple read from GDAL source
// ****************************************************************************
class khGDALSimpleReader : public khGDALReader
{
 protected:
  // implement for my base class
  virtual void FetchPixels(const khExtents<uint32> &srcExtents);

 public:
  // empty bandIndexes means take numbands in order 0,1,...
  // Note: these band numbers are 0 based, not 1 based like that HORRID
  // gdal API.
  khGDALSimpleReader(const khGDALDataset &srcDS, uint numbands)
      : khGDALReader(srcDS, numbands)
  {
  }
};


// ****************************************************************************
// ***  Read from GDAL source and warp to Keyhole standard projection
// ****************************************************************************
class TransformArgGuard
{
  // private and unimplemented - you can't copy these
  TransformArgGuard(const TransformArgGuard &o);
  TransformArgGuard& operator=(const TransformArgGuard &o);

  typedef void (*DestroyFunc)(void*);

  void *arg;
  DestroyFunc destroyFunc;

 public:
  inline TransformArgGuard(DestroyFunc df) : arg(0), destroyFunc(df) { }
  inline explicit TransformArgGuard(void *a) : arg(a) { }
  TransformArgGuard& operator=(void *a);
  ~TransformArgGuard(void);
  inline operator void*(void) const { return arg; }
};

class SnapArg
{
 public:
  double xScale;
  double yScale;

  SnapArg(const double src[6], const double dest[6]) {
    xScale = dest[1] / src[1];
    yScale = dest[5] / src[5];
  }
};

class khGDALWarpingReader : public khGDALReader
{
  SnapArg            snapArg;
  TransformArgGuard  genImgProjArg;
  TransformArgGuard  approxArg;
  GDALWarpOperation  warpOperation;

 protected:
  // implement for my base class
  virtual void FetchPixels(const khExtents<uint32> &srcExtents);

 public:
  // empty bandIndexes means take numbands in order 0,1,...
  // Note: these band numbers are 0 based, not 1 based like that HORRID
  // gdal API.
  // leifhendrik: Added support for multiple internal projections
  // (projectionType).
  // The parameter *noData is the noData value to be used for both source and
  // destination images. If it is NULL that means abscence of noData parameter.
  khGDALWarpingReader(const khGDALDataset &srcDS,
                      GDALResampleAlg resampleAlg,
                      uint numbands,
                      double transformErrorThreshold =
                      0.125 /* from gdal_warp.c */,
                      khTilespace::ProjectionType projectionType =
                      khTilespace::FLAT_PROJECTION,
                      const double* noData = NULL);
};

#endif /* GEO_EARTH_ENTERPRISE_SRC_FUSION_KHGDAL_KHGDALREADER_H_ */
