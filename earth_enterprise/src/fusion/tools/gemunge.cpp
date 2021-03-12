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


#include <string>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <khSpawn.h>
// this one is not exported in normal gdal distributions only in keyhole's
#include "vrtdataset.h"
#include <khgdal/khGDALDataset.h>
#include <cpl_string.h>
#include <gdal_alg.h>
#include "common/khException.h"

void
usage(const char *prog, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <input> <output>\n"
          "   Supported options are:\n"
          "      --help | -?:       Display this usage message\n"
          "      --split <num>:     Split image into <num> pieces (each axis)\n"
          "      --overlap <num>:   Split pieces overlap by this amount (default 0)\n"
          "      --fill a,b,...:    Fill value to use for split khvr's (default none)\n"
          "      --index <num>:     Convert to rgb palette w/ <num> entries\n"
          "      --lut <lutfile>:   Add <lutfile> to the .khvr\n"
          "      --scaleOffset:     Value to add to each pixel (default 0.0)\n"
          "      --scaleFactor:     Value to multiple each pixel by (default 1.0)\n"
          "      --type <typename>: Output datatype\n"
          "                         std::uint8_t,  std::int8_t, std::uint16_t, std::int16_t,\n"
          "                         std::uint32_t, std::int32_t, float32, float64\n"
          "      --band <bandnum>:  Band number (1 based) to include. Can be\n"
          "                         specified multiple tiles. (e.g. -b 3 -b 2 -b 1)\n"
          "                         will reverse the bands\n",
          prog);
  exit(1);
}


void ValidateTypename(const std::string &n) {
  khTypes::StorageEnum storage = khTypes::StorageNameToEnum(n.c_str());
  if ((storage == khTypes::UInt64) ||
      (storage == khTypes::Int64)) {
    throw khException(kh::tr("Unsupported type name: %1").arg(n.c_str()));
  }
}

