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
#include "common/khsimple_strconv.h"
#include "common/notify.h"
#include "fusion/portableglobe/partialglobebuilder.h"

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
          "E.g. gepartialglobebuilder --source=http://myserver --max_level=18 "
          "--default_level=8 --hires_qt_nodes_file=qt_nodes.txt \\\n"
          "--globe_directory /tmp/my_portable_globe \\\n"
          "\nusage: %s \\\n"
          "           --source=<http_source_string> \\\n"
          "           --default_level=<directory_string> \\\n"
          "           --max_level=<max_level_int> \\\n"
          "           --default_level=<default_level_int> \\\n"
          "           --partial_file=3 \\\n"
          "           --partial_start=310212123 \\\n"
          "           --partial_end=3102122 \\\n"
          "           --hires_qt_nodes_file=<hires_qt_nodes_file_string>\n"
          "\n"
          " Creates a set of packet bundles and other associated files that\n"
          " can serve as a portable globe.\n"
          "\n"
          " Options:\n"
          "   --source:        The source globe from which we are deriving\n"
          "                    a sub-globe.\n"
          "   --globe_directory: Directory where portable globe should be\n"
          "                    built.\n"
          "   --max_level:     Level of resolution of quadtree above which no\n"
          "                    packets should be saved.\n"
          "                    Default is 24.\n"
          "   --min_level:     Level of resolution of quadtree below which\n"
          "                    no data (non-quadtree) packets are kept\n"
          "                    independent of the ROI and default level.\n"
          "                    Default is 0.\n"
          "   --default_level: Level of resolution of quadtree for which all\n"
          "                    packets are kept independent of the ROI.\n"
          "                    Trumps max_level.\n"
          "                    Default is 7.\n"
          "   --hires_qt_nodes_file: Name of file containing quadtree nodes\n"
          "                    that define the high-resolution area of the\n"
          "                    globe.\n"
          "                    Default is no file.\n"
          "   --dbroot_file:   Name of file containing dbroot that should be\n"
          "                    saved with the globe.\n"
          "                    Default is no file; dbroot is read from\n"
          "                    source.\n"
          "   --show_quadtree: Show quadtree at given quadtree path.\n"
          "   --quadtree_path: Path to generated quadtree relative to the\n"
          "                    globe directory.\n"
          "   --build_quadtree:  Build quadtree, removing nodes depending on\n"
          "                    min level, max level, default level, and\n"
          "                    hi-res qt nodes (cut ROI).\n"
          "   --build_mercator_quadtree:  Build mercator quadtree indicating\n"
          "                    where data is likely to be found.\n"
          "                    Requires that the flat quadtree first be\n"
          "                    built.\n"
          "   --build_vector_quadtree:  Build quadtree indicating where data\n"
          "                    could possibly be located based on\n"
          "                    constraints.\n"
          "   --partial_file:  Integer index of file.\n"
          "                    Default is 0.\n"
          "   --partial_start: First quadtree address of data to save.\n"
          "                    E.g. 310212123 (no lead 0).\n"
          "                    Default is empty.\n"
          "   --partial_end:   Last quadtree address of data to save.\n"
          "                    E.g. 310212123 (no lead 0).\n"
          "                    Default is 333333333333333333333333.\n"
          "   --no_write:      Do not write packets, just give print out\n"
          "                    the total size of the globe.\n"
          "   --use_post:      Use HTTP POST (e.g. for Earth Builder) to get\n"
          "                    packets from server.\n"
          "   --data_type:     Type of data packet to cut (ALL, IMAGERY,\n"
          "                    TERRAIN, or VECTOR).\n"
          "                    Default is ALL.\n"
          "   --additional_args: Arguments to be added to each request to\n"
          "                    the Earth Server being cut.\n"
          "                    Default is \"&ct=c\" indicating Cutter\n"
          "                    context.\n"
          "   --min_imagery_version: Imagery packets must be of this version\n"
          "                    or higher to be saved.\n"
          "   --imagery_packets_per_mark: Number of imagery packets reached\n"
          "                    before next quadtree address (qtmark) is\n"
          "                    written.\n"
          "                    Default is 0, meaning every packet address is\n"
          "                    written.\n"
          "   --save_qtaddresses:  Save only qt addresses spaced by\n"
          "                    imagery_packets_per_mark for imagery packets\n"
          "                    in the partial globe.\n"
          "   --qtaddress_file:  File where qt addresses should be saved.\n"
          "   --create_delta:  Only save packets that are missing or\n"
          "                    different from those in the base glb.\n"
          "   --base_glb:      Base glb for calculating the delta.",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    bool no_write = false;
    bool use_post = false;
    bool build_quadtree = false;
    bool build_mercator_quadtree = false;
    bool build_vector_quadtree = false;
    bool show_quadtree = false;
    bool create_delta = false;
    bool save_qtaddresses = false;
    std::string source;
    std::string hires_qt_nodes_file;
    std::string dbroot_file;
    std::string globe_directory;
    std::string base_glb;
    std::string quadtree_path;
    std::string qtaddresses_file;
    std::string qtpacket_version = "1";
    // Default indicates Cutter context.
    std::string additional_args = "&ct=c";
    std::uint16_t default_level = 7;
    std::uint16_t max_level = 24;
    std::uint16_t min_level = 0;
    std::uint16_t min_imagery_version = 0;
    std::uint32_t imagery_packets_per_mark = 0;
    std::uint32_t file_index = 0;
    std::string data_type = kCutAllDataFlag;
    std::string partial_start = "";
    // Note that this is a dead area of the map so it is ok that
    // it is not inclusive.
    std::string partial_end = "333333333333333333333333";

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("no_write", no_write);
    options.flagOpt("use_post", use_post);
    options.flagOpt("build_quadtree", build_quadtree);
    options.flagOpt("build_mercator_quadtree", build_mercator_quadtree);
    options.flagOpt("build_vector_quadtree", build_vector_quadtree);
    options.flagOpt("show_quadtree", show_quadtree);
    options.flagOpt("create_delta", create_delta);
    options.flagOpt("save_qtaddresses", save_qtaddresses);

    options.flagOpt("?", help);
    options.opt("quadtree_path", quadtree_path);
    options.opt("source", source);
    options.opt("globe_directory", globe_directory);
    options.opt("base_glb", base_glb);
    options.opt("qtaddresses_file", qtaddresses_file);
    options.opt("default_level", default_level);
    options.opt("min_level", min_level);
    options.opt("max_level", max_level);
    options.opt("qtpacket_version", qtpacket_version);
    options.opt("min_imagery_version", min_imagery_version);
    options.opt("hires_qt_nodes_file", hires_qt_nodes_file);
    options.opt("data_type", data_type);
    options.opt("additional_args", additional_args);
    // TODO: dbroot should no longer be needed now that it isn't
    // TODO: necessary for decrypt.
    options.opt("dbroot_file", dbroot_file);
    options.opt("file_index", file_index);
    options.opt("partial_start", partial_start);
    options.opt("partial_end", partial_end);
    options.opt("imagery_packets_per_mark", imagery_packets_per_mark);
    options.setRequired("source", "globe_directory");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
    }

    // Make sure data_type is legal.
    data_type = UpperCaseString(data_type);
    if ((data_type != kCutAllDataFlag) &&
        (data_type != kCutImageryDataFlag) &&
        (data_type != kCutTerrainDataFlag) &&
        (data_type != kCutVectorDataFlag)) {
      usage(progname, "** ERROR ** Bad data_type flag.");
    }

    fusion_portableglobe::PortableGlobeBuilder globe_builder(
        source,
        default_level,
        min_level,
        max_level,
        hires_qt_nodes_file,
        dbroot_file,
        globe_directory,
        additional_args,
        qtpacket_version,
        min_imagery_version,
        no_write,
        use_post,
        data_type,
        imagery_packets_per_mark);

    if (show_quadtree) {
      globe_builder.ShowQuadtree(quadtree_path);
    } else if (build_quadtree) {
      globe_builder.BuildQuadtree();
      std::cout << "Quadtree built." << std::endl;
    } else if (build_vector_quadtree) {
      globe_builder.BuildVectorQuadtree();
      std::cout << "Vector quadtree built." << std::endl;
    } else if (build_mercator_quadtree) {
      globe_builder.BuildMercatorQuadtree();
      std::cout << "Mercator quadtree built." << std::endl;
    } else if (save_qtaddresses) {
      globe_builder.OutputPartialGlobeQtAddresses(
          qtaddresses_file, partial_start, partial_end);
      std::cout << "Addresses saved to " << qtaddresses_file << std::endl;
      printf("%lu image packets\n", globe_builder.num_image_packets);
    } else {
      globe_builder.BuildPartialGlobe(file_index,
                                      partial_start,
                                      partial_end,
                                      create_delta,
                                      base_glb);
      std::cout << "Partial globe built." << std::endl;
    }
    printf("%lu %lu %lu\n",
           globe_builder.num_image_packets,
           globe_builder.num_terrain_packets,
           globe_builder.num_vector_packets);
    printf("%lu %lu %lu\n",
           globe_builder.image_size,
           globe_builder.terrain_size,
           globe_builder.vector_size);
    if (build_quadtree) {
      printf("0 max version: %u\n", globe_builder.max_image_version);
      printf("1 max version: %u\n", globe_builder.max_terrain_version);
      std::map<std::uint16_t, std::uint16_t>::iterator iter;
      for (iter = globe_builder.max_vector_version.begin();
           iter != globe_builder.max_vector_version.end();
           iter++) {
        printf("%u max version: %u\n", iter->first, iter->second);
      }
    }
    printf("%lu\n", globe_builder.total_size);
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
