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


/******************************************************************************
File:        coverage.cpp
Description:

-------------------------------------------------------------------------------
For history see CVS log (cvs log coverage.cpp -or- Emacs Ctrl-xvl).
******************************************************************************/
#include <stdio.h>
#include <khGetopt.h>
#include <khTileAddr.h>
#include <khInsetCoverage.h>
#include <khMisc.h>



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
          "\nusage: %s [options] <packdir>... \n"
          "   Calculate coverages.\n"
          "   Supported options are:\n"
          "      --help | -?:           Display this usage message\n"
          "      --maxLevel <num>:      Highest res PIXEL level (REQUIRED)\n"
          "      --pixelCovSize <num>:  Full res size in each axis (REQUIRED)\n"
          "      --lat      <float>:    Latitude as decimal degrees [default 0.0]\n"
          "      --lon      <float>:    Longitude as decimal degrees [default 0.0]\n"
          "      --minLevel <num>:      Lowest res PIXEL level [default log2(tileSize)]\n"
          "      --stepOut  <num>:      Num tiles to step out in each direction\n"
          "                             when zooming out. [default 0]\n"
          "      --tileSize <num>:      Tile size in pixels. Must be a power of 2.\n"
          "                             [default 256]\n"
          "",
          progn.c_str());
  exit(1);
}


int main(int argc, char **argv)
{
  try {
    std::string progname = argv[0];

    // ********** process commandline options **********
    int argn;
    khGetopt options;
    bool help = false;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    unsigned int maxLevel = 0;
    options.opt("maxLevel", maxLevel);
    unsigned int pixelCovSize = 0;
    options.opt("pixelCovSize", pixelCovSize);
    double lat = 0.0;
    options.opt("lat", lat);
    double lon = 0.0;
    options.opt("lon", lon);
    unsigned int minLevel = 0;
    options.opt("minLevel", minLevel);
    unsigned int stepOut = 0;
    options.opt("stepOut", stepOut);
    unsigned int tileSize = 256;
    options.opt("tileSize", tileSize);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (argn > argc)
      usage(progname);

    if (maxLevel == 0) {
      usage(progname, "You must specify a maxLevel");
    }
    if (pixelCovSize == 0) {
      usage(progname, "You must specify a pixelCovSize");
    }
    if (maxLevel < minLevel) {
      usage(progname, "maxLevel < minLevel");
    }
    if (maxLevel >= 32) {
      usage(progname, "maxLevel must be < 32");
    }
    if (minLevel >= 32) {
      usage(progname, "minLevel must be < 32");
    }
    unsigned int tileSizeLog2 = log2(tileSize);
    if (tileSizeLog2 == 0) {
      usage(progname, "tileSize must be a power of 2");
    }

    // Make our tilespace
    // For now it's just a copy
    khTilespaceFlat tilespace(tileSizeLog2,   // tileSize
                              tileSizeLog2,   // pixelsAtlevel0
                              StartUpperLeft, // unused here, but must specify
                              false);         // isVector

    // adjust levels from PIXEL space to tilespace
    if (minLevel != 0) {
      // translate incoming PIXEL level to my tilespace level
      if (minLevel < tileSizeLog2) {
        usage(progname, "A tilesize of %u requires PIXEL levels >= %u",
              tilespace.tileSize, tileSizeLog2);
      } else {
        minLevel = minLevel - tileSizeLog2;
      }
    }
    // translate incoming PIXEL level to my tilespace level
    if (maxLevel < tileSizeLog2) {
      usage(progname, "A tilesize of %u requires PIXEL levels >= %u",
            tilespace.tileSize, tileSizeLog2);
    } else {
      maxLevel = maxLevel - tileSizeLog2;
    }



    // figure out the target coverage
    double degCovSize = tilespace.DegPixelSize(maxLevel) * pixelCovSize;
    khExtents<double> degExtents(NSEWOrder,
                                 lat + degCovSize/2,
                                 lat - degCovSize/2,
                                 lon + degCovSize/2,
                                 lon - degCovSize/2);
    khInsetCoverage coverage(tilespace,
                             degExtents, maxLevel,
                             minLevel, maxLevel+1,
                             stepOut,
                             0 /* padding size */);

    std::uint32_t totalTiles = 0;
    printf("Lev: #tiles: tile extents (rcwh): deg extents (nsew)\n");
    for (unsigned int lev = coverage.beginLevel();
         lev < coverage.endLevel(); ++lev) {
      const khExtents<std::uint32_t>& levExtents(coverage.levelExtents(lev));
      khExtents<double> degExtents(coverage.levelCoverage(lev)
                                   .degExtents(tilespace));

      std::uint32_t numTiles = levExtents.numRows() * levExtents.numCols();
      totalTiles += numTiles;
      printf("%u:%u:%u,%u,%u,%u:%.12f,%.12f,%.12f,%.12f\n",
             lev, numTiles,
             levExtents.beginRow(), levExtents.beginCol(),
             levExtents.numRows(), levExtents.numCols(),
             degExtents.north(), degExtents.south(),
             degExtents.east(), degExtents.west());
    }
    printf("Total Tiles: %u\n", totalTiles);
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}
