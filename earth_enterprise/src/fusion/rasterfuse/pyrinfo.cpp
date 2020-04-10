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



#include <cstdint>
#include <khraster/pyrio.h>
#include <notify.h>
#include <khTileAddr.h>
#include <khGuard.h>
#include <khGeomUtils.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "fusion/khgdal/khgdal.h"
#include "fusion/tools/geImageWriter.h"
#include "fusion/khgdal/khGeoExtents.h"

void PrintToTiff(pyrio::Reader* reader,
                 const char * const output_tiff_name);

int main(int argc, char *argv[])
{
  const char *filename = NULL;

  // command line
  if (argc < 2) {
    notify(NFY_NOTICE, "Usage: %s <pyramid filename> [output_tiff_name]",
           argv[0]);
    return 1;
  } else {
    filename = argv[1];
    notify(NFY_NOTICE, "Checking file %s", filename);
  }
  const char * const output_tiff_name = (argc == 3) ? argv[2] : NULL;

  // open the file for reading
  khDeleteGuard<pyrio::Reader> reader;
  try {
    reader = TransferOwnership(new pyrio::Reader(filename));
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "Unable to open %s for reading: %s",
           filename, e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unable to open %s for reading: Unknown error",
           filename);
  }

  const pyrio::Header &hdr(reader->header());
  notify(NFY_NOTICE, "File format version: %u.%u",
         hdr.formatVersion / 10, hdr.formatVersion % 10);
  notify(NFY_NOTICE, "Raster type: %s",
         ToString(hdr.rasterType).c_str());
  notify(NFY_NOTICE, "Num components: %u", hdr.numComponents);
  notify(NFY_NOTICE, "Component type: %s",
         khTypes::StorageName(hdr.componentType));
  notify(NFY_NOTICE, "Compression: %s", CompressModeName(hdr.compressMode).c_str());
  notify(NFY_NOTICE, "Tile count (WH): %u, %u",
         hdr.levelSize.width, hdr.levelSize.height);
  const khExtents<double> &de(hdr.dataExtents);
  const khExtents<double> &te(hdr.tileExtents);
  const bool is_mercator = hdr.IsMercator();
  notify(NFY_NOTICE, "Projection: %s", is_mercator ? "MERCATOR" : "FLAT");
  notify(NFY_NOTICE, "Data coverage (NSEW): %.20f, %.20f, %.20f, %.20f",
         de.north(), de.south(), de.east(), de.west());
  notify(NFY_NOTICE, "Tile coverage (NSEW): %.20f, %.20f, %.20f, %.20f",
         te.north(), te.south(), te.east(), te.west());


  const std::uint32_t bufSize =
    khTypes::StorageSize(hdr.componentType) *
    RasterProductTileResolution *
    RasterProductTileResolution;

  khFreeGuard red(malloc(bufSize));
  khFreeGuard green(malloc(bufSize));
  khFreeGuard blue(malloc(bufSize));
  unsigned char* bufs[3] = {
    (unsigned char*)red.ptr(),
    (unsigned char*)green.ptr(),
    (unsigned char*)blue.ptr()
  };
  if (!reader->ReadAllBandBufs(0, 0, bufs, hdr.componentType,
                               hdr.numComponents)) {
    notify(NFY_FATAL, "Unable to read first tile");
  } else {
    notify(NFY_NOTICE, "Read first block");
  }
  if (output_tiff_name != NULL) {
    PrintToTiff(&(*reader), output_tiff_name);
  }
}


