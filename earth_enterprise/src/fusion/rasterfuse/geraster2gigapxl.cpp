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


#include <khMisc.h>
#include <khGetopt.h>
#include <khGuard.h>
#include <khFileUtils.h>
#include <khTileAddr.h>
#include <notify.h>
#include <khraster/ProductLevelReaderCache.h>
#include <khraster/khRasterProduct.h>
#include <khraster/Interleave.h>
#include <compressor.h>
#include <khProgressMeter.h>

void ExtractSubtiles(const ImageryProductTile& tile, int tile_size,
                     khLevelCoverage level_coverage,
                     const std::string& path);

void usage(const std::string& progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
      "\nusage: %s [options] --kip <raster.kip> --output <dirname>\n"
      "   Supported options are:\n"
      "      --tile_size <size>     Output tilesize (256, 512 or 1024)\n"
      "      --jpg_quality <qual>   Set quality for jpg compression\n"
      "      --force                Remove output directory first if it exists\n"
      "      --help | -?:           Display this usage message\n",
      progn.c_str());
  exit(1);
}

Compressor* jpg_compressor = NULL;
khDeleteGuard<khRasterProduct> rp_rgb;
unsigned char interleave_buffer[
    RasterProductTileResolution * RasterProductTileResolution * 4];

int main(int argc, char *argv[]) {
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  std::string imagery_product;
  int jpg_quality = 80;
  std::string out_dir;
  int tile_size = 256;
  std::uint32_t end_level = 0;
  bool force = false;

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("force", force);
  options.opt("kip", imagery_product);
  options.opt("output", out_dir);
  options.opt("jpg_quality", jpg_quality);
  options.opt("tile_size", tile_size);
  options.opt("end_level", end_level);

  if (!options.processAll(argc, argv, argn))
    usage(progname);

  if (help)
    usage(progname);

  if (RasterProductTilespaceBase.tileSize % tile_size != 0)
    notify(NFY_FATAL, "Tile size %d is invalid!", tile_size);

  if (out_dir.empty())
    usage(progname, "Output directory required!");

  if (force) {
    if (!khMakeCleanDir(out_dir)) {
      notify(NFY_FATAL, "Unable to create empty output directory \"%s\".",
             out_dir.c_str());
    }
  } else {
    // ensure output directory doesn't exist already
    // and that we can make it successfully
    if (khDirExists(out_dir))
      notify(NFY_FATAL, "Specified output directory \"%s\" already exists."
             "  Please remove first.", out_dir.c_str());
    if (!khMakeDir(out_dir))
      notify(NFY_FATAL, "Unable to create specified output directory \"%s\"",
             out_dir.c_str());
  }

  // open imagery product (rgb)
  if (imagery_product.empty() || !khHasExtension(imagery_product, ".kip"))
    notify(NFY_FATAL, "You must specify a valid imagery product (*.kip)");
  rp_rgb = khRasterProduct::Open(imagery_product);
  if (!rp_rgb || rp_rgb->type() != khRasterProduct::Imagery) {
    notify(NFY_FATAL, "Failed to open imagery product %s",
           imagery_product.c_str());
  }

  notify(NFY_DEBUG, "extents n=%lf s=%lf e=%lf w=%lf",
         rp_rgb->degOrMeterNorth(), rp_rgb->degOrMeterSouth(),
         rp_rgb->degOrMeterEast(), rp_rgb->degOrMeterWest());

  fprintf(stderr, "Min level=%d, Max level=%d\n",
         rp_rgb->minLevel(), rp_rgb->maxLevel());

  jpg_compressor = new JPEGCompressor(tile_size, tile_size, 3, jpg_quality);

  // export tiles, one level at a time
  ImageryProductTile rgb_tile;
  if (end_level == 0 || end_level > rp_rgb->maxLevel()) {
    end_level = rp_rgb->maxLevel();
  }

  int tile_size_log2 = log2(tile_size);
  khTilespace target_tile_space(tile_size_log2, tile_size_log2, StartUpperLeft,
                                false, khTilespace::FLAT_PROJECTION, false);

  std::uint32_t begin_level = tile_size_log2;
  if (begin_level < rp_rgb->minLevel()) {
    notify(NFY_WARN, "Unable to build missing levels for tile size %d",
           tile_size);
    begin_level = rp_rgb->minLevel();
  }

  for (std::uint32_t level = begin_level; level <= end_level; ++level) {
    std::uint32_t output_level = level - tile_size_log2;

    // special-case level 0 & 1 to shift the black imagery
    std::uint32_t blank_offset = 0;
    if (output_level == 0)
      blank_offset = tile_size >> 2;
    else if (output_level == 1)
      blank_offset = tile_size >> 1;

    // create level directory
    std::string level_path = out_dir + "/" + ToString(output_level);
    if (!khMakeDir(level_path))
      notify(NFY_FATAL, "Unable to create level directory \"%s\"",
             level_path.c_str());

    int row_begin = rp_rgb->level(level).tileExtents().beginRow();
    int row_end = rp_rgb->level(level).tileExtents().endRow();
    int col_begin = rp_rgb->level(level).tileExtents().beginCol();
    int col_end = rp_rgb->level(level).tileExtents().endCol();

    fprintf(stderr, "Level:%d row:%d->%d col:%d->%d\n",
           level, row_begin, row_end, col_begin, col_end);

    // determine first row in order to subtract
    khLevelCoverage begin_coverage(TranslateLevelCoverage(
        RasterProductTilespaceBase,
        khLevelCoverage(khTileAddr(level, row_begin, col_begin)),
        target_tile_space));
    begin_coverage.cropToWorld(target_tile_space);
    int first_row = begin_coverage.extents.beginRow();

    int total_tiles = (row_end - row_begin) * (col_end - col_begin);
    khProgressMeter progress(total_tiles, "Input image tiles");

    for (int row = row_begin; row < row_end; ++row) {
      for (int col = col_begin; col < col_end; ++col) {
        rp_rgb->level(level).ReadTile(row, col, rgb_tile);

        khLevelCoverage coverage(TranslateLevelCoverage(
            RasterProductTilespaceBase,
            khLevelCoverage(khTileAddr(level, row, col)),
            target_tile_space));

        // extract origin
        khOffset<std::uint32_t> tile_origin = coverage.extents.origin();
        coverage.cropToWorld(target_tile_space);

        // special-case level 1 to handle the shift since it drops row 1
        if (output_level == 1)
          coverage.extents = khExtents<std::uint32_t>(RowColOrder, 0, 1, 0, 2);

        for (std::uint32_t sub_row = coverage.extents.beginRow();
             sub_row < coverage.extents.endRow(); ++sub_row) {
          for (std::uint32_t sub_col = coverage.extents.beginCol();
               sub_col < coverage.extents.endCol(); ++sub_col) {

            std::string image_path = level_path +
                                     "/r" + ToString(sub_row - first_row) +
                                     "_c" + ToString(sub_col) + ".jpg";

            ExtractAndInterleave(
                rgb_tile,
                ((sub_row - tile_origin.row()) * tile_size) + blank_offset,
                (sub_col - tile_origin.col()) * tile_size,
                khSize< unsigned int> (tile_size, tile_size),
                StartUpperLeft, interleave_buffer);

            jpg_compressor->compress((char*)interleave_buffer);
            khWriteSimpleFile(image_path, jpg_compressor->data(),
                              jpg_compressor->dataLen());
          }
        }
        progress.increment();
      }
    }
  }

  delete jpg_compressor;
}
