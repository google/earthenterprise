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


#include <functional>
#include <list>

#include "khVRDataset.h"
#include "khgdal.h"
#include <khgdal/.idl/khVirtualRaster.h>
#include "khGDALBuffer.h"

#include <khFileUtils.h>
#include <khstrconv.h>


// ****************************************************************************
// ***  khVRRasterBand definition
// ***    - implementation at end of file
// ****************************************************************************
class khVRRasterBand : public GDALRasterBand
{
  GDALColorInterp                colorInterp;
  khDeleteGuard<GDALColorTable>  ctbl;
  bool                           haveNoData;
  double                         noDataValue;
 public:
  khVRRasterBand(GDALDataset          *dataset,
                 int                   gdalBandNum,
                 const khSize<std::uint32_t> &rastersize,
                 const khSize<std::uint32_t> &blocksize,
                 GDALDataType          gdalDatatype,
                 GDALColorInterp       colorInterp_,
                 const khTransferGuard<GDALColorTable> &ctbl_,
                 bool                  haveNoData_,
                 double                noDataValue_);
  virtual CPLErr IRasterIO(GDALRWFlag eRWFlag,
                           int nXOff, int nYOff, int nXSize, int nYSize,
                           void *pData, int nBufXSize, int nBufYSize,
                           GDALDataType eBufType,
                           GSpacing nPixelSpace, GSpacing nLineSpace,
                           GDALRasterIOExtraArg* psExtraArg);
  virtual CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void *pImage);
  virtual GDALColorInterp GetColorInterpretation(void) {
    return colorInterp;
  }
  virtual GDALColorTable *GetColorTable(void) {
    return ctbl;
  }
  virtual double GetNoDataValue(int *pbSuccess) {
    if (pbSuccess)
      *pbSuccess = (int)haveNoData;
    return noDataValue;
  }
};


// ****************************************************************************
// ***  khVRDatset
// ****************************************************************************
template <class T, class U>
struct my_equal_to : public std::binary_function <T, U, bool>
{
  bool operator()(const T& a, const U& b) const { return a == b; }
};


template <class LUTType>
khGDALDataset
khVRDataset<LUTType>::FetchDataset(const std::string &filename)
{
  typename std::list<CacheNode>::iterator found =
    find_if(datasets.begin(), datasets.end(),
            bind2nd(my_equal_to<CacheNode, std::string>(), filename));
  if (found != datasets.end()) {
    // move the found node to the front
    CacheNode tmp(*found);
    datasets.erase(found);
    datasets.push_front(tmp);
  } else {
    if (datasets.size() == maxCacheSize) {
      datasets.pop_back();
    }
    // will throw on failure
    datasets.push_front(CacheNode(filename));
  }
  return datasets.front().dataset;
}

