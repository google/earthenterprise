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


#include <khraster/khRasterProduct.h>
#include <notify.h>
#include <khGetopt.h>
#include <khGuard.h>
#include <khTileAddr.h>
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
          "\nusage: %s [options] -o {<output.kip>|<output.ktp>} {<input.kip>|<input.ktp>}\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "      --level <level>   Level to extract from (dflt: max level)\n"
          "      --reflevel <level> Level for row/col coords\n"
          "      --br              Begin Row\n"
          "      --er              End Row - (one beyond)\n"
          "      --bc              Begin Column\n"
          "      --ec              End Column - (one beyond)\n",
          progn.c_str());
  exit(1);
}


int main(int argc, char *argv[]) {
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  unsigned int level = 0;
  unsigned int reflevel = 0;
  int br = -1;
  int er = -1;
  int bc = -1;
  int ec = -1;
  std::string output;
  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.opt("level", level);
  options.opt("reflevel", reflevel);
  options.opt("br", br);
  options.opt("er", er);
  options.opt("bc", bc);
  options.opt("ec", ec);
  options.opt("output", output);
  if (!options.processAll(argc, argv, argn))
    usage(progname);
  if (help)
    usage(progname);


  if (output.empty()) {
    usage(progname, "You must specify an output file");
  }


  // get the raster product name and try to open in
  std::string rasterfile;
  if (argn < argc) {
    rasterfile = argv[argn++];
  } else {
    usage(progname);
  }
  khDeleteGuard<khRasterProduct> inRP(khRasterProduct::Open(rasterfile));
  if (!inRP) {
    notify(NFY_FATAL, "Unable to open %s as a Keyhole Raster Product",
           rasterfile.c_str());
  }
  khDeleteGuard<khRasterProduct> deleteGuard(TransferOwnership(inRP));

  // validate the level and reflevel options
  if (level == 0) {
    level = inRP->maxLevel();
  }
  if (reflevel == 0) {
    reflevel = level;
  }
  if ((level < inRP->minLevel()) || (level > inRP->maxLevel())) {
    notify(NFY_FATAL, "Invalid level specified %d. Valid range is %d -> %d inclusive.",
           level, inRP->minLevel(), inRP->maxLevel());
  }
  if ((reflevel < inRP->minLevel()) || (reflevel > inRP->maxLevel())) {
    notify(NFY_FATAL, "Invalid reflevel specified %d. Valid range is %d -> %d inclusive.",
           reflevel, inRP->minLevel(), inRP->maxLevel());
  }

  // validate all the row/col options
  const khRasterProduct::Level &reflev(inRP->level(reflevel));
  if (br < 0) br = reflev.tileExtents().beginRow();
  if (er < 0) er = reflev.tileExtents().endRow();
  if (bc < 0) bc = reflev.tileExtents().beginCol();
  if (ec < 0) ec = reflev.tileExtents().endCol();
  khExtents<std::uint32_t> refextents(RowColOrder, br, er, bc, ec);
  if (refextents.empty() || !reflev.tileExtents().contains(refextents)) {
    notify(NFY_FATAL,
           "Bad row/col. Valid rows: %u -> %u. Valid cols: %u -> %u",
           reflev.tileExtents().beginRow(), reflev.tileExtents().endRow(),
           reflev.tileExtents().beginCol(), reflev.tileExtents().endCol());
  }
  khLevelCoverage refCov(reflevel, refextents);


  // ***** figure out what we're going to extract *****
  khLevelCoverage extractCov(refCov);
  extractCov.scaleToLevel(level).cropTo(inRP->level(level).tileExtents());
  notify(NFY_NOTICE, "Extracting --level %d -br %d -er %d -bc %d -ec %d",
         extractCov.level,
         extractCov.extents.beginY(), extractCov.extents.endY(),
         extractCov.extents.beginX(), extractCov.extents.endX());


  try {
    RasterGenerator::ExtractDataProduct(inRP, extractCov, output);
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }

  return 0;
}