void PrintToTiff(pyrio::Reader* reader,
                 const char * const output_tiff_name) {
  const pyrio::Header &hdr(reader->header());
  const khExtents<double> &te(hdr.tileExtents);
  const bool is_mercator = hdr.IsMercator();

  const std::uint32_t tile_size =
    khTypes::StorageSize(hdr.componentType) *
    RasterProductTileResolution *
    RasterProductTileResolution;

  khFreeGuard red(malloc(tile_size));
  khFreeGuard green(malloc(tile_size));
  khFreeGuard blue(malloc(tile_size));
  unsigned char* bufs[3] = {
    reinterpret_cast<unsigned char*>(red.ptr()),
    reinterpret_cast<unsigned char*>(green.ptr()),
    reinterpret_cast<unsigned char*>(blue.ptr())
  };

  // Read in the pixels, tile number is counted from bottom to top, so also the
  // pixels in each tile.
  const size_t buf_size = tile_size * hdr.levelSize.width *
                          hdr.levelSize.height;
  khFreeGuard big_buff(malloc(buf_size * hdr.numComponents));

  unsigned char* big_buff_ptrs[3] = {
    reinterpret_cast<unsigned char*>(big_buff.ptr()),
    reinterpret_cast<unsigned char*>(big_buff.ptr()) + buf_size,
    reinterpret_cast<unsigned char*>(big_buff.ptr()) + 2 * buf_size };
  const size_t tile_row_size = khTypes::StorageSize(hdr.componentType) *
                        RasterProductTileResolution;
  const size_t big_row_size = tile_row_size *  hdr.levelSize.width;
  for (size_t i = 0; i < hdr.levelSize.width; ++i) {
    for (size_t k = 0; k < hdr.levelSize.height; ++k) {
      const size_t offset = tile_size * hdr.levelSize.width * k +
                            tile_row_size * i;
      const size_t j = (hdr.levelSize.height - k - 1);
      for (size_t n = 0; n < hdr.numComponents; ++n) {
        if (!reader->ReadBandBuf(j, i, n, bufs[n],
                                 hdr.componentType, true)) {
          notify(NFY_FATAL, "Unable to read a tile");
        }
        for (size_t row = 0; row < RasterProductTileResolution; ++row) {
          memcpy(big_buff_ptrs[n]+ offset + big_row_size * row,
                 bufs[n] + tile_row_size * row, tile_row_size);
        }
      }
    }
  }

  // Now cut the buffer (from whole multiple of tile boundary) to image boundary
  const int orig_width = RasterProductTileResolution * hdr.levelSize.width;
  const int orig_height = RasterProductTileResolution * hdr.levelSize.height;
  const khExtents<double> &de(hdr.dataExtents);
  int cut_width;
  int cut_height;
  for (size_t i = 0; i <  hdr.numComponents; ++i) {
    cut_width = orig_width;
    cut_height = orig_height;
    khGeomUtils::CutTileOptimized(
        big_buff_ptrs[i], khTypes::StorageSize(hdr.componentType),
        &cut_width, &cut_height,
        te.north(), te.south(), te.east(), te.west(),
        de.north(), de.south(), de.east(), de.west());
  }
  khSize<std::uint32_t> const img_size(cut_width, cut_height);

  khGDALInit();
  GDALDriver* driver_ =
      reinterpret_cast<GDALDriver*>(GDALGetDriverByName("GTiff"));
  khDeletingVector<GDALRasterBand> rasterBands;
  std::vector<BandInfo> bands;
  for (size_t n = 0; n < hdr.numComponents; ++n) {
    // Set band number to 0 since it is free-standing raster band (there is no
    // owner, referenced dataset is set to NULL).
    rasterBands.push_back(new BufferRasterBand(NULL,  /* dataset */
                                               0,  /*  band number */
                                               big_buff_ptrs[n],
                                               img_size,
                                               hdr.componentType));
    bands.push_back(BandInfo(rasterBands.back(),
                             khExtents<std::uint32_t>(khOffset<std::uint32_t>(XYOrder, 0, 0),
                                               img_size),
                             hdr.componentType));
  }
  const std::string output(output_tiff_name);
  geImageWriter::CopyAndWriteImage(
      img_size, bands, driver_, output, khGeoExtents(de, img_size), is_mercator,
      NULL);
}