template <class LUTType>
khVRDataset<LUTType>::khVRDataset(const khVirtualRaster &virtraster) :
    srs(virtraster.srs),
    defaultLuts(virtraster.outputBands.size()),
    inFillValues(virtraster.outputBands.size()),  // <- fill with 0's
    outFillValues(virtraster.outputBands.size()), // <- fill with 0's
    inFillTolerance(),   // <- 0 value for type
    cropOrigin(virtraster.cropExtents.origin()),
    wantTransparentFill(false)
{
  // since we can have lots of child blocks open, we need to make sure
  // that we have a big cache, else we'll thrash
  // If the user has specifically chosen a cache size, leave it alone.
  if (getenv("GDAL_CACHEMAX") == NULL) {
    int minRasterBlockCache = 200 * 1024 * 1024;
    if (GDALGetCacheMax64() < minRasterBlockCache) {
      GDALSetCacheMax64(minRasterBlockCache);
    }
  }

  // Setup some stuff in our base class
  poDriver     = 0;
  eAccess      = GA_ReadOnly;
  nRasterXSize = virtraster.cropExtents.width();
  nRasterYSize = virtraster.cropExtents.height();
  nBands       = virtraster.outputBands.size();
  assert(nBands);

  // prefill our geoTransform
  geoTransform[0] = virtraster.cropOriginX();
  geoTransform[1] = virtraster.pixelsize.width;
  geoTransform[2] = 0.0;
  geoTransform[3] = virtraster.cropOriginY();
  geoTransform[4] = 0.0;
  geoTransform[5] = virtraster.pixelsize.height;

  // prefetch the first dataset. This allows code to assume that we always
  // have at least one in the set.
  FetchDataset(virtraster.inputTiles[0].file);

  // process my band information
  for (unsigned int b = 0; b < (unsigned int)nBands; ++b) {
    const khVirtualRaster::OutputBand &bandDef(virtraster.outputBands[b]);
    defaultLuts[b] = TransferOwnership(new LUTType(bandDef.defaultLut));

    bool   haveNoData = false;
    if (!bandDef.noDataValue.empty()) {
      haveNoData = true;

      // the KHVR def has a noDataValue, let's use it for our fill
      FromString(bandDef.noDataValue, inFillValues[b]);
      outFillValues[b] = (*defaultLuts[b])[inFillValues[b]];

      // the presence of a noDataValue is our key to fill transparent
      wantTransparentFill = true;
    }

    // create my GDAL band
    GDALDataType gdalBandType = GDTFromName(bandDef.outDatatype);
    GDALColorInterp colorInterp = GCIFromName(bandDef.colorinterp);
    khDeleteGuard<GDALColorTable> ctbl;
    if (colorInterp == GCI_PaletteIndex) {
      GDALRasterBand *gdalTileBand =
        datasets.front().dataset->GetRasterBand(b+1);
      if (gdalTileBand) {
        GDALColorTable *tileCtbl = gdalTileBand->GetColorTable();
        if (tileCtbl) {
          ctbl = TransferOwnership(tileCtbl->Clone());
        }
      }
      if (!ctbl) {
        throw khException
          (kh::tr("Unable to load color table for band %1").arg(b));
      }
    }
    khVRRasterBand *rasterBand =
      new khVRRasterBand(this, b+1,
                         virtraster.cropExtents.size(),
                         bandDef.outBlocksize,
                         gdalBandType,
                         colorInterp,
                         TransferOwnership(ctbl.take()),
                         haveNoData, (double)outFillValues[b]);
    SetBand(b+1, rasterBand); // will assume ownership
  }
  if (!virtraster.fillTolerance.empty()) {
    FromString(virtraster.fillTolerance, inFillTolerance);
  }


  // create our Tile objects
  tiles.reserve(virtraster.inputTiles.size());
  for (std::vector<khVirtualRaster::InputTile>::const_iterator tileDef =
         virtraster.inputTiles.begin();
       tileDef < virtraster.inputTiles.end(); ++tileDef) {

    khOffset<std::uint32_t>
      tileOrigin(XYOrder,
                 std::uint32_t(((tileDef->origin.x() - virtraster.origin.x()) /
                         virtraster.pixelsize.width) + 0.5),
                 std::uint32_t(((tileDef->origin.y() - virtraster.origin.y()) /
                         virtraster.pixelsize.height) + 0.5));
    khExtents<std::uint32_t> pixelExtents(tileOrigin,
                                   tileDef->rastersize);


    std::vector<khSharedHandle<LUTType > > tileLuts(defaultLuts);
    for (unsigned int b = 0;
         (b < tileDef->bandLUTs.size()) && (b < tileLuts.size());
         ++b) {
      if (!tileDef->bandLUTs[b].empty()) {
        tileLuts[b] =
          TransferOwnership(new LUTType(tileDef->bandLUTs[b]));
      }
    }
    tiles.push_back(TileInfo(tileDef->file, pixelExtents, tileLuts));
  }

  // check to see if tiles overlap
  unsigned int numTiles = tiles.size();
  intersectsOthers.resize(0);
  intersectsOthers.resize(numTiles, false);
  for (unsigned int i = 0; i < numTiles; ++i) {
    for (unsigned int j = i+1; j <numTiles; ++j) {
      if (tiles[i].extents.intersects(tiles[j].extents)) {
        intersectsOthers[i] = true;
        intersectsOthers[j] = true;
      }
    }
  }
}


