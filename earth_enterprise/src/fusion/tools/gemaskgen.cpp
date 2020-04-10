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

// TODO: (speed) use opacity value in feathering
// - use bit to try to jump over empty tiles when feathering
//           (compute SAT in one go)
//     --> only works when floodfill & feathering block sizes are equal
//     --> only works if result of current tile is not affected (ALL neighboring
// pixels at most filtersize away were all the same type)
// - detect if an all-opaque tile became partially opaque after feathering

// TODO: (speed) add offset-inside-first-tile to flood fill
// algorithm so we can read the processing blocks aligned with the
// raster product tiles.

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "khraster/khRasterProduct.h"
#include "notify.h"
#include "khGetopt.h"
#include "khgdal/khgdal.h"
#include "khgdal/khGeoExtents.h"
#include "khgdal/khGDALDataset.h"
#include "vrtdataset.h"
#include "khProgressMeter.h"
#include "khraster/LevelRasterBand.h"
#include "khFileUtils.h"
#include "khStringUtils.h"

#include "tiledfloodfill.h"
#include "fusion/tools/geImageWriter.h"
#include "fusion/tools/Featherer.h"
#include "fusion/tools/in_memory_floodfill.h"
#include "boxfilter.h"

#define KHMASKGENVERSION "1.2"

namespace {
typedef std::map<std::string, std::string> OptionsMap;

// This is the default maximum height and width mask that will be
// generated.  The input raster product is reduced in resolution until
// it fits in this size, then the mask is created at that resolution.
// Overridden by command-line argument "maxsize".  Default value is
// the same as gemaskgen.
static const int kDefaultMaxMaskSize = 16000;

// This is the default maximum height and width mask that will be
// generated in memory (without tiling).  In-memory processing is much
// faster.  Overridden by command-line argument "maxsizeinmemory". The
// default value is set to provide backward-compatible operation with
// gemaskgen.
static const int kDefaultMaxInMemorySize = 16000;


// This class provides out-of-memory flood filling for output masks that are in
// GDAL format.  FloodFillRPImagery and FloodFillRPTerrain provide the input
// routines to read Imagery and Terrain respectively into the input tiles.
class GDALMaskFloodFill : public TiledFloodFill {
 private:
  static const int kNotTouched = 256;  // flag tiles that haven't been touched

 public:
  GDALMaskFloodFill(GDALRasterBand *alpha,
                    int image_width, int image_height,
                    int tile_width, int tile_height,
                    int tolerance,
                    int hole_size)
      : TiledFloodFill(image_width, image_height,
                       tile_width, tile_height,
                       tolerance,
                       hole_size),
        alpha_(alpha),
        image_tile_(new unsigned char[tile_width * tile_height]),
        mask_tile_(new unsigned char[tile_width * tile_height]),
        last_tile_read_(-1),
        opacities_(num_tiles_x_ * num_tiles_y_, kNotTouched) {}

  virtual ~GDALMaskFloodFill() {
    delete [] image_tile_;
    delete [] mask_tile_;
  }

  // DoFloodFill() sets the corners of the image as seeds, performs the
  // floodfill, and writes any untouched tiles. All fill values must be set
  // before calling.
  void DoFloodFill() {
    AddSeed(0, image_height_ - 1);
    AddSeed(image_width_ - 1, image_height_ - 1);
    AddSeed(0, 0);
    AddSeed(image_width_ - 1, 0);

    FloodFill();

    // Now write out all tiles that haven't been touched.
    int untouched_count = 0;
    for (int tile_x = 0; tile_x <num_tiles_x_; ++tile_x)
      for (int tile_y = 0; tile_y < num_tiles_y_; ++tile_y)
        if (opacities_[tile_x + tile_y * num_tiles_x_] == kNotTouched)
          ++untouched_count;
    if (untouched_count > 0) {
      notify(NFY_NOTICE, "Saving untouched tiles");
      khProgressMeter progress(untouched_count);
      memset(mask_tile_, kNotFilled, tile_width_ * tile_height_);
      for (int tile_x = 0; tile_x <num_tiles_x_; ++tile_x)
        for (int tile_y = 0; tile_y < num_tiles_y_; ++tile_y)
          if (opacities_[tile_x + tile_y * num_tiles_x_] == kNotTouched) {
            last_tile_read_ = tile_y * num_tiles_x_ + tile_x;
            SaveMaskTile(tile_x, tile_y, kNotFilled);
            progress.incrementDone();
          }
    }
  }

