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

#ifndef __khGDALBuffer_h
#define __khGDALBuffer_h

#include <vector>
#include <khTypes.h>
#include <khExtents.h>
#include <khException.h>
#include <khCalcHelper.h>
#include <cpl_error.h>

class GDALDataset;


// ****************************************************************************
// ***  Encapsulates all information about a GDAL Dataset RasterIO dest buffer
// ****************************************************************************
class khGDALBuffer {
 public:
  khTypes::StorageEnum pixelType;
  std::vector<uint>    bands;
  void*                buf;
  int64                pixelStep;  // in pixelSize steps
  int64                lineStep;   // in pixelSize steps
  int64                bandStep;   // in pixelSize steps

  khGDALBuffer(khTypes::StorageEnum pixType,
               const std::vector<uint> &bands_,
               void* dataBuf,
               int pixelStep_,
               int lineStep_,
               int bandStep_) :
      pixelType(pixType),
      bands(bands_),
      buf(dataBuf),
      pixelStep(pixelStep_),
      lineStep(lineStep_),
      bandStep(bandStep_)
  {
  }

  // construct from an offset inside another GDALBuffer
  khGDALBuffer(const khGDALBuffer &o, const khOffset<uint32> &offset) :
      pixelType(o.pixelType),
      bands(o.bands),
      buf(o.buf),
      pixelStep(o.pixelStep),
      lineStep(o.lineStep),
      bandStep(o.bandStep)
  {
    uint pixelSize = khTypes::StorageSize(pixelType);
    buf = ((uchar*)buf) +
          offset.y() * lineStep * pixelSize +
          offset.x() * pixelStep * pixelSize;
  }


  CPLErr Read(GDALDataset *srcDS, const khExtents<uint32> &srcExtents);

  template <class FillType>
  void Fill(const khSize<uint32> &rasterSize,
            const std::vector<FillType> &bandFillValues);

 private:
  template <class FillType, class DestType>
  void TypedFill(const khSize<uint32> &rasterSize,
                 const std::vector<FillType> &bandFillValues);
};


template <class FillType>
void
khGDALBuffer::Fill(const khSize<uint32> &rasterSize,
                   const std::vector<FillType> &bandFillValues)
{
  switch (pixelType) {
    case khTypes::UInt8:
      TypedFill<FillType, uint8>(rasterSize, bandFillValues);
      break;
    case khTypes::UInt16:
      TypedFill<FillType, uint16>(rasterSize, bandFillValues);
      break;
    case khTypes::Int16:
      TypedFill<FillType, int16>(rasterSize, bandFillValues);
      break;
    case khTypes::UInt32:
      TypedFill<FillType, uint32>(rasterSize, bandFillValues);
      break;
    case khTypes::Int32:
      TypedFill<FillType, int32>(rasterSize, bandFillValues);
      break;
    case khTypes::Float32:
      TypedFill<FillType, float32>(rasterSize, bandFillValues);
      break;
    case khTypes::Float64:
      TypedFill<FillType, float64>(rasterSize, bandFillValues);
      break;
    default:
      throw khException(kh::tr("Internal Error: Unsupported buffer type"));
  }
}


template <class FillType, class DestType>
void
khGDALBuffer::TypedFill(const khSize<uint32> &rasterSize,
                        const std::vector<FillType> &bandFillValues)
{
  for (uint band = 0; band < bands.size(); ++band) {
    DestType fillVal =
      ClampRange<DestType>(bandFillValues[bands[band]]);
    DestType *bandBuf = ((DestType*)buf) + band * bandStep;

    if ((sizeof(DestType) == 1) && (pixelStep == 1) &&
        (std::abs(lineStep) == static_cast<int64>(rasterSize.width))) {
      // special case, packed bytes
      if (lineStep > 0) {
        memset(bandBuf, (int)fillVal,
               rasterSize.width * rasterSize.height);
      } else {
        memset(bandBuf + (rasterSize.height - 1) * lineStep,
               (int)fillVal, rasterSize.width * rasterSize.height);
      }
    } else {
      for (uint line = 0; line < rasterSize.height; ++line) {
        DestType *lineBuf = bandBuf + line * lineStep;
        for (uint pixel = 0; pixel < rasterSize.width; ++pixel) {
          lineBuf[pixel * pixelStep] = fillVal;
        }
      }
    }
  }
}


#endif /* __khGDALBuffer_h */
