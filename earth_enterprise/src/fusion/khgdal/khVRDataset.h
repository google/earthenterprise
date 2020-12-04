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

#ifndef __khVRDataset_h
#define __khVRDataset_h

#include <memory>

#include "khLUT.h"
#include <khCalcHelper.h>
#include <khRefCounter.h>
#include <khExtents.h>
#include "khGDALDataset.h"

#include <gdal_priv.h>

class khVirtualRaster;
class khGDALBuffer;

template <class LUTType>
class khVRDataset : public GDALDataset
{
 public:
  typedef typename LUTType::InPixelType InPixelType;
  typedef typename LUTType::OutPixelType OutPixelType;
  typedef typename khCalcHelper<InPixelType>::DiffType InPixelDiffType;


  // stuff that makes me a GDALDataset
  double       geoTransform[6];
  std::string  srs;
  virtual CPLErr GetGeoTransform( double *transform ) {
    memcpy(transform, geoTransform, sizeof(geoTransform));
    return( CE_None );
  }
  virtual const char *GetProjectionRef(void) {
    return srs.c_str();
  }
  virtual CPLErr IRasterIO(GDALRWFlag eRWFlag, int nXOff, int nYOff, int nXSize,
                           int nYSize, void *pData, int nBufXSize,
                           int nBufYSize, GDALDataType eBufType, int nBandCount,
                           int *panBandMap, GSpacing nPixelSpace,
                           GSpacing nLineSpace, GSpacing nBandSpace,
                           GDALRasterIOExtraArg* psExtraArg);

 private:
  // stuff for managing my general band information
  std::vector<khSharedHandle<LUTType> > defaultLuts;
  std::vector<InPixelType>              inFillValues;
  std::vector<OutPixelType>             outFillValues;
  InPixelDiffType                       inFillTolerance;


  // stuff for managing my tiles
  class TileInfo {
   public:
    std::string filename;
    khExtents<std::uint32_t> extents;
    std::vector<khSharedHandle<LUTType> > bandLuts;

    TileInfo(const std::string &fname,
             const khExtents<std::uint32_t> &extents_,
             const std::vector<khSharedHandle<LUTType> > &luts) :
        filename(fname),
        extents(extents_),
        bandLuts(luts) { }
  };
  std::vector<TileInfo> tiles;
  std::vector<bool> intersectsOthers;
  khOffset<std::uint32_t> cropOrigin;


  // stuff for supporting IRasterIO
  std::vector<unsigned char> readBuf;
  bool               wantTransparentFill;
  class CacheNode {
   public:
    std::string filename;
    // auto_ptr will delete it's contents when destroyed
    // copies will steal ownership
    khGDALDataset dataset;

    CacheNode(const std::string &filename_)
        : filename(filename_),
          dataset(filename_) {
      //(GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly)) {
      if (!dataset.get()) {
        throw khException(kh::tr("Unable to load tile %1")
                          .arg(filename.c_str()));
      }
    }
    CacheNode(const CacheNode &o)
        : filename(o.filename),
          dataset(o.dataset) { }
    bool operator==(const std::string &fname) const { return filename == fname;}
  };
  static const unsigned int maxCacheSize = 20;
  std::list<CacheNode> datasets;
  // will throw on failure
  khGDALDataset FetchDataset(const std::string &filename);

  template <bool checkFill, class Dest>
  void TypedContribute(const khGDALBuffer   &destBuffer,
                       const khGDALBuffer   &srcBuffer,
                       const khSize<std::uint32_t> &rasterSize,
                       const std::vector<khSharedHandle<LUTType> >&bandLuts,
                       const khGDALBuffer *alphaBuffer);

  template <bool checkFill>
  void Contribute(const khGDALBuffer   &destBuffer,
                  const khGDALBuffer   &srcBuffer,
                  const khSize<std::uint32_t> &rasterSize,
                  const std::vector<khSharedHandle<LUTType> >&bandLuts,
                  const khGDALBuffer *alphaBuffer =0);

  // cannot create one directly. It must be created through the GDAL API
  khVRDataset( const khVirtualRaster & );
  friend GDALDataset* CreateKHVRDataset(const khVirtualRaster &);
  template <class In>
  friend GDALDataset*
  CreateLUTKHVRDataset(khTypes::StorageEnum, const khVirtualRaster &);
};


#endif /* __khVRDataset_h */