  virtual unsigned char *LoadMaskTile(int tile_x, int tile_y, double *old_opacity) {
    double &tile_opacity = opacities_[tile_x + tile_y * num_tiles_x_];
    if (tile_opacity == kNotTouched) {
      tile_opacity = kNotFilled;
      notify(NFY_VERBOSE, "all blank mask tile %d,%d", tile_x, tile_y);
      memset(mask_tile_, kNotFilled, tile_width_ * tile_height_);
    } else {
      int xsz = std::min(tile_width_, image_width_ - tile_x * tile_width_),
          ysz = std::min(tile_height_, image_height_ - tile_y * tile_height_);
      notify(NFY_VERBOSE, "mask rasterio offset (%d,%d) size (%d,%d)",
             tile_x * tile_width_, tile_y * tile_height_, xsz, ysz);
      if (alpha_->RasterIO(GF_Read, tile_x * tile_width_, tile_y * tile_height_,
                           xsz, ysz,
                           mask_tile_,
                           xsz, ysz,
                           GDT_Byte, 0, tile_width_) == CE_Failure)
        notify(NFY_FATAL, "Could not load mask tile %d,%d.", tile_x, tile_y);
    }
    last_tile_read_ = tile_y * num_tiles_x_ + tile_x;
    *old_opacity = tile_opacity;
    return mask_tile_;
  }

  virtual void SaveMaskTile(int tile_x, int tile_y, double opacity) {
    if (tile_y * num_tiles_x_ + tile_x != last_tile_read_)
      notify(NFY_FATAL, "Saving different tile from loaded");
    opacities_[tile_x + tile_y * num_tiles_x_] = opacity;
    int xsz = std::min(tile_width_, image_width_ - tile_x * tile_width_),
        ysz = std::min(tile_height_, image_height_ - tile_y * tile_height_);

    notify(NFY_VERBOSE,
           "save mask rasterio offset (%d,%d) size (%d,%d) opacity %f",
           tile_x * tile_width_, tile_y * tile_height_, xsz, ysz, opacity);

    if (alpha_->RasterIO(GF_Write, tile_x * tile_width_, tile_y * tile_height_,
                         xsz, ysz,
                         mask_tile_,
                         xsz, ysz,
                         GDT_Byte, 0, tile_width_) == CE_Failure)
      notify(NFY_FATAL, "Could not save mask tile %d,%d.", tile_x, tile_y);
  }

 protected:
  GDALRasterBand *alpha_;
  unsigned char *image_tile_;
  unsigned char *mask_tile_;
  int last_tile_read_;  // used to ensure that the saved tile is same as read
  std::vector<double> opacities_;
};

// This class provides out-of-memory flood filling for an imagery Raster
// Product.  Given the base class, it mainly adds the fill values based on
// corners plus the ability to load imagery data into tiles as needed by the
// base class algorithm.
class FloodFillRPImagery : public GDALMaskFloodFill {
 public:
  FloodFillRPImagery(const khRasterProductLevel &rp_level,
                     unsigned int mask_band,
                     khExtents<std::uint32_t> extents,
                     GDALRasterBand *alpha,
                     int image_width, int image_height,
                     int tile_width, int tile_height,
                     int tolerance,
                     int hole_size)
      : GDALMaskFloodFill(alpha,
                          image_width, image_height,
                          tile_width, tile_height,
                          tolerance,
                          hole_size),
        rp_level_(rp_level), band_(mask_band), extents_(extents) {
  }

  ~FloodFillRPImagery() {
  }

  // Perform flood fill for an image.
  void FloodFillImagery(int fill_value, bool fill_white) {
    // Set fill values.
    if (fill_value != TiledFloodFill::kNoFillValueSpecified) {
      AddFillValue(fill_value);
    } else {
      LoadImageTile(0, 0);
      AddFillValue(image_tile_[0]);
      LoadImageTile(num_tiles_x_ - 1, 0);
      AddFillValue(image_tile_[(image_width_ - 1) % tile_width_]);
      LoadImageTile(0, num_tiles_y_ - 1);
      AddFillValue(image_tile_[((image_height_ - 1) % tile_height_) *
                               tile_width_]);
      LoadImageTile(num_tiles_x_ - 1, num_tiles_y_ - 1);
      AddFillValue(image_tile_[((image_height_ - 1) % tile_height_)
                               * tile_width_
                               + (image_width_ - 1) % tile_width_]);
    }
    if (fill_white) AddFillValue(255);

    // Then perform the flood fill, writing all mask tiles.
    DoFloodFill();
  }

