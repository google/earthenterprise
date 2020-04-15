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
khGDALReader::TypedRead(const khExtents<std::uint32_t> &srcExtents, bool topToBottom,
                        TileType &tile, const khOffset<std::uint32_t> &tileOffset)
{
  assert(storage == khTypes::Helper<SrcPixelType>::Storage);
  // Make sure SrcPixelType has a min and max
  assert(std::numeric_limits<SrcPixelType>::is_specialized);

  if (topToBottom != this->topToBottom) {
    throw khException(kh::tr("topToBottom doesn't match dataset"));
  }

  // prep buffer
  const unsigned int pixelsPerBand = srcExtents.width() * srcExtents.height();
  const unsigned int bandSize = (pixelsPerBand * sizeof(SrcPixelType));
  rawReadBuf.reserve(numBands * bandSize);

  // Fill buffer with NoData or zero
  SrcPixelType no_data = GetNoDataOrZero<SrcPixelType>();
  for (unsigned int i = 0; i < pixelsPerBand * numBands; ++i) {
    ((SrcPixelType *) (&rawReadBuf[0]))[i] = no_data;
  }

  // read pixels from all bands into rawReadBuf
  FetchPixels(srcExtents);

  const std::uint32_t width  = srcExtents.width();
  const std::uint32_t height = srcExtents.height();
  const std::uint32_t rowOff = tileOffset.row();
  const std::uint32_t colOff = tileOffset.col();
  if (palette) {
    SrcPixelType *bandBuf = (SrcPixelType*)&rawReadBuf[0];
    for (unsigned int row = 0; row < height; ++row) {
      const std::uint32_t rbase = (topToBottom ? (height - row - 1) : row)
                           * width;
      const std::uint32_t tbase = ((row + rowOff) *
                            TileType::TileWidth) + colOff;
      for (unsigned int col = 0; col < width; ++col) {
        std::uint32_t rpos = rbase + col;
        std::uint32_t tpos = tbase + col;
        if ((unsigned int)bandBuf[rpos] < paletteSize) {
          PaletteAssigner<typename TileType::PixelType,
            TileType::NumComp>::Assign
            (tile.bufs, tpos, palette[(unsigned int)bandBuf[rpos]]);
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
    for (unsigned int b = 0; b < numBands; ++b) {
      SrcPixelType *bandBuf =
        (SrcPixelType*)(&rawReadBuf[0] + b * bandSize);
      for (unsigned int row = 0; row < height; ++row) {
        const std::uint32_t rbase = (topToBottom ? (height - row - 1) : row)
                             * width;
        const std::uint32_t tbase = ((row + rowOff) *
                              TileType::TileWidth) + colOff;
        for (unsigned int col = 0; col < width; ++col) {
          std::uint32_t rpos = rbase + col;
          std::uint32_t tpos = tbase + col;
          tile.bufs[b][tpos] =
            ClampRange<typename TileType::PixelType>
            (bandBuf[rpos]);
        }
      }
    }

    // replicate bands if necessary
    for (unsigned int b = numBands; b < TileType::NumComp; ++b) {
      notify(NFY_DEBUG, "     replicating band %d", b);
      memcpy(tile.bufs[b], tile.bufs[0], TileType::BandBufSize);
    }
  }
}

template <class SrcPixelType>
SrcPixelType khGDALReader::GetNoDataOrZero() {
  if (!no_data_set) {
    double no_data;
    int nodata_exists;
    GetNoDataFromSrc(no_data, nodata_exists);
    if (nodata_exists) {
      // Check that nodata fits in the pixel type
      SrcPixelType pixelMin = std::numeric_limits<SrcPixelType>::lowest();
      SrcPixelType pixelMax = std::numeric_limits<SrcPixelType>::max();
      if (no_data >= pixelMin && no_data <= pixelMax) {
        sanitized_no_data = no_data;
      }
      else {
        notify(NFY_WARN, "Ignoring NoData (%.2f) because it is too large for the pixel type.", no_data);
      }
    }

    if (sanitized_no_data != 0) {
      notify(NFY_NOTICE, "Using non-zero NoData (%.2f). If Fusion produces black regions "
                         "around this image, consider creating your own mask and disabling "
                         "Auto Masking for this resource.", no_data);
    }

    no_data_set = true;
  }

  return static_cast<SrcPixelType>(sanitized_no_data);
}


#endif /* __khGDALReaderImpl_h */
