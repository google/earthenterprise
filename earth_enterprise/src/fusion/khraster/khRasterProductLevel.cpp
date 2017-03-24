// Copyright 2017 Google Inc.
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
khRasterProductLevel::ReadTileBandIntoBuf(uint32 row, uint32 col,
                                          uint band, uchar* destBuf,
                                          khTypes::StorageEnum destType,
                                          uint bufsize,
                                          bool flipTopToBottom) const
{
  const uint32 CalcBufSize = (RasterProductTileResolution *
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
khRasterProductLevel::ReadImageIntoBufs(const khExtents<uint32> &destExtents,
                                        uchar *destBufs[],
                                        uint   bands[],
                                        uint   numbands,
                                        khTypes::StorageEnum destType) const
{
  if (!reader && !OpenReader()) {
    throw khException(kh::tr("Unable to open reader"));
  }

  // allocate buffers for tile data
  uint32 BandSize = (RasterProductTileResolution *
                     RasterProductTileResolution *
                     khTypes::StorageSize(destType));
  khDeleteGuard<uchar, ArrayDeleter> bufguards[numbands];
  uchar* readBufs[numbands];
  for (uint b = 0; b < numbands; ++b) {
    bufguards[b] = TransferOwnership(readBufs[b] = new uchar[BandSize]);
  }

  // compute list of tiles to read
  khExtents<uint32> tileExtents
    (PixelExtentsToTileExtents
     (khExtents<int64>(XYOrder,
                       destExtents.beginX(),
                       destExtents.endX(),
                       destExtents.beginY(),
                       destExtents.endY()),
      RasterProductTileResolution));

  const uint PixelSize = khTypes::StorageSize(destType);

  for (uint row = tileExtents.beginRow();
       row < tileExtents.endRow(); ++row) {
    for (uint col = tileExtents.beginCol();
         col < tileExtents.endCol(); ++col) {

      if (!reader->ReadBandBufs(row, col, readBufs, bands, numbands,
                                destType)) {
        throw khException(kh::tr("Unable to read (rc) %1,%2")
                          .arg(row).arg(col));
      }

      // compute pixel intersection with this tile
      khOffset<uint32> thisOrigin(RowColOrder,
                                  row * RasterProductTileResolution,
                                  col * RasterProductTileResolution);
      khExtents<uint32>
        thisExtents(thisOrigin,
                    khSize<uint32>(RasterProductTileResolution,
                                   RasterProductTileResolution));
      khExtents<uint32>
        wantExtents(khExtents<uint32>::Intersection(destExtents,
                                                    thisExtents));

      // calculate offset into the dest buffer
      uint32 destOffsetX = wantExtents.beginX() - destExtents.beginX();
      uint32 destOffsetY = wantExtents.beginY() - destExtents.beginY();

      // Finally, let's actually move the data
      uint32 beginWantY = wantExtents.beginY() - thisOrigin.y();
      uint32 endWantY   = wantExtents.endY() - thisOrigin.y();
      uint32 srcx       = wantExtents.beginX() - thisOrigin.x();
      uint32 wantLineSize  = wantExtents.width() * PixelSize;
      uint32 numLines = 0;
      for (uint32 srcy = beginWantY; srcy < endWantY;
           ++srcy, ++numLines) {
        uint32 desty = destOffsetY + numLines;
        uint32 destx = destOffsetX;

        // we want the result image to be top -> bottom
        // so invert the desty
        desty = destExtents.height() - desty - 1;

        for (uint b = 0; b < numbands; ++b) {
          // Compute the buffer addresses
          uchar *srcPtr = readBufs[b] +
                          (((srcy * RasterProductTileResolution) + srcx) *
                           PixelSize);
          uchar *destPtr = destBufs[b] +
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
khRasterProductLevel::WriteTile(uint32 row, uint32 col, const SrcTile &src)
{
  COMPILE_TIME_ASSERT(SrcTile::TileWidth == RasterProductTileResolution,
                      IncompatibleTileWidth);
  COMPILE_TIME_ASSERT(SrcTile::TileHeight == RasterProductTileResolution,
                      IncompatibleTileHeight);
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
(uint32 row, uint32 col, const AlphaProductTile &src);
template bool
khRasterProductLevel::WriteTile<ImageryProductTile>
(uint32 row, uint32 col, const ImageryProductTile &src);
template bool
khRasterProductLevel::WriteTile<HeightmapInt16ProductTile>
(uint32 row, uint32 col, const HeightmapInt16ProductTile &src);
template bool
khRasterProductLevel::WriteTile<HeightmapFloat32ProductTile>
(uint32 row, uint32 col, const HeightmapFloat32ProductTile &src);