template <class LUTType>
template <bool checkFill, class Dest>
void
khVRDataset<LUTType>::TypedContribute
(const khGDALBuffer   &destBuffer,
 const khGDALBuffer   &srcBuffer,
 const khSize<std::uint32_t> &rasterSize,
 const std::vector<khSharedHandle<LUTType> > &bandLuts,
 const khGDALBuffer *alphaBuffer)
{
  typedef InPixelType Src;
  typedef InPixelDiffType SrcDiff;

  const unsigned int numDestBands = destBuffer.bands.size();
  const unsigned int numSrcBands  = srcBuffer.bands.size();

#ifndef NDEBUG
  assert(khTypes::Helper<Src>::Storage == srcBuffer.pixelType);
  if (checkFill) {
    // in the checkFill case, the srcBuffer must contain all my bands
    // in order.
    assert(numSrcBands == (unsigned int)nBands);
    for (unsigned int b = 0; b < (unsigned int)nBands; ++b) {
      assert(srcBuffer.bands[b] == b);
    }
  } else {
    // in the non-checkFill case, the srcBuffer must contain all the
    // destBands, in order.
    assert(numDestBands == numSrcBands);
    for (unsigned int b = 0; b < numDestBands; ++b) {
      assert(srcBuffer.bands[b] == destBuffer.bands[b]);
    }
  }
#endif

  if (alphaBuffer) {
    for (unsigned int destBand = 0; destBand < numDestBands; ++destBand) {
      unsigned int srcBand = destBand;  // srcBand exactly match the destBand
      // the lutBand matches the orig band that the srcBand maps to.
      unsigned int lutBand = srcBuffer.bands[srcBand];
      Src  *srcBandBuf  = ((Src*)srcBuffer.buf) +
                          srcBand * srcBuffer.bandStep;
      Dest *destBandBuf =  ((Dest*)destBuffer.buf) +
                           destBand * destBuffer.bandStep;
      std::uint8_t *alphaBuf  = (std::uint8_t*)alphaBuffer->buf;

      for (std::uint32_t line = 0; line < rasterSize.height; ++line) {
        Src  *src = srcBandBuf;
        Dest *dest = destBandBuf;
        std::uint8_t *alpha = alphaBuf;
        for (std::uint32_t pixel = 0; pixel < rasterSize.width; ++pixel) {
          float a = (float)(*alpha) / 255.f;
          *dest = ClampRange<Dest>((*dest) * (1.f - a) +
                                   (*bandLuts[lutBand])[*src] * a);
          src += srcBuffer.pixelStep;
          dest += destBuffer.pixelStep;
          alpha += alphaBuffer->pixelStep;
        }
        srcBandBuf += srcBuffer.lineStep;
        destBandBuf += destBuffer.lineStep;
        alphaBuf += alphaBuffer->lineStep;
      }
    }
  } else {
    for (std::uint32_t line = 0; line < rasterSize.height; ++line) {
      Src  *sLineBuf  = ((Src*)srcBuffer.buf) +
                        line * srcBuffer.lineStep;
      Dest *dLineBuf  = ((Dest*)destBuffer.buf) +
                        line * destBuffer.lineStep;
      for (std::uint32_t pixel = 0; pixel < rasterSize.width; ++pixel) {
        Src  *sPixelBuf = sLineBuf + pixel * srcBuffer.pixelStep;
        Dest *dPixelBuf = dLineBuf + pixel * destBuffer.pixelStep;

        // Check for fill and only contribute if pixel isn't fill.  For
        // the (!checkFill) case, the compiler will remove the unused
        // variable "exceeds_tolerance"
        bool exceeds_tolerance = false;
        if (checkFill) { // removed at compile tile
          // we're guaranteed that the srcBands are ordered
          for (unsigned int srcBand = 0; srcBand < numSrcBands; ++srcBand) {
            Src *src  = sPixelBuf + srcBand * srcBuffer.bandStep;
            SrcDiff diff =
              SrcDiff(*src) - SrcDiff(inFillValues[srcBand]);
            if (diff < 0) diff = -diff;
            if (diff > inFillTolerance) {
              exceeds_tolerance = true;
              // If any one band exceeds tolerance no need to test the rest.
              break;
            }
          }
        }

        // checkFill check removed at compile time. Will reduce to
        // either "if (1)" or "exceeds_tolerance"
        if (!checkFill || exceeds_tolerance) {
          for (unsigned int destBand = 0; destBand < numDestBands; ++destBand) {
            Dest *dest = dPixelBuf + destBand * destBuffer.bandStep;

            unsigned int srcBand;
            unsigned int lutBand;
            if (checkFill) { // removed at compile time
              // srcBands exactly match real bands - figure out which
              // one the dest maps it to
              srcBand = destBuffer.bands[destBand];

              // the lutBand exactly matches the srcBand
              lutBand = srcBand;
            } else {
              // srcBand exactly match the destBand
              srcBand = destBand;

              // the lutBand matches the orig band that the srcBand
              // maps to.
              lutBand = srcBuffer.bands[srcBand];
            }
            Src  *src  = sPixelBuf + srcBand * srcBuffer.bandStep;

            // depending on Src, Dest, & LUTType, the compiler
            // may simplfy this to a simple assignment, or it could do
            // lookup and range clamping. The template implementations
            // will do the right thing to generate minimal code
            *dest = ClampRange<Dest>((*bandLuts[lutBand])[*src]);
          }
        }
      }
    }
  }
}

