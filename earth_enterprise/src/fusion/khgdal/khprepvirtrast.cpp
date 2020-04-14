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


// Description: Converts khprep tool xml to virtual raster xml


#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gdal_priv.h>
#include <algorithm>
#include <fstream>

#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>

#include <notify.h>
#include <khFileUtils.h>
#include <gst/gstBBox.h>
#include <khgdal/khgdal.h>
#include <khstrconv.h>
#include <khGetopt.h>
#include <khstl.h>
#include <khGuard.h>

#include <khgdal/.idl/khVirtualRaster.h>
#include "khLUT.h"
#include "khGDALDataset.h"

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }
  //fprintf(stderr, "%s <input xml> <output khvr>") ;
  fprintf(stderr,
          "\nusage: %s [options] -i <input.xml> -o <output.khvr>\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "      --lut <lutfile>:  LUT file for entire image\n"
          "      --relax:          Relax constraints on inputs\n"
          "      --fill a,b,... :  Band values to use as fill\n"
          "      --tolerance <num> :  Tolerance to be applied to fill (default 0)\n"
          "      --crop pixelx,pixely,pixelw,pixelh: Crop to given pixel extents\n",
          progn.c_str());
  exit(1);
}


bool operator==(const GDALColorEntry &a, const GDALColorEntry &b)
{
  return ((a.c1 == b.c1) &&
          (a.c2 == b.c2) &&
          (a.c3 == b.c3) &&
          (a.c4 == b.c4));
}

bool operator!=(const GDALColorEntry &a, const GDALColorEntry &b)
{
  return !(a == b);
}

class ColorTable
{
 public:
  GDALPaletteInterp interp;
  std::vector<GDALColorEntry> entries;

  ColorTable(void) : interp(GDALPaletteInterp()) { }
  ColorTable(const GDALColorTable &o) :
      interp(o.GetPaletteInterpretation())
  {
    int count = o.GetColorEntryCount();
    for (int i = 0; i < count; ++i) {
      entries.push_back(*o.GetColorEntry(i));
    }
  }

  bool operator==(const ColorTable &o) {
    return ((interp == o.interp) &&
            (entries == o.entries));
  }
  bool operator!=(const ColorTable &o) {
    return !operator==(o);
  }
  bool operator==(const GDALColorTable &o) {
    if (interp != o.GetPaletteInterpretation())
      return false;
    if ((int)entries.size() != o.GetColorEntryCount())
      return false;
    for (unsigned int i = 0; i < entries.size(); ++i) {
      if (entries[i] != *o.GetColorEntry(i))
        return false;
    }
    return true;
  }
  bool operator!=(const GDALColorTable &o) {
    return !operator==(o);
  }
};


