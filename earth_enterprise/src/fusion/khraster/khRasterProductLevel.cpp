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

#include "khRasterProductLevel.h"
#include "khRasterProduct.h"
#include <notify.h>
#include <khException.h>

std::string
khRasterProductLevel::filename(void) const
{
  return product()->LevelName(levelnum());
}

uint
khRasterProductLevel::numComponents(void) const {
  return product()->numComponents();
}

khTypes::StorageEnum
khRasterProductLevel::componentType(void) const {
  return product()->componentType();
}


bool
khRasterProductLevel::OpenReader(void) const
{
  if (!reader) {
    try {
      reader = TransferOwnership(new pyrio::Reader(filename()));
      return true;
    } catch (const std::exception &e) {
      notify(NFY_WARN, "%s", e.what());
    } catch (...) {
      notify(NFY_WARN, "Unable to open %s for reading: Unknown error",
             filename().c_str());
    }
    return false;
  } else {
    return true;
  }
}

bool
khRasterProductLevel::ReadTileBandIntoBuf(std::uint32_t row, std::uint32_t col,
                                          unsigned int band, unsigned char* destBuf,
                                          khTypes::StorageEnum destType,
                                          unsigned int bufsize,
                                          bool flipTopToBottom) const
{
  const std::uint32_t CalcBufSize = (RasterProductTileResolution *
                              RasterProductTileResolution *
                              khTypes::StorageSize(destType));
  if (bufsize < CalcBufSize) {
    notify(NFY_WARN, "Internal Error: bufsize too small %u < %u",
           bufsize, CalcBufSize);
    return false;
  }
  if (!tileExtents().ContainsRowCol(row, col)) {
    notify(NFY_WARN, "Internal Error: Invalid row/col: %u,%u", row, col);
    return false;
  }

  if (!reader && !OpenReader()) return false;
  return reader->ReadBandBuf(row - tileExtents().beginRow(),
                             col - tileExtents().beginCol(),
                             band, destBuf,
                             destType, flipTopToBottom);
}


void
khRasterProductLevel::ReadImageIntoBufs(const khExtents<std::uint32_t> &destExtents,
                                        unsigned char *destBufs[],
                                        unsigned int   bands[],
                                        unsigned int   numbands,
                                        khTypes::StorageEnum destType) const
{
  if (!reader && !OpenReader()) {
    throw khException(kh::tr("Unable to open reader"));
  }

  // allocate buffers for tile data
  std::uint32_t BandSize = (RasterProductTileResolution *
                     RasterProductTileResolution *
                     khTypes::StorageSize(destType));
  khDeleteGuard<unsigned char, ArrayDeleter> bufguards[numbands];
  unsigned char* readBufs[numbands];
  for (unsigned int b = 0; b < numbands; ++b) {
    bufguards[b] = TransferOwnership(readBufs[b] = new unsigned char[BandSize]);
  }

  // compute list of tiles to read
  khExtents<std::uint32_t> tileExtents
    (PixelExtentsToTileExtents
     (khExtents<std::int64_t>(XYOrder,
                       destExtents.beginX(),
                       destExtents.endX(),
                       destExtents.beginY(),
                       destExtents.endY()),
      RasterProductTileResolution));

  const unsigned int PixelSize = khTypes::StorageSize(destType);

  for (unsigned int row = tileExtents.beginRow();
       row < tileExtents.endRow(); ++row) {
    for (unsigned int col = tileExtents.beginCol();
         col < tileExtents.endCol(); ++col) {

      if (!reader->ReadBandBufs(row, col, readBufs, bands, numbands,
                                destType)) {
        throw khException(kh::tr("Unable to read (rc) %1,%2")
                          .arg(row).arg(col));
      }

      // compute pixel intersection with this tile
      khOffset<std::uint32_t> thisOrigin(RowColOrder,
                                  row * RasterProductTileResolution,
                                  col * RasterProductTileResolution);
      khExtents<std::uint32_t>
        thisExtents(thisOrigin,
                    khSize<std::uint32_t>(RasterProductTileResolution,
                                   RasterProductTileResolution));
      khExtents<std::uint32_t>
        wantExtents(khExtents<std::uint32_t>::Intersection(destExtents,
                                                    thisExtents));

      // calculate offset into the dest buffer
      std::uint32_t destOffsetX = wantExtents.beginX() - destExtents.beginX();
      std::uint32_t destOffsetY = wantExtents.beginY() - destExtents.beginY();

      // Finally, let's actually move the data
      std::uint32_t beginWantY = wantExtents.beginY() - thisOrigin.y();
      std::uint32_t endWantY   = wantExtents.endY() - thisOrigin.y();
      std::uint32_t srcx       = wantExtents.beginX() - thisOrigin.x();
      std::uint32_t wantLineSize  = wantExtents.width() * PixelSize;
      std::uint32_t numLines = 0;
      for (std::uint32_t srcy = beginWantY; srcy < endWantY;
           ++srcy, ++numLines) {
        std::uint32_t desty = destOffsetY + numLines;
        std::uint32_t destx = destOffsetX;

        // we want the result image to be top -> bottom
        // so invert the desty
        desty = destExtents.height() - desty - 1;

        for (unsigned int b = 0; b < numbands; ++b) {
          // Compute the buffer addresses
          unsigned char *srcPtr = readBufs[b] +
                          (((srcy * RasterProductTileResolution) + srcx) *
                           PixelSize);
          unsigned char *destPtr = destBufs[b] +
                           (((desty*destExtents.width()) + destx) *
                            PixelSize);

          memcpy(destPtr, srcPtr, wantLineSize);
        }
      }
    }
  }
}