template <class LUTType>
template <bool checkFill>
void
khVRDataset<LUTType>::Contribute
(const khGDALBuffer   &destBuffer,
 const khGDALBuffer   &srcBuffer,
 const khSize<std::uint32_t> &rasterSize,
 const std::vector<khSharedHandle<LUTType> > &bandLuts,
 const khGDALBuffer *alphaBuffer)
{
  switch (destBuffer.pixelType) {
    case khTypes::UInt8:
      TypedContribute<checkFill, std::uint8_t>(destBuffer, srcBuffer,
                                        rasterSize, bandLuts, alphaBuffer);
      break;
    case khTypes::UInt16:
      TypedContribute<checkFill, std::uint16_t>(destBuffer, srcBuffer,
                                         rasterSize, bandLuts, alphaBuffer);
      break;
    case khTypes::Int16:
      TypedContribute<checkFill, std::int16_t>(destBuffer, srcBuffer,
                                        rasterSize, bandLuts, alphaBuffer);
      break;
    case khTypes::UInt32:
      TypedContribute<checkFill, std::uint32_t>(destBuffer, srcBuffer,
                                         rasterSize, bandLuts, alphaBuffer);
      break;
    case khTypes::Int32:
      TypedContribute<checkFill, std::int32_t>(destBuffer, srcBuffer,
                                        rasterSize, bandLuts, alphaBuffer);
      break;
    case khTypes::Float32:
      TypedContribute<checkFill, float32>(destBuffer, srcBuffer,
                                          rasterSize, bandLuts, alphaBuffer);
      break;
    case khTypes::Float64:
      TypedContribute<checkFill, float64>(destBuffer, srcBuffer,
                                          rasterSize, bandLuts, alphaBuffer);
      break;
    default:
      throw khException(kh::tr("Internal Error: Unsupported buffer type"));
  }
}


