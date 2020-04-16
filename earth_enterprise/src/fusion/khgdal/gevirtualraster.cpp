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



#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gdal_priv.h>
#include <algorithm>
#include <fstream>

#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>

#include <khgdal/.idl/khVirtualRaster.h>
#include "fusion/gst/gstBBox.h"
#include "fusion/khgdal/khgdal.h"
#include "fusion/khgdal/RasterClusterAnalyzer.h"
#include "fusion/khgdal/khLUT.h"
#include "fusion/khgdal/khGDALDataset.h"
#include "common/notify.h"
#include "common/khFileUtils.h"
#include "common/khstrconv.h"
#include "common/khGetopt.h"
#include "common/khstl.h"
#include "common/khGuard.h"

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
          "\nusage: %s [options] -o <output.khvr> { --filelist <file> | <sourcefile> ...}\n"
          "   Source filenames may be specified on the commandline or in a filelist.\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "      --validate:       Validate the inputs and exit\n"
          //          "      --lut <lutfile>:  LUT file for entire image\n"
          //          "      --relax:          Relax constraints on inputs\n"
          "      --fill a,b,... :  Band values to use as fill\n"
          "      --tolerance <num> :  Tolerance to be applied to fill (default 0)\n"
          "      --crop pixelx,pixely,pixelw,pixelh: Crop to given pixel extents\n"
          "      --srs <override srs>: Use given SRS\n\n"
          "Analyze virtual raster:"
          "\nusage: %s [options] --check_area|--check_clusters <input.khvr> \n"
          "      --mercator : use mercator projection\n"
          "      --flat : use flat(Plate Carre) projection (default)\n"
          "      --check_area <input.khvr>: compare sum of source inset areas\n"
          "      to virtual raster area\n"
          "      --check_clusters <input.khvr>: finds source inset sets\n"
          "      connected in tilespace\n"
          "      Cluster includes connected inset areas grouped based on\n"
          "      intersecting of their bounding boxes\n",
          progn.c_str(),
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