bool
khRasterProductLevel::OpenWriter(const bool is_monochromatic) const
{
  if (writer)
    return true;

  try {
    writer = TransferOwnership(
        new pyrio::Writer
        (filename(),
         is_monochromatic ? 1 : product()->numComponents(),
         coverage.level,
         coverage.extents.size(),                         /* level size */
         product()->degOrMeterExtents(),                  /* data extents*/
         (product()->IsMercator() ?
          coverage.meterExtents(RasterProductTilespaceBase) :
          coverage.degExtents(RasterProductTilespaceBase)),  /* tile extents*/
         compdef.mode,
         product()->type(),
         product()->componentType(),
         compdef.quality));
    return true;
  } catch (const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unable to open %s for writing: Unknown error",
           filename().c_str());
  }
  return false;
}




template <class SrcTile>
bool
khRasterProductLevel::WriteTile(std::uint32_t row, std::uint32_t col, const SrcTile &src)
{
  static_assert(SrcTile::TileWidth == RasterProductTileResolution,
                      "Incompatible Tile Width");
  static_assert(SrcTile::TileHeight == RasterProductTileResolution,
                      "Incompatible Tile Height");
  assert(SrcTile::NumComp == numComponents());
  assert(SrcTile::Storage == componentType());

  if (!writer && !OpenWriter(src.IsMonoChromatic())) return false;
  bool result = writer->WritePyrTile(row-tileExtents().beginRow(),
                                     col-tileExtents().beginCol(),
                                     src);
  if (result) {
    product_->wroteSomeTiles = true;
    if (product_->type() == khRasterProduct::AlphaMask) {
      khOpacityMask::OpacityType opacity = ComputeOpacity(src);
      product_->opacityMask_->SetOpacity(khTileAddr(levelnum(),row,col),
                                         opacity);
    }
  }
  return result;
}

// ****************************************************************************
// ***  expliti instantiation since it's defined here in the .cpp
// ****************************************************************************
template bool
khRasterProductLevel::WriteTile<AlphaProductTile>
(std::uint32_t row, std::uint32_t col, const AlphaProductTile &src);
template bool
khRasterProductLevel::WriteTile<ImageryProductTile>
(std::uint32_t row, std::uint32_t col, const ImageryProductTile &src);
template bool
khRasterProductLevel::WriteTile<HeightmapInt16ProductTile>
(std::uint32_t row, std::uint32_t col, const HeightmapInt16ProductTile &src);
template bool
khRasterProductLevel::WriteTile<HeightmapFloat32ProductTile>
(std::uint32_t row, std::uint32_t col, const HeightmapFloat32ProductTile &src);
