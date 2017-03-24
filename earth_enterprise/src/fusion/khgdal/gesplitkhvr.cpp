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


#include <string.h>
#include <fstream>
#include <notify.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <khgdal/khgdal.h>
#include <iomanip>
#include <iostream>

#include <khgdal/.idl/khVirtualRaster.h>

const uint DefaultOverlap = 300;
const uint MaxSplit = 100;

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

  fprintf(
stderr,
"\n"
"usage: %s [options] <input .khvr> \n"
"Split a single large .khvr into a grid of smaller khvrs.\n"
"Output filenames are written next to the input khvr with filenames like\n"
" -R#C#.khvr\n"
"Supported options are:\n"
"  --help | -?:      Display this usage message\n"
"  --rows <num>:     number of rows [required]\n"
"  --cols <num>:     number of cols [required]\n"
"  --overlap <num>:  number of pixels overlap between pieces [default %u]\n"
"  --quiet:          Don't report files/sizes at end\n",
progn.c_str(), DefaultOverlap);
  exit(1);
}


int
main(int argc, char *argv[])
{
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  bool quiet = false;
  std::string input;
  uint rows    = 0;
  uint cols    = 0;
  uint overlap = DefaultOverlap;
  khGetopt options;
  options.helpOpt(help);
  options.flagOpt("quiet", quiet);
  options.opt("rows", rows,
              &khGetopt::RangeValidator<uint, 1, MaxSplit>);
  options.opt("cols", cols,
              &khGetopt::RangeValidator<uint, 1, MaxSplit>);
  options.opt("overlap", overlap);
  options.setRequired("rows", "cols");

  if (!options.processAll(argc, argv, argn) || help)
    usage(progname);

  input = GetNextArgAsString();
  // validate, post-process options
  if (input.empty())
    usage(progname, "No .khvr specified");


  std::string splitname = khDropExtension(input);


  try {
    // load existing virtualraster
    khVirtualRaster virtraster;
    if (!virtraster.Load(input)) {
      notify(NFY_FATAL, "Unable to load %s",
             input.c_str());
    }

    uint32 nominalwidth =
      (uint32)ceil((double)virtraster.cropExtents.width() / cols);
    uint32 nominalheight =
      (uint32)ceil((double)virtraster.cropExtents.height() / rows);
    if ((overlap >= nominalwidth) || (overlap >= nominalheight)) {
      notify(NFY_FATAL, "Overlap too big for resulting split size");
    }


    uint numgroups = 0;

    // compute pixel extents of khvr tiles
    std::vector<khExtents<uint32> > tileExtents;
    tileExtents.reserve(virtraster.inputTiles.size());
    for (std::vector<khVirtualRaster::InputTile>::const_iterator tileDef =
           virtraster.inputTiles.begin();
         tileDef < virtraster.inputTiles.end(); ++tileDef) {

      khOffset<uint32>
        tileOrigin(XYOrder,
                   uint32(((tileDef->origin.x() -
                            virtraster.origin.x()) /
                           virtraster.pixelsize.width) + 0.5),
                   uint32(((tileDef->origin.y() -
                            virtraster.origin.y()) /
                           virtraster.pixelsize.height) + 0.5));
      tileExtents.push_back(khExtents<uint32>(tileOrigin,
                                              tileDef->rastersize));
    }

    // process each of the split pieces
    for (uint32 r = 0; r < rows; ++r) {
      uint32 y, height;
      if (r == 0) {
        y = virtraster.cropExtents.beginY();
        // "+ overlap - (overlap / 2)" to deal with odd overlap
        height = nominalheight + overlap - (overlap / 2);
      } else {
        y = virtraster.cropExtents.beginY() +
            (r * nominalheight) - (overlap / 2);
        height = nominalheight + overlap;
      }
      if (y >= virtraster.cropExtents.endY()) {
        // in practice this shouldn't ever happen
        // put a really small image (height < rows) could trigger it
        continue;
      }
      if (y + height > virtraster.cropExtents.endY()) {
        height = virtraster.cropExtents.endY() - y;
      }

      for (uint32 c = 0; c < cols; ++c) {
        uint32 x, width;
        if (c == 0) {
          x = virtraster.cropExtents.beginX();
          // "+ overlap - (overlap / 2)" to deal with odd overlap
          width = nominalwidth + overlap - (overlap / 2);
        } else {
          x = virtraster.cropExtents.beginX() +
              (c * nominalwidth) - (overlap / 2);
          width = nominalwidth + overlap;
        }
        if (x >= virtraster.cropExtents.endX()) {
          // in practice this shouldn't ever happen
          // put a really small image (width < cols) could trigger it
          continue;
        }
        if (x + width > virtraster.cropExtents.endX()) {
          width = virtraster.cropExtents.endX() - x;
        }

        khExtents<uint32> piecePixelExtents
          (khOffset<uint32>(XYOrder, x, y),
           khSize<uint32>(width, height));


        // see if any tiles intersect with this piece
        std::vector<uint> found;
        found.reserve(tileExtents.size());
        khExtents<uint32> coveredExtents;
        for (uint i = 0; i < tileExtents.size(); ++i) {
          if (tileExtents[i].intersects(piecePixelExtents)) {
            coveredExtents.grow(tileExtents[i]);
            found.push_back(i);
          }
        }

        // make a new khvr
        if (found.size()) {
          khExtents<uint32> saveExtents =
            khExtents<uint32>::Intersection
            (coveredExtents, piecePixelExtents);
          khVirtualRaster copy = virtraster;
          copy.cropExtents = saveExtents;
          copy.inputTiles.clear();
          copy.inputTiles.reserve(found.size());
          for (std::vector<uint>::const_iterator i = found.begin();
               i != found.end(); ++i) {
            copy.inputTiles.push_back(virtraster.inputTiles[*i]);
          }


          ++numgroups;
          std::ostringstream fname;
          fname << splitname
                << "-R" << r
                << "C"  << c
                << ".khvr";

          if (!copy.Save(fname.str())) {
            notify(NFY_FATAL, "Unable to save %s",
                   fname.str().c_str());
          }

          if (!quiet) {
            double size = ((double)saveExtents.width() *
                           (double)saveExtents.height() * 3) /
                          (1024 * 1024 * 1024);

            std::cout << std::setw(6) << std::fixed
                      << std::setprecision(2)
                      << size << "G  "
                      << fname.str() << std::endl;
          }
        }
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }


  return 0;
}