int main(int argc, char *argv[]) {
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  bool validate = false;
  bool relax = false;
  bool mercator = false;
  std::string check_area;
  std::string check_clusters;
  std::string output;
  std::string fillstr;
  std::string tolerancestr;
  std::string filelistname;
  std::string lutfilename;
  std::string cropstr;
  std::string overridesrs;
  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("validate", validate);
  options.flagOpt("relax", relax);
  options.flagOpt("mercator", mercator);
  options.opt("check_area", check_area);
  options.opt("check_clusters", check_clusters);
  options.opt("output", output);
  options.opt("fill", fillstr);
  options.opt("tolerance", tolerancestr);
  options.opt("filelist", filelistname, &khGetopt::FileExists);
  options.opt("lut", lutfilename, &khGetopt::FileExists);
  options.opt("crop", cropstr);
  options.opt("srs", overridesrs);
  if (!options.processAll(argc, argv, argn))
    usage(progname);
  if (help)
    usage(progname);

  if (!check_area.empty()) {
    // Do check area.
    if (!khHasExtension(check_area, ".khvr")) {
      usage(progname,
            "Input file for --check_area must be a virtual raster (*.khvr)");
    }

    ClusterAnalyzer analyzer;
    analyzer.CalcAssetAreas(check_area, overridesrs, mercator);
    return 0;
  }

  if (!check_clusters.empty()) {
    // Do check clusters.
    if (!khHasExtension(check_clusters, ".khvr")) {
      usage(
          progname,
          "Input file for --check_clusters must be a virtual raster (*.khvr)");
    }

    ClusterAnalyzer analyzer;
    analyzer.CalcAssetClusters(check_clusters, overridesrs, mercator);
    analyzer.PrintClusters();
    return 0;
  }

  // validate, post-process options
  if (output.empty())
    usage(progname, "No output specified");
  if (!khHasExtension(output, ".khvr"))
    usage(progname, "Output file must end in .khvr");
  std::vector<std::string> fill;
  if (fillstr.size()) {
    split(fillstr, ",", back_inserter(fill));
  }
  std::vector<std::string> cropstrs;
  khExtents<std::uint32_t> cropExtents;
  if (cropstr.size()) {
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
  if (filelist.empty()) {
    usage(progname, "No source files specified");
  }

  unsigned int numTiles = filelist.size();

  khGDALInit();

  // create our mosaic object that we're going to populate
  khVirtualRaster virtraster;
  khExtents<double> mosaicExtents;

  // place to store information about our prototype (1st tile)
  const char *prototype = filelist[0].c_str();
  unsigned int mosaicNumBands = 0;
  ColorTable colorTable;
  const char *nodataPrototype = prototype;
  if (tolerancestr.size()) {
    virtraster.fillTolerance = tolerancestr;
  }

  try {

    // process all the tiles
    notify(NFY_INFO, "%u tiles to process", numTiles);
    for (unsigned int i = 0; i < numTiles; ++i) {
      const char *filename = filelist[i].c_str();

      notify(NFY_INFO, "Processing %s ...", filename);


      // Get all the image information from khGDALDataset
      // it will validate that the SRS & geoExtents exist
      khGDALDataset dataset(filename, overridesrs);
      unsigned int numbands = dataset->GetRasterCount();
      std::string srsStr = dataset->GetProjectionRef();
      khSize<std::uint32_t> rastersize = dataset.rasterSize();
      khGeoExtents geoExtents = dataset.geoExtents();
      if (!geoExtents.noRotation()) {
        notify(NFY_FATAL, "Unsupported geo transform in %s.\n"
               "  Cannot create khvr with rotation in projection.", filename);
      }
      khSize<double> pixelsize(geoExtents.pixelWidth(),
                               geoExtents.pixelHeight());

      // update the overall extents
      mosaicExtents.grow(geoExtents.extents());

      if (i == 0) {
        // Use info from the first tile to configure the overall mosaic
        virtraster.srs       = srsStr;
        mosaicNumBands       = numbands;
        virtraster.pixelsize = pixelsize;
        notify(NFY_INFO,
               "Using as a prototype for the mosaic virtual raster: %s",
               prototype);
        notify(NFY_INFO, "Mosaic virtual raster properties:");
        notify(NFY_INFO, "number of bands: %d", mosaicNumBands);
        notify(NFY_INFO,
               "spatial reference system: %s",
               virtraster.srs.c_str());
        notify(NFY_INFO, "pixel width: %f", virtraster.pixelsize.width);
        notify(NFY_INFO, "pixel height: %f", virtraster.pixelsize.height);
      } else {
        // confirm that subsequent tiles match
        if (numbands != mosaicNumBands)
          notify(NFY_FATAL,
                 "Raster file has number of bands incompatible with mosaic:"
                 " %s (%d vs %d)",
                 filename, numbands, mosaicNumBands);
        if (srsStr != virtraster.srs)
          notify(
              NFY_FATAL,
              "Raster file has spatial reference system incompatible with mosaic:"
              " %s (%s vs %s)",
              filename, srsStr.c_str(), virtraster.srs.c_str());

        if (!CompatiblePixelSizes(pixelsize.width,
                                  virtraster.pixelsize.width,
                                  rastersize.width)) {
          if (!relax) {
            notify(NFY_FATAL,
                   "Raster file has pixel width incompatible with mosaic:"
                   " %s (%f vs %f)",
                   filename, pixelsize.width, virtraster.pixelsize.width);
          }
        }

        if (!CompatiblePixelSizes(pixelsize.height,
                                  virtraster.pixelsize.height,
                                  rastersize.height)) {
          if (!relax) {
            notify(NFY_FATAL,
                   "Raster file has pixel height incompatible with mosaic:"
                   " %s (%f vs %f)",
                   filename, pixelsize.height, virtraster.pixelsize.height);
          }
        }
      }

      // load the band data
      for (unsigned int b = 1; b <= numbands; ++b) {
        GDALRasterBand *rasterband = dataset->GetRasterBand(b);
        GDALDataType rasterType = rasterband->GetRasterDataType();
        if (rasterType == GDT_Unknown) {
          notify(NFY_FATAL, "%s (band %u): unknown band type",
                 filename, b);
        }
        int blockSizeX;
        int blockSizeY;
        rasterband->GetBlockSize(&blockSizeX, &blockSizeY);
        if ((blockSizeX <= 0) || (blockSizeY <= 0)) {
          notify(NFY_FATAL, "%s (band %u): Invalid block size: %d %d",
                 filename, b, blockSizeX, blockSizeY);
        }
        GDALColorInterp colorInterp = rasterband->GetColorInterpretation();

        // figure out what to use for the nodata value
        std::string noDataStr;
        if (fill.size() > b-1) {
          // The user gave us a value to use. Use it.
          noDataStr = fill[b-1];
        } else {
          // Check if the source tile supplies a nodata value
          int haveNoData = 0;
          double noDataVal = rasterband->GetNoDataValue(&haveNoData);
          if (haveNoData)
            noDataStr = ToString(noDataVal);
        }

        if (i == 0) {
          if (colorInterp == GCI_PaletteIndex) {
            GDALColorTable *ctbl = rasterband->GetColorTable();
            if (!ctbl) {
              notify(NFY_FATAL,
                     "%s (band %u): unable to get color table",
                     filename, b);
            }
            colorTable = *ctbl;
          }


          virtraster.outputBands.push_back
            (khVirtualRaster::OutputBand
             (GDALGetDataTypeName(rasterType), // output type
              khSize<std::uint32_t>(blockSizeX, blockSizeY),
              GDALGetColorInterpretationName(colorInterp),
              noDataStr,
              GDALGetDataTypeName(rasterType))); // input type
        } else {
          khVirtualRaster::OutputBand &band =
            virtraster.outputBands[b-1];
          if (band.inDatatype != GDALGetDataTypeName(rasterType)) {
            notify(NFY_FATAL,
                   "Incompatible band data types: %s and %s",
                   prototype, filename);
          }
          if (band.colorinterp != GDALGetColorInterpretationName(colorInterp)) {
            notify(NFY_FATAL,
                   "Incompatible band color interpretations: %s and %s",
                   prototype, filename);
          }
          if (colorInterp == GCI_PaletteIndex) {
            GDALColorTable *ctbl = rasterband->GetColorTable();
            if (!ctbl) {
              notify(NFY_FATAL,
                     "%s (band %u): unable to get color table",
                     filename, b);
            }
            if (colorTable != *ctbl) {
              notify(NFY_FATAL,
                     "Incompatible color table: %s and %s (band %d)",
                     prototype, filename, b);
            }
          }
          if (band.noDataValue != noDataStr) {
            // the no data value doesn't match the prototype
            // in certain cases this is not fatal
            // let's see if this one is
            bool fatal = true;

            // treat '0' and '' as if both were '0'
            if (band.noDataValue.empty() && (noDataStr == "0")) {
              band.noDataValue = "0";
              nodataPrototype = prototype;
              fatal = false;
            } else if ((band.noDataValue == "0") && noDataStr.empty()){
              fatal = false;
            }


            // nodata can be sloppy w/ palette images
            else if (colorInterp == GCI_PaletteIndex) {
              if (!noDataStr.empty() && band.noDataValue.empty()) {
                // our prototype doesn't have it but I do
                int noDataVal;
                FromString(noDataStr, noDataVal);
                if (noDataVal >= (int)colorTable.entries.size()) {
                  // the no data value is outside the palette
                  fatal = false;
                  band.noDataValue = noDataStr;
                  nodataPrototype = prototype;
                }
              } else if (noDataStr.empty()) {
                // I don't have nodata, but prototype does
                double protoNoData = 0.0;
                FromString(band.noDataValue, protoNoData);
                if (protoNoData >= colorTable.entries.size()) {
                  // the no data value is outside the palette
                  fatal = false;
                }
              }
            }

            if (fatal) {
              notify(NFY_FATAL,
                     "Incompatible nodata: %s (%s) and %s (%s)",
                     nodataPrototype, noDataStr.c_str(),
                     filename, band.noDataValue.c_str());
            }
          }

          // we want the largest of all block sizes
          band.outBlocksize.width  =
            std::max(band.outBlocksize.width,
                     (std::uint32_t)blockSizeX);
          band.outBlocksize.height =
            std::max(band.outBlocksize.height,
                     (std::uint32_t)blockSizeY);
          band.outBlocksize.height =
            std::max<std::uint32_t>(std::max(band.outBlocksize.height,
                                      (std::uint32_t)blockSizeY),
                             200);
        }

      }

      // create and add the Tile
      std::string absfile = khAbspath(filename);
      virtraster.inputTiles.push_back
        (khVirtualRaster::InputTile(absfile,
                                    geoExtents.geoTransformOrigin(),
                                    rastersize));
    }

    // do the summary calculations
    double dX = virtraster.pixelsize.width;
    double dY = virtraster.pixelsize.height;
    std::uint32_t sizeX = (std::uint32_t)((mosaicExtents.width()  / fabs(dX)) + 0.5);
    std::uint32_t sizeY = (std::uint32_t)((mosaicExtents.height() / fabs(dY)) + 0.5);
    notify(NFY_INFO, "overall size = %6u x %6u",
           (unsigned int)sizeX, (unsigned int)sizeY);

    // add the summary info into the mosaic object
    virtraster.origin     =
      khOffset<double>(XYOrder,
                       (dX > 0)
                       ? mosaicExtents.beginX() : mosaicExtents.endX(),
                       (dY > 0)
                       ? mosaicExtents.beginY() : mosaicExtents.endY());
    virtraster.rastersize = khSize<std::uint32_t>(sizeX, sizeY);

    if (!cropExtents.empty()) {
      khExtents<std::uint32_t> total(khOffset<std::uint32_t>(XYOrder, 0,0),
                              virtraster.rastersize);
      khExtents<std::uint32_t> intersection =
        khExtents<std::uint32_t>::Intersection(total, cropExtents);
      if ((intersection.width() != cropExtents.width()) ||
          (intersection.height() != cropExtents.height())) {
        notify(NFY_FATAL, "Invalid clip extents (xywh) %u,%u,%u,%u: avail size: (wh) %u,%u",
               cropExtents.beginX(), cropExtents.beginY(),
               cropExtents.width(), cropExtents.height(),
               virtraster.rastersize.width,
               virtraster.rastersize.height);
      }
      virtraster.cropExtents = cropExtents;
    }

    // process / validate LUT
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


  if (!validate) {
    // save it out to XML
    if (!virtraster.Save(output)) {
      notify(NFY_FATAL, "Unable to write %s", output.c_str());
    }
  }

  return 0;
}