  virtual const unsigned char *LoadImageTile(int tile_x, int tile_y) {
    khExtents<std::uint32_t> tileExtents(
        XYOrder,
        extents_.beginX() + tile_x * tile_width_,
        extents_.beginX() + (tile_x+1) * tile_width_,
        (extents_.beginY() +
         std::max(0, image_height_ - (tile_y + 1) * tile_height_)),
        extents_.beginY() + image_height_ - tile_y * tile_height_);
    notify(NFY_VERBOSE, "requesting tile %d,%d: origin (%d,%d), size %dx%d",
           tile_x, tile_y,
           tileExtents.beginX(), tileExtents.beginY(),
           tileExtents.width(), tileExtents.height());
    rp_level_.ReadImageIntoBufs(tileExtents,
                                &image_tile_, &band_,
                                1 /* numbands */,
                                khTypes::UInt8);
    return image_tile_;
  }
 private:
  const khRasterProductLevel &rp_level_;
  unsigned int band_;
  khExtents<std::uint32_t> extents_;
};


// Reads terrain data from a Raster Product into the buffer. Used by both class
// FloodFillRPTerrain for tiled flooding and main() for in-memory flooding.
void ReadTerrainToBuffer(const khRasterProductLevel &rp_level,
                         const int terrain_band,
                         const khExtents<std::uint32_t> &extract_extents,
                         float *terrain_buffer) {
  unsigned char *read_buffers[] = {reinterpret_cast<uchar *>(&terrain_buffer[0])};
  unsigned int read_bands[] = {static_cast< unsigned int> (terrain_band)};
  // Read terrain data into memory.
  rp_level.ReadImageIntoBufs(extract_extents, read_buffers, read_bands,
                             1 /* numbands */, khTypes::Float32);
}

// A class to represent fill values for float input (terrain). If a value is >=
// interval.first and <= interval.second for at least one interval in a
// FillRangeSet, then it is a fill value.
typedef std::pair<float, float> Interval;
typedef std::vector<Interval> FillRangeSet;

// Convert the float terrain data to a unsigned char image in image_buffer.  All terrain
// "pixels" that are in ranges in fill_values are 0 and all other pixels are
// 255. Used by both class FloodFillRPTerrain for tiled flooding and main() for
// in-memory flooding.
void ConvertTerrainToImage(const float *terrain_buffer,
                           const int width, const int height,
                           const FillRangeSet &fill_values,
                           unsigned char *image_buffer) {
  // Convert to unsigned char data, converting to 0 when the pixel may be filled and 255
  // when it should not be filled.
  for (int x = 0; x < width; ++x)
    for (int y = 0; y < height; ++y) {
      float elev = terrain_buffer[y * width + x];
      char pixel_value = 255;
      for (unsigned int i = 0; i < fill_values.size(); ++i)
        if (elev >= fill_values[i].first &&
            elev <= fill_values[i].second) {
          pixel_value = 0;
          break;
        }
      image_buffer[y * width + x] = pixel_value;
    }
}

// FloodFillTerrain provides out-of-memory flood filling for terrain input. It
// performs the correct transformations from terrain input to the unsigned char images
// filled by the base class.
class FloodFillRPTerrain : public GDALMaskFloodFill {
 public:
  FloodFillRPTerrain(const khRasterProductLevel &rp_level,
                     int terrain_band,
                     khExtents<std::uint32_t> extents,
                     GDALRasterBand *alpha,
                     int image_width, int image_height,
                     int tile_width, int tile_height,
                     int hole_size)
      : GDALMaskFloodFill(alpha,
                          image_width, image_height,
                          tile_width, tile_height,
                          0,  // tolerance
                          hole_size),
        rp_level_(rp_level), band_(terrain_band), extents_(extents) {
  }

  ~FloodFillRPTerrain() {
  }

  // Perform flood fill for terrain.
  void FloodFillTerrain(const FillRangeSet &fill_values_input,
                        float tolerance) {
    // If no fill values given, then use corner values +/- tolerance.
    fill_values_ = fill_values_input;
    if (fill_values_.empty()) {
      khDeleteGuard<float, ArrayDeleter> terrain_tile(
        TransferOwnership(new float[tile_width_ * tile_height_]));
      ReadTerrainTile(0, 0, terrain_tile);
      float elev = terrain_tile[0];
      fill_values_.push_back(Interval(elev - tolerance, elev + tolerance));
      ReadTerrainTile(num_tiles_x_ - 1, 0, terrain_tile);
      elev = terrain_tile[(image_width_ - 1) % tile_width_];
      fill_values_.push_back(Interval(elev - tolerance, elev + tolerance));
      ReadTerrainTile(0, num_tiles_y_ - 1, terrain_tile);
      elev = terrain_tile[((image_height_ - 1) % tile_height_) * tile_width_];
      fill_values_.push_back(Interval(elev - tolerance, elev + tolerance));
      ReadTerrainTile(num_tiles_x_ - 1, num_tiles_y_ - 1, terrain_tile);
      elev = terrain_tile[((image_height_ - 1) % tile_height_) * tile_width_ +
                          (image_width_ - 1) % tile_width_];
      fill_values_.push_back(Interval(elev - tolerance, elev + tolerance));
    }

    // Fill all 0-valued pixels in the unsigned char images returned from
    // ConvertTerrainToImage().
    AddFillValue(0);

    // Then perform the flood fill, writing all mask tiles.
    DoFloodFill();
  }