template <class LUTType>
CPLErr
khVRDataset<LUTType>::IRasterIO(GDALRWFlag eRWFlag,
                                int nXOff, int nYOff, int nXSize, int nYSize,
                                void * pData, int nBufXSize, int nBufYSize,
                                GDALDataType eBufType,
                                int nBandCount, int *panBandMap,
                                GSpacing nPixelSpace, GSpacing nLineSpace,
                                GSpacing nBandSpace,
                                GDALRasterIOExtraArg* psExtraArg) {
  try {
    // The public API, RasterIO, has already checked for empty and out of
    // bounds requests. It also checks for invalid bands numbers. We can
    // safely assume that we have all the pixels requested.

    // we only support reading
    if (eRWFlag != GF_Read) {
      CPLError(CE_Failure, CPLE_AppDefined,
               "KHVR only supports reading");
      return CE_Failure;
    }

    // we don't support scaling
    if ((nXSize != nBufXSize) || (nYSize != nBufYSize)) {
      CPLError(CE_Failure, CPLE_AppDefined,
               "KHVR doesn't support scaling");
      return CE_Failure;
    }

    // check the output type
    khTypes::StorageEnum destType;
    if (!StorageEnumFromGDT(eBufType, destType)) {
      CPLError(CE_Failure, CPLE_AppDefined,
               "KHVR doesn't support reading as %s",
               GDALGetDataTypeName(eBufType));
      return CE_Failure;
    }

    // build my list of bands - we know the bands passed to us are valid
    std::vector< unsigned int>  destBands(nBandCount);
    for (unsigned int b = 0; b < (unsigned int)nBandCount; ++b) {
      destBands[b] = panBandMap[b]-1;
    }

    // convert 'Space' parameters (char based) to Step (pixel based)
    unsigned int destPixelSize = khTypes::StorageSize(destType);
    std::int64_t destPixelStep = nPixelSpace / destPixelSize;
    std::int64_t destLineStep  = nLineSpace  / destPixelSize;
    std::int64_t destBandStep  = nBandSpace  / destPixelSize;


    // package up all the information about the dest buffer
    khGDALBuffer destBuffer(destType, destBands, pData,
                            destPixelStep, destLineStep, destBandStep);

    // fill the buffers with the fill values
    destBuffer.Fill(khSize<std::uint32_t>(nXSize, nYSize), outFillValues);


    nXOff += cropOrigin.x();
    nYOff += cropOrigin.y();
    const khExtents<std::uint32_t> readExtents(XYOrder,
                                        nXOff, nXOff + nXSize,
                                        nYOff, nYOff + nYSize);

    unsigned int numTiles = 0;
    std::vector<khExtents<std::uint32_t> > tileIExtents;
    for (unsigned int ti = 0; ti < tiles.size(); ++ti) {

      khExtents<std::uint32_t> intersection
        (khExtents<std::uint32_t>::Intersection(readExtents,
                                         tiles[ti].extents));
      if (intersection.empty()) {
        continue;
      }
      ++numTiles;

      bool needComposite = false;
      if (intersectsOthers[ti]) {
        for (unsigned int ii = 0; ii < tileIExtents.size(); ++ii)
          if (!khExtents<std::uint32_t>::Intersection(
                  intersection, tileIExtents[ii]).empty()) {
            needComposite = true;
            break;
          }
        tileIExtents.push_back(intersection);
      }

      khOffset<std::uint32_t> readOffset(XYOrder,
                                  intersection.beginX() -
                                  readExtents.beginX(),
                                  intersection.beginY() -
                                  readExtents.beginY());
      khOffset<std::uint32_t> tileOffset(XYOrder,
                                  intersection.beginX() -
                                  tiles[ti].extents.beginX(),
                                  intersection.beginY() -
                                  tiles[ti].extents.beginY());

      khGDALBuffer tileDestBuffer(destBuffer, readOffset);
      khGDALDataset tileDS = FetchDataset(tiles[ti].filename);

      needComposite = needComposite &&
                      (tileDS.getMaskDS() || wantTransparentFill);

      // If we're not doing compositing and we have no LUTs to
      // apply we can just have each tile contribute directly to the
      // request
      if (LUTType::IsIdentity && !needComposite) {
        CPLErr err = tileDestBuffer.Read
                     (tileDS,
                      khExtents<std::uint32_t>(tileOffset, intersection.size()));
        if (err != CE_None) {
          CPLError(err, CPLE_AppDefined,
                   "Error reading virtual raster tile %s",
                   tiles[ti].filename.c_str());
          return err;
        }
        continue; // done with this tile, proceed to next one
      }

      // We need to do some manipulation of the raw data

      // So read the data in its raw format
      khTypes::StorageEnum readType =
        khTypes::Helper<InPixelType>::Storage;

      // figure out which bands we need to read
      std::vector< unsigned int>  readBands;
      if (needComposite && !tileDS.getMaskDS()) {
        // must read all bands, not just those requested
        // since compositing will be based on 3-channel color matching
        readBands.resize(nBands);
        for (unsigned int i = 0; i < (unsigned int)nBands; ++i) {
          readBands[i] = i;
        }
      } else {
        // only need the bands requested
        readBands = destBands;
      }

      // reserve enough read buffer space for the intersection
      const unsigned int readPixelSize = khTypes::StorageSize(readType);
      readBuf.reserve(intersection.width() *
                      intersection.height() *
                      readPixelSize *
                      (readBands.size() + (tileDS.getMaskDS()==0?0:1)));

      // package up all the information about the dest buffer
      khGDALBuffer readBuffer(readType, readBands, &readBuf[0],
                              1,                      /* pixelStep */
                              intersection.width(),   /* lineStep */
                              intersection.width() *
                              intersection.height()); /* bandStep */

      // read into our readBuffer
      CPLErr err = readBuffer.Read
                   (tileDS.get(),
                    khExtents<std::uint32_t>(tileOffset, intersection.size()));
      if (err != CE_None) {
        CPLError(err, CPLE_AppDefined,
                 "Error reading virtual raster tile %s",
                 tiles[ti].filename.c_str());
        return err;
      }

      if (!needComposite) {
        Contribute<false>(tileDestBuffer, readBuffer,
                          intersection.size(),
                          tiles[ti].bandLuts);
      } else if (!tileDS.getMaskDS()) {
        Contribute<true>(tileDestBuffer, readBuffer,
                         intersection.size(),
                         tiles[ti].bandLuts);
      } else {
        int alphaOffs = intersection.width() * intersection.height() *
                        readPixelSize * readBands.size();
        std::vector< unsigned int>  alphaband(1);
        alphaband[0] = 0;
        // package up all the information about the alpha buffer
        khGDALBuffer alphaBuffer(khTypes::Helper<std::uint8_t>::Storage,
                                 alphaband,
                                 &readBuf[0] + alphaOffs,
                                 1,                    /* pixelStep */
                                 intersection.width(),   /* lineStep */
                                 intersection.width() *
                                 intersection.height()); /* bandStep */

        // read into our alphaBuffer
        CPLErr err = alphaBuffer.Read
                     (tileDS.getMaskDS(), khExtents<std::uint32_t>(
                         tileOffset, intersection.size()));
        if (err != CE_None) {
          CPLError(err, CPLE_AppDefined,
                   "Error reading virtual raster tile (alpha) %s",
                   tiles[ti].filename.c_str());
          return err;
        }
        Contribute<false>(tileDestBuffer, readBuffer,
                          intersection.size(),
                          tiles[ti].bandLuts, &alphaBuffer);
      }
    }
  } catch (const std::exception &e) {
    CPLError(CE_Failure, CPLE_AppDefined,
             "KHVR read error: %s", e.what());
    return CE_Failure;
  } catch (...) {
    CPLError(CE_Failure, CPLE_AppDefined,
             "Unknown KHVR read error");
    return CE_Failure;
  }
  return CE_None;
}