bool prep_process(
  // variation of main process that gets data from an xml supplied by prep process
  // such xmls are specified with the --prep <file> option.
  bool validate,
  bool relax,
  std::string input,
  std::string output,
  std::string tolerancestr,
  std::string lutfilename,
  std::vector<std::string> fill,
  khExtents<std::uint32_t> cropExtents
) {
  khGDALInit();

  // Create and load our Prep Data object, and do some sanity checks on it
  PrepData prep;
  prep.Load(input);
  unsigned int numTiles = prep.tiles.size(); // used here and in tile loop below
  if (numTiles == 0) {
   notify(NFY_FATAL, "No tiles to raster");
  }
  if (prep.bands.size() == 0) {
   notify(NFY_FATAL, "No color bands");
  }

  // make mosaic object and load easy stuff into it
  khVirtualRaster virtraster;
  khExtents<double> mosaicExtents;
  // trim leading spaces and newlines from srs (they mess up some fusion tools)
  while (prep.srs.at(0) == ' ' || prep.srs.at(0) == '\n') prep.srs.erase(0, 1);
  virtraster.srs = prep.srs;
  if (!tolerancestr.empty()) {
    virtraster.fillTolerance = tolerancestr;
  }
  virtraster.pixelsize = prep.tiles[0].pixel_size; // might be increased later

  // load Band prep data into mosaic
  std::vector<PrepData::PrepDataBand>::iterator band_itr;
  for (band_itr=prep.bands.begin(); band_itr != prep.bands.end(); band_itr++) {
    khVirtualRaster::OutputBand outBand;
    outBand.outDatatype = band_itr->datatype;
    outBand.inDatatype = band_itr->datatype;
    outBand.outBlocksize = band_itr->blocksize;
    outBand.colorinterp = band_itr->colorinterp;
    outBand.noDataValue = band_itr->nodata_value;
    virtraster.outputBands.push_back(outBand);
  }

  // Add LUT information to bands if lut file was specified
  try {
    if (!lutfilename.empty()) {
      std::ifstream in(lutfilename.c_str());
      if (!in) {
        notify(NFY_FATAL, "Unable to open lut file %s",
               lutfilename.c_str());
      }
      unsigned int numbands;
      in >> numbands;
      if (!in) {
        notify(NFY_FATAL, "Unable to interpret lut file %s",
               lutfilename.c_str());
      }
      if (numbands != virtraster.outputBands.size()) {
        notify(NFY_FATAL, "Incompatible LUT has %u bands. Imagery has %u.",
               numbands, static_cast<unsigned>(virtraster.outputBands.size()));
      }
      for (unsigned int b = 0; b < numbands; ++b) {
        khVirtualRaster::OutputBand &band = virtraster.outputBands[b];
        GDALDataType intype  = GDTFromName(band.inDatatype);
        // for now the presence of a LUT implies an output type of
        // GDT_Byte
        GDALDataType outtype = GDT_Byte;
        band.outDatatype = GDALGetDataTypeName(outtype);
        switch (intype) {
          case GDT_Byte:
            ParseAndStoreLUT<unsigned char>(in, outtype, band.defaultLut);
            break;
          case GDT_UInt16:
            ParseAndStoreLUT<std::uint16_t>(in, outtype, band.defaultLut);
            break;
          case GDT_Int16:
            ParseAndStoreLUT<std::int16_t>(in, outtype, band.defaultLut);
            break;
          default:
            notify(NFY_FATAL,
                   "Band %d has type %s which cannot be used as input to a LUT",
                   b+1, band.inDatatype.c_str());
        }
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }
  try {
    // process all the tiles
    notify(NFY_INFO, "%u tiles to process", numTiles);
    std::vector<PrepData::PrepDataTile>::iterator ptile;
    for (ptile=prep.tiles.begin(); ptile != prep.tiles.end(); ptile++) {
      notify(NFY_INFO, "Processing %s ...", ptile->path.c_str());
      // confirm that tile pixel sizes are close enough
      if (!relax) {
        if (!CompatiblePixelSizes(
          ptile->pixel_size.width,
          virtraster.pixelsize.width,
          ptile->raster_size.width
        )) {
          notify(NFY_FATAL, "Incompatible pixel width in: %s", ptile->path.c_str());
        }
        if (!CompatiblePixelSizes(
          ptile->pixel_size.height,
          virtraster.pixelsize.height,
          ptile->raster_size.height
        )) {
          notify(NFY_FATAL, "Incompatible pixel height in: %s", ptile->path.c_str());
        }
      }
      // add an input tile and grow our mosaic
      virtraster.inputTiles.push_back(khVirtualRaster::InputTile(
        ptile->path,
        khGeoExtents(ptile->extents, ptile->raster_size).geoTransformOrigin(),
        ptile->raster_size
      ));
      mosaicExtents.grow(ptile->extents);
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }

  // do the summary calculations
  double dX = virtraster.pixelsize.width;
  double dY = virtraster.pixelsize.height;
  std::uint32_t sizeX = (std::uint32_t)((mosaicExtents.width()  / fabs(dX)) + 0.5);
  std::uint32_t sizeY = (std::uint32_t)((mosaicExtents.height() / fabs(dY)) + 0.5);
  notify(NFY_INFO, "overall size = %6u x %6u",
           (unsigned int)sizeX, (unsigned int)sizeY);

  // add the summary info into the mosaic object
  virtraster.origin = khOffset<double>(
    XYOrder,
    (dX > 0) ? mosaicExtents.beginX() : mosaicExtents.endX(),
    (dY > 0) ? mosaicExtents.beginY() : mosaicExtents.endY()
  );
  virtraster.rastersize = khSize<std::uint32_t>(sizeX, sizeY);

  // crop extents if provided
  if (!cropExtents.empty()) {
    khExtents<std::uint32_t> total(
      khOffset<std::uint32_t>(XYOrder, 0,0),
      virtraster.rastersize
    );
    khExtents<std::uint32_t> intersection = khExtents<std::uint32_t>::Intersection(
      total,
      cropExtents
    );
    if (
      (intersection.width() != cropExtents.width()) ||
      (intersection.height() != cropExtents.height())
    ) {
      notify(
        NFY_FATAL,
        "Invalid clip extents (xywh) %u,%u,%u,%u: avail size: (wh) %u,%u",
        cropExtents.beginX(), cropExtents.beginY(),
        cropExtents.width(), cropExtents.height(),
        virtraster.rastersize.width,
        virtraster.rastersize.height
      );
    }
    virtraster.cropExtents = cropExtents;
  }

  // finish up
  if (!validate) {
    // save it out to XML
    if (!virtraster.Save(output)) {
      notify(NFY_FATAL, "Unable to write %s", output.c_str());
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  bool validate = false;
  bool relax = false;
  std::string input;
  std::string output;
  std::string fillstr;
  std::string tolerancestr;
  std::string filelistname;
  std::string lutfilename;
  std::string cropstr;
  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("validate", validate);
  options.flagOpt("relax", relax);
  options.opt("i", input);
  options.opt("output", output);
  options.opt("fill", fillstr);
  options.opt("tolerance", tolerancestr);
  options.opt("filelist", filelistname, &khGetopt::FileExists);
  options.opt("lut", lutfilename, &khGetopt::FileExists);
  options.opt("crop", cropstr);
  if (!options.processAll(argc, argv, argn))
    usage(progname);
  if (help)
    usage(progname);
  // validate, post-process options
  if (output.empty())
    usage(progname, "No output specified");
  if (!khHasExtension(output, ".khvr"))
    usage(progname, "Output file must end in .khvr");
  std::vector<std::string> fill;
  if (fillstr.size()) {
    split(fillstr, ",", back_inserter(fill));
  }
  khExtents<std::uint32_t> cropExtents;
  if (!cropstr.empty()) {
    std::vector<std::string> cropstrs;
    split(cropstr, ",", back_inserter(cropstrs));
    if (cropstrs.size() != 4) {
      usage(progname, "--crop must specify 4 values");
    }
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t w = 0;
    std::uint32_t h = 0;
    FromString(cropstrs[0], x);
    FromString(cropstrs[1], y);
    FromString(cropstrs[2], w);
    FromString(cropstrs[3], h);
    cropExtents = khExtents<std::uint32_t>(khOffset<std::uint32_t>(XYOrder, x, y),
                                    khSize<std::uint32_t>(w, h));
  }

  // get the source files
  khFileList filelist;
  try {
    if (filelistname.size())
      filelist.AddFromFilelist(filelistname);
    if (argn < argc)
      filelist.AddFiles(&argv[argn], &argv[argc]);
  } catch (const std::exception &e) {
    usage(progname, e.what());
  } catch (...) {
    usage(progname, "Unknown error with source files");
  }

  if (input.empty()) {
    usage(progname, "No source files specified");
  }

  if (!prep_process(
    validate,
    relax,
    input,
    output,
    tolerancestr,
    lutfilename,
    fill,
    cropExtents
  )) {
    notify(NFY_FATAL, "Something went wrong with prep_process");
  }
  return 0 ;
}