  // Read the given tile of terrain data into the buffer.
  void ReadTerrainTile(int tile_x, int tile_y, float *terrain_buffer) {
    khExtents<std::uint32_t> tile_extents(
        XYOrder,
        extents_.beginX() + tile_x * tile_width_,
        extents_.beginX() + (tile_x+1) * tile_width_,
        (extents_.beginY() +
         std::max(0, image_height_ - (tile_y + 1) * tile_height_)),
        extents_.beginY() + image_height_ - tile_y * tile_height_);
    notify(NFY_VERBOSE, "reading tile %d, %d: origin (%d, %d), size %dx%d",
           tile_x, tile_y,
           tile_extents.beginX(), tile_extents.beginY(),
           tile_extents.width(), tile_extents.height());
    ReadTerrainToBuffer(rp_level_, band_, tile_extents, terrain_buffer);
  }

  // Read the given tile of terrain data, and convert to a unsigned char image with the
  // appropriate fill values.
  virtual const unsigned char *LoadImageTile(int tile_x, int tile_y) {
    khDeleteGuard<float, ArrayDeleter> terrain_buffer(
      TransferOwnership(new float[tile_width_ * tile_height_]));
    ReadTerrainTile(tile_x, tile_y, terrain_buffer);
    ConvertTerrainToImage(terrain_buffer, tile_width_, tile_height_,
                          fill_values_, image_tile_);
    return image_tile_;
  }
 private:
  const khRasterProductLevel &rp_level_;
  int band_;
  khExtents<std::uint32_t> extents_;
  FillRangeSet fill_values_;
};



void
usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\n"
          "usage: %s [mode] [options] <rasterproduct> <output>\n"
          "   Supported modes are:\n"
          "      --mask:          A 8 bit mask file is generated (default)\n"
          "      --prep:          A 8 bit mask-prep file is generated\n"
          "      --preview:       An RGBA preview file is generated\n"
          "      --extract:       Extract image from product. No mask work done\n"
          "                       Output datatype will match product\n"
          "   Supported options are:\n"
          "      --help | -?:     Display this usage message\n"
          "      --formats:       Show supported output formats\n"
          "      --debug:         Read input & compute size, don't write\n"
          "      --oformat:       Output file format [default GTiff] (see --formats)\n"
          "      --co NAME=VALUE: Pass construction arguments to output GDAL dataset.\n"
          "                       May use several times.\n"
          "      --maxsize <num>: Maximum mask size [default %d]\n"
          "      --maxsizeinmem <num>: Maximum size processed in-memory [default %d]\n"
          "      --band <num>:    Band number (0 based) to use for mask and prep\n"
          "                       [default 1 for imagery, 0 for terrain]\n"
          "      --fill <0-255>:  Pixel value to treat as fill [default corner colors]\n"
          "      --tolerance <num>: Difference from --fill that is still considered\n"
          "                         to be fill [default 0]\n"
          "      --fillvalues <list>: Comma-separated list of fill values and ranges\n"
          "                           with no spaces (e.g. \"-9999,-999.9:0\")\n"
          "                           Note: only one range is allowed for\n"
          "                           imagery inputs while more than one is\n"
          "                           allowed for terrain inputs.\n"
          "      --feather <num>: Number of pixels to feather [default no feather]\n"
          "      --holesize <num>: Interior holes of fill color will be masked\n"
          "                        if they are at least this big. [default don't\n"
          "                        look for holes\n"
          "      --whitefill:     Also fill blocks of white [default false]\n"

          , progn.c_str(), kDefaultMaxMaskSize, kDefaultMaxInMemorySize);
  exit(1);
}


// Given a list of values and ranges in a string, parses and pushes Intervals
// onto the back of a FillRangeSet.
// Examples:
//   "" --> []
//   "0,1:3" --> [Interval(0,0), Interval(1,3)]
//   "-99.9:-0.1,-9999" --> [Interval(-99.9,-0.1), Interval(-9999,-9999)]
// Throws a fatal error on failure to parse.