int
main(int argc, char *argv[])
{
  try {
    khGDALInit();

    // process commandline options
    int argn;
    bool help = false;
    unsigned int split = 1;
    unsigned int index = 0;
    std::string typestr;
    double scaleOffset = 0.0;
    double scaleFactor = 1.0;
    std::vector< unsigned int>  usebands;
    unsigned int overlap = 0;
    std::string fillstr;
    std::string lutfile;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("split", split);
    options.opt("index", index);
    options.opt("scaleOffset", scaleOffset);
    options.opt("scaleFactor", scaleFactor);
    options.opt("type", typestr, ValidateTypename);
    options.vecOpt("band", usebands);
    options.opt("overlap", overlap);
    options.opt("fill", fillstr);
    options.opt("lut", lutfile);
    if (!options.processAll(argc, argv, argn)) {
      usage(argv[0]);
    }
    if (help) {
      usage(argv[0]);
    }

    // process rest of commandline arguments
    const char *input = GetNextArg();
    if (!input) {
      usage(argv[0], "<input> not specified");
    }
    const char *output = GetNextArg();
    if (!output) {
      usage(argv[0], "<output> not specified");
    }
    if (!khExists(input)) {
      usage(argv[0], "%s: no such file", input);
    }
    if (khExists(output) && !khUnlink(output)) {
      notify(NFY_FATAL, "%s: unable to remove: %s",
             output, khstrerror(errno).c_str());
    }
    if (split < 1 || split > 100) {
      usage(argv[0], "--split must be between 1 and 100 (inclusive)");
    }

    GDALDriverH outDriver = GDALGetDriverByName("GTiff");
    if (!outDriver) {
      notify(NFY_FATAL, "Unable to get GDAL GTiff driver");
    }


    // load the source dataset
    khGDALDataset srcDS(input);

    // check size & bands
    khSize<std::uint32_t> rasterSize = srcDS.rasterSize();
    unsigned int numbands = srcDS->GetRasterCount();
    if (numbands == 0) {
      notify(NFY_FATAL, "No bands");
    }
    GDALRasterBand* firstband = srcDS->GetRasterBand(1);
    GDALDataType gdalDatatype = firstband->GetRasterDataType();
    khTypes::StorageEnum srcComponentType;
    if (!StorageEnumFromGDT(gdalDatatype, srcComponentType)) {
      notify(NFY_FATAL, "Unsupported source datatype: %s",
             GDALGetDataTypeName(gdalDatatype));
    }
    khTypes::StorageEnum outType = typestr.empty()
                                   ? srcComponentType : khTypes::StorageNameToEnum(typestr.c_str());

    if (index) {
      if (index <= 256) {
        if (outType != khTypes::UInt8) {
          usage(argv[0],
                "--type doesn't match index size. Should be uint8");
        }
      } else {
        usage(argv[0], "index size too large. Must be <= 256");
      }
    }


    // get geoTransform
    double geoTransform[6];
    memcpy(geoTransform, srcDS.geoExtents().geoTransform(),
           sizeof(geoTransform));


    if (usebands.empty()) {
      for (unsigned int i = 1; i <= numbands; ++i) {
        usebands.push_back(i);
      }
    } else {
      for (unsigned int i = 0; i < usebands.size(); ++i) {
        if ((usebands[i] < 1) || (usebands[i] > numbands)) {
          notify(NFY_FATAL,
                 "Invalid band %u. Valid range is %u -> %u",
                 usebands[i], 1, numbands);
        }
      }
    }

    khDeleteGuard<GDALColorTable> rgbColorTable;
    // build a color table if we want indexed output
    if (index > 0) {
      if (usebands.size() != 3) {
        notify(NFY_FATAL,
               "--rgbindex needs 3 bands, have %u",
               (unsigned int)usebands.size());
      }
      // build rgb colortable
      rgbColorTable = TransferOwnership(new GDALColorTable(GPI_RGB));
      // stick some garbage color in the last slot, that way we
      // allocate it all up front. Otherwise it would realloc for
      // each color in the table.
      GDALColorEntry tmp;
      rgbColorTable->SetColorEntry(index-1, &tmp);

      if (GDALComputeMedianCutPCT(srcDS->GetRasterBand(usebands[0]),
                                  srcDS->GetRasterBand(usebands[1]),
                                  srcDS->GetRasterBand(usebands[2]),
                                  0,
                                  index,
                                  (GDALColorTable*)rgbColorTable,
                                  0, 0) != CE_None) {
        notify(NFY_FATAL, "Unable to build palette index");
      }
    }


    bool needKHVR = ((split > 1) || lutfile.size());

    // write output (splitting as necessary)
    std::uint32_t outWidthStep  = (std::uint32_t)ceil((double)rasterSize.width / split);
    std::uint32_t outHeightStep = (std::uint32_t)ceil((double)rasterSize.height / split);
    std::uint32_t outWidth      = outWidthStep + overlap;
    std::uint32_t outHeight     = outHeightStep + overlap;
    std::string base = khDropExtension(output);
    std::string ext  = khExtension(output);
    std::vector<std::string> files;
    for (unsigned int row = 0; row < split; ++row) {
      unsigned int yoff = row * outHeightStep;
      unsigned int ysize = std::min(outHeight, rasterSize.height - yoff);
      for (unsigned int col = 0; col < split; ++col) {
        unsigned int xoff = col * outWidthStep;
        unsigned int xsize = std::min(outWidth, rasterSize.width - xoff);

        char buf[256];
        snprintf(buf, 256, ".r%02uc%02u", row, col);
        std::string outfile;
        if (needKHVR) {
          outfile = base + buf + ext;
          files.push_back(outfile);
        } else {
          outfile = output;
        }

        khDeleteGuard<GDALDataset> vDS(TransferOwnership(
                                           new VRTDataset(xsize, ysize)));
        vDS->SetProjection(srcDS->GetProjectionRef());

        double xform[6];
        xform[0] = geoTransform[0] +
                   xoff * geoTransform[1] +
                   yoff * geoTransform[2];
        xform[3] = geoTransform[3] +
                   xoff * geoTransform[4] +
                   yoff * geoTransform[5];
        xform[1] = geoTransform[1];
        xform[2] = geoTransform[2];
        xform[4] = geoTransform[4];
        xform[5] = geoTransform[5];
        vDS->SetGeoTransform(xform);

        vDS->SetMetadata(srcDS->GetMetadata());

        for (unsigned int b = 0; b < usebands.size(); ++b) {
          GDALRasterBand *srcBand =
            srcDS->GetRasterBand(usebands[b]);
          vDS->AddBand(GDTFromStorageEnum(outType));
          VRTSourcedRasterBand *vBand =
            (VRTSourcedRasterBand*)vDS->GetRasterBand(b+1);

          if (scaleOffset != 0.0 || scaleFactor != 1.0) {
            vBand->AddComplexSource(srcBand,
                                    xoff, yoff,
                                    xsize, ysize,
                                    0, 0,
                                    xsize, ysize,
                                    scaleOffset,
                                    scaleFactor);

          } else {
            vBand->AddSimpleSource(srcBand,
                                   xoff, yoff,
                                   xsize, ysize,
                                   0, 0,
                                   xsize, ysize);
          }
          vBand->SetMetadata( srcBand->GetMetadata() );
          vBand->SetColorTable( srcBand->GetColorTable() );
          vBand->SetColorInterpretation
            (srcBand->GetColorInterpretation());
          if( strlen(srcBand->GetDescription()) > 0 )
            vBand->SetDescription( srcBand->GetDescription() );
        }

        if (!khEnsureParentDir(outfile)) {
          notify(NFY_FATAL, "Unable to make parent dir");
        }
        if (index > 0) {
          khDeleteGuard<GDALDataset>
            outDS(TransferOwnership(
                      (GDALDataset*)
                      GDALCreate(outDriver, outfile.c_str(),
                                 xsize, ysize, 1,
                                 GDTFromStorageEnum(outType),
                                 0)));
          if (!outDS) {
            notify(NFY_FATAL, "Unable to create %s",
                   outfile.c_str());
          }
          outDS->SetProjection(vDS->GetProjectionRef());
          outDS->SetGeoTransform(xform);
          outDS->SetMetadata(vDS->GetMetadata());
          GDALRasterBand *vBand = vDS->GetRasterBand(1);
          GDALRasterBand *oBand = outDS->GetRasterBand(1);
          oBand->SetMetadata( vBand->GetMetadata() );
          oBand->SetColorTable( rgbColorTable );
          // oBand->SetColorInterpretation(GCI_PaletteIndex);
          if( strlen(vBand->GetDescription()) > 0 )
            oBand->SetDescription( vBand->GetDescription() );

          assert(usebands.size() == 3);
          if (GDALDitherRGB2PCT(vDS->GetRasterBand(1),
                                vDS->GetRasterBand(2),
                                vDS->GetRasterBand(3),
                                oBand,
                                (GDALColorTable*)rgbColorTable,
                                0, 0) != CE_None) {
            notify(NFY_FATAL, "Unable to index image");
          }
        } else {
          // just use CreateCopy
          khDeleteGuard<GDALDataset>
            outDS(TransferOwnership(
                      (GDALDataset*)
                      GDALCreateCopy(outDriver, outfile.c_str(),
                                     (GDALDataset*)vDS, false,
                                     0, 0, 0)));
          if (!outDS) {
            notify(NFY_FATAL, "Unable to create outfile");
          }
        }
      } // for each col
    } // for each row

    if (needKHVR) {
      CmdLine cmd;
      cmd << "/opt/google/bin/gevirtualraster"
          << "-o" << base + ".khvr";
      if (fillstr.size()) {
        cmd << "--fill" << fillstr;
      }
      if (lutfile.size()) {
        cmd << "--lut" << lutfile;
      }
      for (std::vector<std::string>::const_iterator f = files.begin();
           f != files.end(); ++f) {
        cmd << *f;
      }
      if (!cmd.System()) {
        notify(NFY_FATAL, "Unable to make virtual raster");
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