// ****************************************************************************
// ***  Factory function
// ***
// ***  This will instantiate all the variants of khVRDataset that we support
// ****************************************************************************
template <class In>
GDALDataset*
CreateLUTKHVRDataset(khTypes::StorageEnum outType,
                     const khVirtualRaster &virtraster)
{
  switch (outType) {
    case khTypes::UInt8:
      return new khVRDataset<khLUT<In, std::uint8_t> >(virtraster);
    case khTypes::UInt16:
      return new khVRDataset<khLUT<In, std::uint16_t> >(virtraster);
    case khTypes::UInt32:
      return new khVRDataset<khLUT<In, std::uint32_t> >(virtraster);
    default:
      throw khException(kh::tr("Unsupported LUT output band type %1")
                        .arg(khTypes::StorageName(outType)));
  }
  return 0; // silent warnings
}

GDALDataset*
CreateKHVRDataset(const khVirtualRaster &virtraster)
{
  try {
    if (virtraster.outputBands.empty()) {
      throw khException(kh::tr("No bands"));
    }
    if (virtraster.inputTiles.empty()) {
      throw khException(kh::tr("No tiles"));
    }

    // For the forseable future, all bands must have the same input types.
    // And they must have the same output types.
    std::string outtypestr = virtraster.outputBands[0].outDatatype;
    GDALDataType gdalOutType = GDTFromName(outtypestr);
    khTypes::StorageEnum outType;
    if (!StorageEnumFromGDT(gdalOutType, outType)) {
      throw khException(kh::tr("Unsupported input band type %1 %2")
                        .arg(outtypestr.c_str()));
    }
    std::string intypestr = virtraster.outputBands[0].inDatatype.empty()
                            ? outtypestr : virtraster.outputBands[0].inDatatype;
    GDALDataType gdalInType = GDTFromName(intypestr);
    khTypes::StorageEnum inType;
    if (!StorageEnumFromGDT(gdalInType, inType)) {
      throw khException(kh::tr("Unsupported output band type %1")
                        .arg(intypestr.c_str()));
    }

    // if any tiles have a lut there will be a default lut. The lut will
    // cover all bands. So it's safe to check the defaultLut on the first
    // band to know if we have luts.
    if (virtraster.outputBands[0].defaultLut.size()) {
      switch (inType) {
        case khTypes::UInt8:
          return CreateLUTKHVRDataset<std::uint8_t>(outType, virtraster);
        case khTypes::UInt16:
          return CreateLUTKHVRDataset<std::uint16_t>(outType, virtraster);
        case khTypes::Int16:
          return CreateLUTKHVRDataset<std::int16_t>(outType, virtraster);
        default:
          break;
      }
      throw khException(kh::tr("Unsupported LUT input band type %1")
                        .arg(intypestr.c_str()));
    } else {
      switch (outType) {
        case khTypes::UInt8:
          return new khVRDataset<khIdentityLUT<std::uint8_t> >(virtraster);
        case khTypes::UInt16:
          return new khVRDataset<khIdentityLUT<std::uint16_t> >(virtraster);
        case khTypes::Int16:
          return new khVRDataset<khIdentityLUT<std::int16_t> >(virtraster);
        case khTypes::UInt32:
          return new khVRDataset<khIdentityLUT<std::uint32_t> >(virtraster);
        case khTypes::Int32:
          return new khVRDataset<khIdentityLUT<std::int32_t> >(virtraster);
        case khTypes::Float32:
          return new khVRDataset<khIdentityLUT<float32> >(virtraster);
        case khTypes::Float64:
          return new khVRDataset<khIdentityLUT<float64> >(virtraster);
        default:
          throw khException
            (kh::tr("Internal Error: Unhandled output band type %1")
             .arg(outtypestr.c_str()));
      }
    }
  } catch (const std::exception &e) {
    CPLError( CE_Failure, CPLE_OpenFailed,
              "KHVR creation error: %s.\n",
              e.what());
  } catch (...) {
    CPLError( CE_Failure, CPLE_OpenFailed,
              "KHVR creation error: Unknown error.\n");
  }
  return 0;
}