void ParseFillValues(std::string fill_string, FillRangeSet *fill_values) {
  using std::string;
  std::vector<string> ranges;
  TokenizeString(fill_string, ranges, ",");
  for (unsigned int i = 0; i < ranges.size(); ++i) {
    const string &range = ranges[i];
    string::size_type colon_i = range.find_first_of(":");
    if (colon_i == string::npos) {
      float value;
      if (!FromStringStrict(range, &value))
        notify(NFY_FATAL,
               "Failed to parse fillvalue range \"%s\", should be start:end",
               range.c_str());
      fill_values->push_back(Interval(value, value));
    } else {
      string start_str(range, 0, colon_i);
      string end_str(range, colon_i + 1, range.length() - colon_i - 1);
      float start, end;
      if (!FromStringStrict(start_str, &start) ||
          !FromStringStrict(end_str, &end))
        notify(NFY_FATAL,
               "Failed to parse fillvalue range \"%s\", should be start:end",
               range.c_str());
      if (end < start)
        notify(NFY_FATAL, "fillvalue range start %g is greater than end %g",
               start, end);
      fill_values->push_back(Interval(start, end));
    }
  }
}

enum Mode {MASK_MODE,     // generate mask of input
           PREP_MODE,     // extract level of RP converted to mask
                          //   with no flood or feathering
           PREVIEW_MODE,  // generate mask for GUI preview icon
           EXTRACT_MODE}; // just extract level of RP

}  // namespace


