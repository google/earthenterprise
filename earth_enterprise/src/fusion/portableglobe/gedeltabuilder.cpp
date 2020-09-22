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


// Supports cutting the quadtree and partial globes.
// When the quadtree is cut, it emits quadtree addresses
// (qtmarks) for some given number of imagery packets
// that are detected. These can then be used to cut
// partial globes of approximately equal size.
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include "common/khAbortedException.h"
#include "common/khGetopt.h"
#include "common/khSimpleException.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/notify.h"
#include "fusion/portableglobe/deltabuilder.h"

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  // TODO: Add novector and nosearch flags.
  // TODO: Add dry_run flag.
  fprintf(stderr,
          "\nusage: %s \\\n"
          "           --original_dir=<original_cut_glb_directory> \\\n"
          "           --latest_dir=<latest_cut_glb_directory> \\\n"
          "           --delta_dir=<dest_directory> \n"
          "\n"
          "E.g. gedeltabuilder "
          "--original_dir=/data/cutter/original_img\\\n"
          "--latest_dir=/data/cutter/latest_img\\\n"
          "--delta_dir=/data/cutter/delta_img\\\n"
          "\n"
          " Creates a packet bundle of packets that are in the corresponding\n"
          " delta packet bundle but are either not in the original or are\n"
          " different from those in the corresponding original packet bundle.\n"
          "\n"
          " After creating a delta glb from these delta packet bundles, the\n"
          " new glb can effectively be served by serving this delta glb with\n"
          " the orginal glb marked as a parent.\n"
          "\n"
          " Options:\n"
          "   --original_dir:  Directory containing the data for the original\n"
          "                    glb that is currently out in the field.\n"
          "   --latest_dir:    Directory containing the data for the latest\n"
          "                    glb that has recently been cut.\n"
          "   --delta_dir:     Directory where data for delta glb should\n"
          "                    be stored.\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string original_dir;
    std::string latest_dir;
    std::string delta_dir;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("original_dir", original_dir);
    options.opt("latest_dir", latest_dir);
    options.opt("delta_dir", delta_dir);
    options.setRequired("original_dir", "latest_dir", "delta_dir");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    fusion_portableglobe::PortableDeltaBuilder delta_builder(
        original_dir, latest_dir, delta_dir);

    delta_builder.BuildDeltaGlobe();
    notify(NFY_INFO, "Delta built.");
    // Optional
    delta_builder.DisplayDeltaQtNodes();
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