// ****************************************************************************
// ***  Open function
// ****************************************************************************
GDALDataset *
OpenKHVRDataset( GDALOpenInfo * openInfo )
{
  // we can't just look at initial header bytes to see if this is our file
  // we need to actually try to load it
  std::string fname = openInfo->pszFilename;

  // if its a URI then it's not mine
  if (khIsURI(fname)) return 0;

  if (khHasExtension(fname, ".khm")) {
    // we only allow read-only access
    if( openInfo->eAccess != GA_ReadOnly ) {
      CPLError( CE_Failure, CPLE_NotSupported,
                "The KHM driver only supports read access.\n" );
    } else {
      khMosaic mosaic;
      if (!mosaic.Load(fname)) {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to load .khm file.\n" );
      } else {
        return CreateKHVRDataset(mosaic);
      }
    }
  } else if (khHasExtension(fname, ".khvr")) {
    // we only allow read-only access
    if( openInfo->eAccess != GA_ReadOnly ) {
      CPLError( CE_Failure, CPLE_NotSupported,
                "The KHVR driver only supports read access.\n" );
    } else {
      khVirtualRaster virtraster;
      if (!virtraster.Load(fname)) {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to load .khvr file.\n" );
      } else {
        return CreateKHVRDataset(virtraster);
      }
    }
  }

  return 0;
}


