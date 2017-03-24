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

#include <notify.h>
#include <string>
#include <vector>
#include <khFileUtils.h>
#include <khGetopt.h>
#include "RasterGenerator.h"

void
usage(const std::string &progn, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\n"
          "usage: %s --help | -?\n"
          "       %s --imagery <sourceimage> -o <output>\n"
          "       %s --heightmap <sourceimage> [--scale <to-meters>] [--nonegatives]\n"
	  "           -o <output>\n"
          "       %s --alphamask <maskimage> --dataproduct <dataproduct> -o <output>\n"
          "  valid options:\n"
          "       --srs <srs> : use supplied srs\n"
          "       --mercator : use mercator projection\n"
          "       --flat : use flat(Plate Carre) projection (default)\n"
          "       --north_boundary <double> Crop to north_boundary"
          " (latitude in decimal deg)\n"
          "       --south_boundary <double> Crop to south_boundary"
          " (latitude in decimal deg)\n"
          "       --east_boundary <double> Crop to east_boundary"
          " (longitude in decimal deg)\n"
          "       --west_boundary <double> Crop to west_boundary"
          " (longitude in decimal deg)\n",

          progn.c_str(), progn.c_str(), progn.c_str(), progn.c_str());
  exit(1);
}

int main(int argc, char *argv[])
{
  std::string progname = argv[0];

  // ***** process command line arguments *****
  int argn;
  bool help = false;
  std::string srcImagery;
  std::string srcHeightmap;
  std::string srcAlphamask;
  std::string dataProduct;
  std::string output;
  std::string overridesrs;
  float       scale = 1.0;
  bool        clampNonnegative = false;
  bool        mercator = false;
  bool        flat = false;
  double north_boundary = 90.0;
  double south_boundary = -90.0;
  double east_boundary = 180.0;
  double west_boundary = -180.0;

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.opt("imagery",     srcImagery,   khGetopt::FileExists);
  options.opt("heightmap",   srcHeightmap, khGetopt::FileExists);
  options.opt("alphamask",   srcAlphamask, khGetopt::FileExists);
  options.opt("dataproduct", dataProduct,  khGetopt::FileExists);
  options.opt("output",      output);
  options.opt("scale",       scale);
  options.opt("srs", overridesrs);
  options.flagOpt("nonegatives", clampNonnegative);
  options.flagOpt("mercator", mercator);
  options.flagOpt("flat", flat);
  options.opt("north_boundary", north_boundary);
  options.opt("south_boundary", south_boundary);
  options.opt("east_boundary", east_boundary);
  options.opt("west_boundary", west_boundary);
  options.setExclusive("flat", "mercator");
  options.setRequired("output");
  if (!options.processAll(argc, argv, argn) || (argn < argc)) {
    usage(progname);
  }
  if (help) {
    usage(progname);
  }
  if (north_boundary <= south_boundary) {
    notify(NFY_FATAL, "north_boundary <= south_boundary!.\n");
  }
  if (east_boundary <= west_boundary) {
    notify(NFY_FATAL, "east_boundary <= west_boundary!.\n");
  }
  khCutExtent::cut_extent = khExtents<double>(
      NSEWOrder, north_boundary, south_boundary, east_boundary, west_boundary);

  if (!flat && !mercator) {
    flat = true;
  }

  // Print the input file sizes for diagnostic log file info.
  std::vector<std::string> input_files;
  input_files.push_back(srcImagery);
  input_files.push_back(srcHeightmap);
  input_files.push_back(srcAlphamask);
  khPrintFileSizes("Input File Sizes", input_files);

  // ***** figure out what to make, and then make it *****
  try {
    if (srcAlphamask.size()) {
      khRasterProduct::RasterType dataRasterType =
        khRasterProduct::Imagery;
      std::string datafile;
      if (dataProduct.size()) {
        datafile = dataProduct;
        khDeleteGuard<khRasterProduct> dataRP(khRasterProduct::Open(datafile));
        if (dataRP) {
          dataRasterType = dataRP->type();
        } else {
          usage(progname,
                "Unable to open %s as raster product",
                datafile.c_str());
        }
      } else if (srcImagery.size()) {
        datafile = srcImagery;
        dataRasterType = khRasterProduct::Imagery;
      } else if (srcHeightmap.size()) {
        datafile = srcHeightmap;
        dataRasterType = khRasterProduct::Heightmap;
      } else {
        usage(progname,
              "You must specify --dataproduct with --alphamask");
      }
      RasterGenerator::MakeAlphaProduct(srcAlphamask, output,
                                        datafile, dataRasterType,
                                        flat ? khTilespace::FLAT_PROJECTION
                                             : khTilespace::MERCATOR_PROJECTION,
                                        overridesrs);
    } else if (srcImagery.size()) {
      RasterGenerator::MakeImageryProduct(
          srcImagery, output, flat ? khTilespace::FLAT_PROJECTION
                                   : khTilespace::MERCATOR_PROJECTION,
                              overridesrs);
    } else if (srcHeightmap.size()) {
      RasterGenerator::MakeHeightmapProduct(srcHeightmap, output, overridesrs,
                                            scale, clampNonnegative);
    } else {
      usage(progname,
            "You must specify --imagery, --heightmap, or --alphamask");
    }

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(output);
    output_files.push_back(dataProduct);
    khPrintFileSizes("Output File Sizes", output_files);

    return 0;
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }


  return 1;
}