int main(int argc, char *argv[]) {
  try {
    std::string progname = argv[0];

    // process commandline options
    int argn;
    bool help = false;
    Mode mode = MASK_MODE;
    bool debug = false;
    unsigned int max_size = kDefaultMaxMaskSize;
    unsigned int max_size_in_memory = kDefaultMaxInMemorySize;
    // if fill_value not given, use corner color.
    int fill_value = TiledFloodFill::kNoFillValueSpecified;
    unsigned int tolerance = 0;
    unsigned int feather_radius = 0;
    unsigned int band = 1;
    unsigned int hole_size = 0;
    bool show_formats = 0;
    bool fill_white = false;
    std::string oformat("GTiff");
    OptionsMap output_options_map;
    std::string fill_values_string;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.setOpt("mask",    mode, MASK_MODE);
    options.setOpt("prep",    mode, PREP_MODE);
    options.setOpt("preview", mode, PREVIEW_MODE);
    options.setOpt("extract", mode, EXTRACT_MODE);
    options.flagOpt("debug", debug);
    options.opt("maxsize", max_size);
    options.opt("maxsizeinmem", max_size_in_memory);
    options.opt("fill", fill_value);
    options.opt("tolerance", tolerance);
    options.opt("fillvalues", fill_values_string);
    options.opt("feather", feather_radius);
    options.opt("band", band);
    options.opt("holesize", hole_size);
    options.flagOpt("formats", show_formats);
    options.opt("oformat", oformat, ValidateGDALCreateFormat);
    options.mapOpt("co", output_options_map);
    options.flagOpt("whitefill", fill_white);
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }
    if (show_formats) {
      printf("The following format drivers support output:\n");
      PrintGDALCreateFormats(stdout, "  ");
      exit(0);
    }

    // process rest of commandline arguments
    const char *productfile = GetNextArg();
    if (!productfile) {
      usage(progname, "<rasterproduct> not specified");
    }
    const char *outputfile = GetNextArg();
    if (!outputfile) {
      usage(progname, "<output> not specified");
    }

    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;
    input_files.push_back(productfile);
    khPrintFileSizes("Input File Sizes", input_files);

    // Convert output options to format required by GDAL routines
    int len = 0;
    for (OptionsMap::iterator option = output_options_map.begin();
         option != output_options_map.end(); ++option) {
      len += 2 + option->first.length() + option->second.length();
    }
    const int options_buffer_size = len;
    khDeleteGuard<char, ArrayDeleter>
      options_buffer(TransferOwnership(new char[options_buffer_size]));
    khDeleteGuard<char *, ArrayDeleter> output_options;
    if (output_options_map.size()) {
      output_options =
        TransferOwnership(new char *[output_options_map.size() + 1]);
      int ii = 0;
      len = 0;
      for (OptionsMap::iterator option = output_options_map.begin();
           option != output_options_map.end(); ++option, ++ii) {
        output_options[ii] = &options_buffer[len];
        snprintf(&options_buffer[len], options_buffer_size - len, "%s=%s",
                option->first.c_str(), option->second.c_str());
        len += 2 + option->first.length() + option->second.length();
      }
      output_options[ii] = 0;
    }

    // Parse fillvalues input and setup fill_values set.
    FillRangeSet fill_values;
    if (fill_value != TiledFloodFill::kNoFillValueSpecified)
      fill_values.push_back(Interval(fill_value - tolerance,
                                     fill_value + tolerance));
    ParseFillValues(fill_values_string, &fill_values);

    // Special case: if fill is unspecified but only one fillvalues range is
    // specified, then we will override fill_value and tolerance to
    // use the fill_values pair.
    // This makes a single fillvalues range usable for imagery.
    if (fill_value == TiledFloodFill::kNoFillValueSpecified &&
        fill_values.size() == 1) {
      // Take the fill_values setting and apply to the fill_value and tolerance.
      // This is necessary to optimize the mask generation for the single
      // interval case.
      fill_value = static_cast<int>(fill_values[0].first);
      tolerance = static_cast< unsigned int> (fill_values[0].second - fill_value);
    }

    // load the output driver
    khGDALInit();
    GDALDriver *driver =
      reinterpret_cast<GDALDriver*>(GDALGetDriverByName(oformat.c_str()));
    if (!driver) {
      notify(NFY_FATAL, "Internal Error: Unable to load GDAL driver %s",
             oformat.c_str());
    }

    khDeleteGuard<khRasterProduct> rp(khRasterProduct::Open(productfile));
    if (!rp) {
      notify(NFY_FATAL, "Unable to open raster product file %s",
             productfile);
    }

    if ((rp->type() != khRasterProduct::Imagery) &&
        (rp->type() != khRasterProduct::Heightmap)) {
      notify(NFY_FATAL, "Raster product must be imagery or heightmap");
    }

    // Imagery mask generation was only built with a single "fillvalues" range
    // in mind.
    if (rp->type() == khRasterProduct::Imagery && fill_values.size() > 1) {
      notify(NFY_FATAL,
             "Imagery masks can only be generated using a single range for "
             "the fillvalues argument.");
    }

    // Report some information about the raster product.
    notify(NFY_NOTICE, "raster type: %s", ToString(rp->type()).c_str());
    notify(NFY_NOTICE, "num components: %d", rp->numComponents());
    notify(NFY_NOTICE, "component size: %d", rp->componentSize());
    notify(NFY_NOTICE, "component type: %s",
           ToString(rp->componentType()).c_str());

    // Automatically adjust level for generating alpha mask.
    unsigned int extract_level = rp->maxLevel();  // maxLevel() guaranteed > 0
    bool is_mercator = rp->projectionType() == khTilespace::MERCATOR_PROJECTION;
    khSize<std::uint64_t> checkSize = is_mercator ?
        MeterExtentsToPixelLevelRasterSize(rp->degOrMeterExtents(),
                                           extract_level) :
        DegExtentsToPixelLevelRasterSize(rp->degOrMeterExtents(),
                                         extract_level);
    notify(NFY_NOTICE, "min, max product levels: %u, %u",
           rp->minLevel(), rp->maxLevel());

    if (checkSize.degenerate()) {
      notify(
          NFY_WARN,
          "The raster product has invalid image size, pixels: (wh) %lu, %lu",
          checkSize.width, checkSize.height);
      notify(NFY_FATAL, "Unable to generate mask.");
    }

    notify(NFY_NOTICE,
           "image size at max product level, pixels: (wh) %lu, %lu",
           checkSize.width, checkSize.height);

    if (band >= rp->numComponents()) {
      band = 0;
    }

    notify(NFY_NOTICE, "Mask parameters:");
    notify(NFY_NOTICE, "band selection: %d", band);

    while (checkSize.width > max_size || checkSize.height > max_size) {
      if (extract_level <= rp->minLevel()) {
        notify(NFY_WARN,
               "Cannot find a level to satisfy max_size requirements");
        break;
      }
      --extract_level;
      checkSize = is_mercator?
          MeterExtentsToPixelLevelRasterSize(rp->degOrMeterExtents(),
                                             extract_level) :
          DegExtentsToPixelLevelRasterSize(rp->degOrMeterExtents(),
                                           extract_level);
    }

    notify(NFY_NOTICE, "automatic level selection for generating mask: %u",
           extract_level);

    if (checkSize.degenerate()) {
      notify(NFY_WARN,
             "Cannot find a level to satisfy max_size requirements");
      notify(NFY_WARN,
             "Invalid output image size at level %u, pixels: (wh) %lu, %lu",
             extract_level,
             checkSize.width, checkSize.height);
      notify(NFY_FATAL, "Unable to generate mask.");
    }

    // calculate output image geo extents
    khGeoExtents outputGeoExtents(extract_level, rp->degOrMeterExtents(),
                                  true /* top->bottom */, is_mercator);

    // calculate extract_extents (pixel extents w/in the tiles of the level)
    // (i.e. 0,0 is the corner of the first tile containing the extents)
    khLevelCoverage tileCov(rp->levelCoverage(extract_level));
    khExtents<double> degTileExtents(is_mercator
        ? tileCov.meterExtents(RasterProductTilespaceBase)
        : tileCov.degExtents(RasterProductTilespaceBase));
    double pixelsize = is_mercator ?
        RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(
            extract_level) :
        RasterProductTilespaceFlat.DegPixelSize(extract_level);
    khOffset<std::uint32_t>
      extractOffset(XYOrder,
                    static_cast<std::uint32_t>(((rp->degOrMeterExtents().beginX() -
                                          degTileExtents.beginX()) / pixelsize)
                                        + 0.5),
                    static_cast<std::uint32_t>(((rp->degOrMeterExtents().beginY() -
                                          degTileExtents.beginY()) / pixelsize)
                                        + 0.5));
    khSize<std::uint32_t> extract_size(checkSize.width, checkSize.height);
    khExtents<std::uint32_t>extract_extents(extractOffset, extract_size);
    notify(NFY_NOTICE, "output image offset, pixels: (xy) %u, %u",
           extract_extents.beginX(), extract_extents.beginY());
    notify(NFY_NOTICE, "output image size, pixels: (wh) %u, %u",
           extract_extents.width(), extract_extents.height());

    if (extract_extents.degenerate()) {
      notify(NFY_WARN, "Invalid output image size, pixels: (wh) %u, %u",
           extract_extents.width(), extract_extents.height());
      notify(NFY_FATAL, "Unable to generate mask.");
    }

    // GTiff gets very slow to write to when >4G. Warn the user in this case.
    if ((static_cast<std::int64_t>(extract_extents.width()) *
         static_cast<std::int64_t>(extract_extents.height())) >
        std::numeric_limits<std::uint32_t>::max() / 4 &&
        oformat == "GTiff")
      notify(NFY_WARN, "Output format GTiff is slow for large masks."
             " Consider \"-oformat HFA\".");

    // Adjust the feather radius and hole size to account for minimization.
    float minify_factor = powf(2, rp->maxLevel() - extract_level);
    int adjusted_feather_radius =
      static_cast<int>(feather_radius / minify_factor + 0.5);
    int adjusted_hole_size = static_cast<int>(hole_size / minify_factor + 0.5);
    // Holes larger than the hole size are to be detected, so don't turn off
    // hole detection completely if it was on.
    if (hole_size > 0 && adjusted_hole_size == 0)
      adjusted_hole_size = 1;
    notify(NFY_NOTICE, "adjusted feather size: %d", adjusted_feather_radius);
    notify(NFY_NOTICE, "adjusted hole size: %d", adjusted_hole_size);
    bool in_memory = (extract_size.width  <= max_size_in_memory &&
                      extract_size.height <= max_size_in_memory);

    if (debug) {
      notify(NFY_NOTICE, "--debug requested. Exiting.");
      exit(0);
    }

    // legacy from old gemaskgen: related to feathering only inside
    // the opaque region.
    adjusted_feather_radius = static_cast<int>(adjusted_feather_radius *
                                               (2.0 -
                                                Featherer::kFeatherPRatio));

    if (mode == EXTRACT_MODE) {
      geImageWriter::ExtractLevelImage(rp->level(extract_level),
                                       extract_extents,
                                       driver, outputfile,
                                       outputGeoExtents,
                                       is_mercator,
                                       output_options);
    } else {
      if (in_memory) {
        // Extract band for alpha generation
        khDeleteGuard<unsigned char, ArrayDeleter> read_alpha_buf(
          TransferOwnership(new unsigned char[extract_size.width*extract_size.height]));
        if (rp->type() == khRasterProduct::Heightmap) {
          // If the image is a heightmap, convert it to a unsigned char image where
          // all maskable pixels are 0 and non-maskable pixels are 255. Then
          // floodfill 0-valued pixels in the image.
          int width = extract_extents.width();
          int height = extract_extents.height();
          khDeleteGuard<float, ArrayDeleter> terrain_buffer(
            TransferOwnership(new float[width * height]));
          ReadTerrainToBuffer(rp->level(extract_level), band,
                              extract_extents, terrain_buffer);
          if (fill_values.empty()) {
            float elev = terrain_buffer[0];
            fill_values.push_back(Interval(elev - tolerance, elev + tolerance));
            elev = terrain_buffer[width - 1];
            fill_values.push_back(Interval(elev - tolerance, elev + tolerance));
            elev = terrain_buffer[(height - 1) * width];
            fill_values.push_back(Interval(elev - tolerance, elev + tolerance));
            elev = terrain_buffer[height * width - 1];
            fill_values.push_back(Interval(elev - tolerance, elev + tolerance));
          }
          ConvertTerrainToImage(terrain_buffer, width, height, fill_values,
                                read_alpha_buf);
          fill_value = 0;
          tolerance = 0;
        } else {
          unsigned char *readBufs[]  = {&read_alpha_buf[0]};
          unsigned int   readBands[] = {band};
          // will succeed or throw trying
          rp->level(extract_level).ReadImageIntoBufs(extract_extents,
                                                     readBufs, readBands,
                                                     1 /* numbands */,
                                                     khTypes::UInt8);
        }

        // Calculate alpha mask.
        khDeleteGuard<unsigned char, ArrayDeleter> calc_alpha_buf;
        unsigned char *alpha_buf = NULL;
        if (mode != PREP_MODE) {
          calc_alpha_buf = TransferOwnership(new unsigned char[extract_size.width *
                                                       extract_size.height]);
          notify(NFY_NOTICE, "Flood filling image in memory");
          InMemoryFloodFill maskgen(read_alpha_buf, calc_alpha_buf,
                                    extract_size.width,
                                    extract_size.height,
                                    fill_value, tolerance,
                                    fill_white,
                                    adjusted_hole_size);
          maskgen.FloodFill();
          if (adjusted_feather_radius) {
            notify(NFY_NOTICE, "Feathering mask in memory");
            InMemoryFeatherer featherer(calc_alpha_buf,
                                        extract_size.width,
                                        extract_size.height,
                                        adjusted_feather_radius,
                                        0 /* border value */);
          }
          alpha_buf = calc_alpha_buf;
        } else {
          alpha_buf = read_alpha_buf;
        }

        if (mode == PREVIEW_MODE) {
          if (rp->type() == khRasterProduct::Imagery) {
            geImageWriter::WriteImageryPreview(rp->level(extract_level),
                                               extract_extents, alpha_buf,
                                               driver, outputfile,
                                               outputGeoExtents,
                                               is_mercator,
                                               output_options);
          } else {
            geImageWriter::WriteHeightmapPreview(rp->level(extract_level),
                                                 extract_extents, alpha_buf,
                                                 driver, outputfile,
                                                 outputGeoExtents,
                                                 is_mercator,
                                                 output_options);
          }
        } else {
          geImageWriter::WriteAlphaImage(extract_size, alpha_buf,
                                         driver, outputfile, outputGeoExtents,
                                         is_mercator, output_options);
        }
      } else {  // Calculate mask on disk
        if (mode != MASK_MODE) {
          notify(NFY_FATAL, "Image too large to fit in memory. "
                 "Only mask mode implemented Out Of Core.");
        }

        if (!khEnsureParentDir(outputfile)) {
          notify(NFY_FATAL, "Unable to mkdir %s",
                 khDirname(outputfile).c_str());
        }

        // Create on-disk output dataset.
        GDALDataset *poDstDS = driver->Create(outputfile, extract_size.width,
                                              extract_size.height,
                                              1 /*band*/,
                                              GDT_Byte, output_options);

        // Set metadata for output mask.
        poDstDS->SetGeoTransform((double*)outputGeoExtents.geoTransform());
        poDstDS->SetProjection(GetWGS84ProjString().c_str());
        GDALRasterBand *poAlphaBand = poDstDS->GetRasterBand(1);

        // Choose a good tile size.  Some rough tests Feb08 by randyw found that
        // 1024^2 tiles ran faster than 512^2 or 2048^2, among other sizes
        // tried. Tiles that spanned the image in X also gave slower results.
        int tile_width = 1024;
        int tile_height = 1024;
        notify(NFY_NOTICE, "Flood filling image in %d x %d tiles",
               tile_width, tile_height);

        // Create alpha mask.
        if (rp->type() == khRasterProduct::Heightmap) {
          FloodFillRPTerrain maskgen(rp->level(extract_level),
                                     band,
                                     extract_extents,
                                     poAlphaBand,
                                     extract_size.width,
                                     extract_size.height,
                                     tile_width, tile_height,
                                     adjusted_hole_size);
          maskgen.FloodFillTerrain(fill_values, tolerance);
        } else {
          FloodFillRPImagery maskgen(rp->level(extract_level),
                                     band,
                                     extract_extents,
                                     poAlphaBand,
                                     extract_size.width,
                                     extract_size.height,
                                     tile_width, tile_height,
                                     tolerance,
                                     adjusted_hole_size);
          maskgen.FloodFillImagery(fill_value, fill_white);
        }

        // Feather the mask.
        if (adjusted_feather_radius) {
          notify(NFY_NOTICE, "Feathering mask in %d x %d tiles",
                 tile_width, tile_height);
          Featherer featherer(poAlphaBand,
                              // ensure block width is larger than filter radius
                              std::max(tile_width, adjusted_feather_radius),
                              tile_height,
                              adjusted_feather_radius,
                              0 /* border value */);
        }
        // Close dataset and force pending writes to flush.
        delete poDstDS;
      }
    }

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(outputfile);
    khPrintFileSizes("Output File Sizes", output_files);
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