/******************************************************************************
 ***  Register Functions
 ******************************************************************************/
extern "C"
void
GDALRegister_KHM()
{
  GDALDriver  *driver;

  if( GDALGetDriverByName( "KHM" ) == 0 ) {
    driver = new GDALDriver();
    driver->SetDescription( "KHM" );
    driver->SetMetadataItem( GDAL_DMD_LONGNAME, "Keyhole Mosaic" );
    driver->SetMetadataItem( GDAL_DMD_HELPTOPIC, "frmt_various.html#" );
    driver->SetMetadataItem( GDAL_DMD_EXTENSION, "khm" );
    driver->pfnOpen = OpenKHVRDataset;
    GetGDALDriverManager()->RegisterDriver( driver );
  }
}

extern "C"
void
GDALRegister_KHVR()
{
  GDALDriver  *driver;

  if( GDALGetDriverByName( "KHVR" ) == 0 ) {
    driver = new GDALDriver();
    driver->SetDescription( "KHVR" );
    driver->SetMetadataItem( GDAL_DMD_LONGNAME, "Keyhole Virtual Raster" );
    driver->SetMetadataItem( GDAL_DMD_HELPTOPIC, "frmt_various.html#" );
    driver->SetMetadataItem( GDAL_DMD_EXTENSION, "khvr" );
    driver->pfnOpen = OpenKHVRDataset;
    GetGDALDriverManager()->RegisterDriver( driver );
  }
}


// ****************************************************************************
// ***  khVRRasterBand
// ****************************************************************************
khVRRasterBand::khVRRasterBand(GDALDataset          *dataset,
                               int                   gdalBandNum,
                               const khSize<std::uint32_t> &rastersize,
                               const khSize<std::uint32_t> &blocksize,
                               GDALDataType          gdalDatatype,
                               GDALColorInterp       colorInterp_,
                               const khTransferGuard<GDALColorTable> &ctbl_,
                               bool                  haveNoData_,
                               double                noDataValue_) :
    colorInterp(colorInterp_),
    ctbl(ctbl_),
    haveNoData(haveNoData_),
    noDataValue(noDataValue_)
{
  // initialize parts of my base class
  // these really should be args to the base class constructor
  poDS         = dataset;
  nBand        = gdalBandNum;
  nRasterXSize = rastersize.width;
  nRasterYSize = rastersize.height;
  eDataType    = gdalDatatype;
  eAccess      = GA_ReadOnly;
  nBlockXSize  = blocksize.width;
  nBlockYSize  = blocksize.height;
}

CPLErr
khVRRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                           int nXOff, int nYOff, int nXSize, int nYSize,
                           void *pData, int nBufXSize, int nBufYSize,
                           GDALDataType eBufType,
                           GSpacing nPixelSpace, GSpacing nLineSpace,
                           GDALRasterIOExtraArg* psExtraArg) {
  return poDS->RasterIO(eRWFlag,
                        nXOff, nYOff, nXSize, nYSize,
                        pData, nBufXSize, nBufYSize,
                        eBufType,
                        1, &nBand,
                        nPixelSpace, nLineSpace,
                        nLineSpace * nBufYSize,
                        psExtraArg);
}

CPLErr
khVRRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void *pImage) {
  int nXOff = nBlockXOff * nBlockXSize;
  int nYOff = nBlockYOff * nBlockYSize;
  int nXSize = nBlockXSize;
  int nYSize = nBlockYSize;

  if (nXOff + nXSize > nRasterXSize)
    nXSize = nRasterXSize - nXOff;
  if (nYOff + nYSize > nRasterYSize)
    nYSize = nRasterYSize - nYOff;

  GSpacing nPixelSpace = GDALGetDataTypeSize(eDataType)/8;
  GSpacing nLineSpace = nPixelSpace * nBlockXSize;

  GDALRasterIOExtraArg sExtraArg;
  INIT_RASTERIO_EXTRA_ARG(sExtraArg);

  return IRasterIO(GF_Read,
                   nXOff, nYOff, nXSize, nYSize,
                   pImage, nXSize, nYSize,
                   eDataType, nPixelSpace, nLineSpace, &sExtraArg);
}
