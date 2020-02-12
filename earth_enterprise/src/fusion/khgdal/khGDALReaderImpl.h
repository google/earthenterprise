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

#ifndef __khGDALReaderImpl_h
#define __khGDALReaderImpl_h

#include "khGDALReader.h"
#include "ReadHelpers.h"
#include <limits>


// ****************************************************************************
// ***  khGDALReader
// ****************************************************************************

template <class SrcPixelType, class TileType>
void
khGDALReader::TypedRead(const khExtents<uint32> &srcExtents, bool topToBottom,
                        TileType &tile, const khOffset<uint32> &tileOffset)
{
  assert(storage == khTypes::Helper<SrcPixelType>::Storage);
  // Make sure SrcPixelType has a min and max
  assert(std::numeric_limits<SrcPixelType>::is_specialized);

  if (topToBottom != this->topToBottom) {
    throw khException(kh::tr("topToBottom doesn't match dataset"));
  }

  // prep buffer
  const uint pixelsPerBand = srcExtents.width() * srcExtents.height();
  const uint bandSize = (pixelsPerBand * sizeof(SrcPixelType));
  rawReadBuf.reserve(numBands * bandSize);

  // Fill buffer with NoData or zero
  SrcPixelType no_data = GetNoDataOrZero<SrcPixelType>();
  for (uint i = 0; i < pixelsPerBand * numBands; ++i) {
    ((SrcPixelType *) (&rawReadBuf[0]))[i] = no_data;
  }

  // read pixels from all bands into rawReadBuf
  FetchPixels(srcExtents);

  const uint32 width  = srcExtents.width();
  const uint32 height = srcExtents.height();
  const uint32 rowOff = tileOffset.row();
  const uint32 colOff = tileOffset.col();
  if (palette) {
    SrcPixelType *bandBuf = (SrcPixelType*)&rawReadBuf[0];
    for (uint row = 0; row < height; ++row) {
      const uint32 rbase = (topToBottom ? (height - row - 1) : row)
                           * width;
      const uint32 tbase = ((row + rowOff) *
                            TileType::TileWidth) + colOff;
      for (uint col = 0; col < width; ++col) {
        uint32 rpos = rbase + col;
        uint32 tpos = tbase + col;
        if ((uint)bandBuf[rpos] < paletteSize) {
          PaletteAssigner<typename TileType::PixelType,
            TileType::NumComp>::Assign
            (tile.bufs, tpos, palette[(uint)bandBuf[rpos]]);
        } else {
          ZeroAssigner<typename TileType::PixelType,
            TileType::NumComp>::Assign(tile.bufs, tpos);
        }
      }
    }
  } else {
    // non-palette image

    // copy the read bands into the tile, flipping and clamping as
    // necessary
    for (uint b = 0; b < numBands; ++b) {
      SrcPixelType *bandBuf =
        (SrcPixelType*)(&rawReadBuf[0] + b * bandSize);
      for (uint row = 0; row < height; ++row) {
        const uint32 rbase = (topToBottom ? (height - row - 1) : row)
                             * width;
        const uint32 tbase = ((row + rowOff) *
                              TileType::TileWidth) + colOff;
        for (uint col = 0; col < width; ++col) {
          uint32 rpos = rbase + col;
          uint32 tpos = tbase + col;
          tile.bufs[b][tpos] =
            ClampRange<typename TileType::PixelType>
            (bandBuf[rpos]);
        }
      }
    }

    // replicate bands if necessary
    for (uint b = numBands; b < TileType::NumComp; ++b) {
      notify(NFY_DEBUG, "     replicating band %d", b);
      memcpy(tile.bufs[b], tile.bufs[0], TileType::BandBufSize);
    }
  }
}

template <class SrcPixelType>
SrcPixelType khGDALReader::GetNoDataOrZero() {
  double no_data;
  int nodata_exists;
  GetNoDataFromSrc(no_data, nodata_exists);
  SrcPixelType sanitized_no_data = 0;
  if (nodata_exists) {
    // Check that nodata fits in the pixel type
    if (no_data >= std::numeric_limits<SrcPixelType>::min() &&
        no_data <= std::numeric_limits<SrcPixelType>::max()) {
      sanitized_no_data = static_cast<SrcPixelType>(no_data);
    }
    else if (printNotifications) {
      notify(NFY_WARN, "Ignoring NoData (%.2f) because it is too large for the pixel type.", no_data);
    }
  }

  if (printNotifications && sanitized_no_data != 0) {
    notify(NFY_NOTICE, "Using non-zero NoData (%.2f). If Fusion produces black regions "
                       "around this image, consider creating your own mask and disabling "
                       "Auto Masking for this resource.", no_data);
  }

  // Only print notifications the first time through
  printNotifications = false;

  return sanitized_no_data;
}


#endif /* __khGDALReaderImpl_h */
