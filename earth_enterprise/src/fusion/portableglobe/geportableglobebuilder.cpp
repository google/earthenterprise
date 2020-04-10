// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
// Copyright 2020 Open GEE Contributors
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


// Prunes away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes. Saves data for the quadtrees that are
// kept in packet bundles.

#include <notify.h>
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
#include "fusion/portableglobe/portableglobebuilder.h"

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
          "E.g. geportableglobebuilder --source=http://myserver --max_level=18 "
          "--default_level=8 --hires_qt_nodes_file=qt_nodes.txt \\\n"
          "--globe_directory /tmp/my_portable_globe \\\n"
          "\nusage: %s \\\n"
          "           --source=<http_source_string> \\\n"
          "           --default_level=<directory_string> \\\n"
          "           --max_level=<max_level_int> \\\n"
          "           --default_level=<default_level_int> \\\n"
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
          "   --default_level: Level of resolution of quadtree for which all\n"
          "                    packets are kept independent of the ROI.\n"
          "                    Default is 7.\n"
          "   --hires_qt_nodes_file: Name of file containing quadtree nodes\n"
          "                    that define the high-resolution area of the\n"
          "                    globe.\n"
          "                    Default is no file.\n"
          "   --dbroot_file:   Name of file containing dbroot that should be\n"
          "                    saved with the globe.\n"
          "                    Default is no file; dbroot is read from\n"
          "                    source.\n"
          "   --no_write:      Do not write packets, just give print out\n"
          "                    the total size of the globe.\n"
          "   --use_post:      Use HTTP POST (e.g. for Earth Builder) to get\n"
          "                    packets from server.\n"
          "   --metadata_file: Name of file that will store boundary metadata.\n"
          "                    Default is no file.\n"
          "   --additional_args: Arguments to be added to each request to\n"
          "                    the Earth Server being cut.\n"
          "                    Default is \"&ct=c\" indicating Cutter\n"
          "                    context.\n",
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
    std::string source;
    std::string hires_qt_nodes_file;
    std::string dbroot_file;
    std::string globe_directory;
    std::string qtpacket_version = "1";
    std::string metadata_file;
    // Default indicates Cutter context.
    std::string additional_args = "&ct=c";
    std::uint32_t default_level = 7;
    std::uint32_t max_level = 24;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("no_write", no_write);
    options.flagOpt("use_post", use_post);
    options.flagOpt("?", help);
    options.opt("source", source);
    options.opt("globe_directory", globe_directory);
    options.opt("default_level", default_level);
    options.opt("max_level", max_level);
    options.opt("qtpacket_version", qtpacket_version);
    options.opt("hires_qt_nodes_file", hires_qt_nodes_file);
    options.opt("additional_args", additional_args);
    options.opt("dbroot_file", dbroot_file);
    options.opt("metadata_file", metadata_file);

    options.setRequired("source", "globe_directory");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    fusion_portableglobe::PortableGlobeBuilder
        globe_builder(source,
                      default_level,
                      max_level,
                      hires_qt_nodes_file,
                      dbroot_file,
                      globe_directory,
                      additional_args,
                      qtpacket_version,
                      metadata_file,
                      no_write,
                      use_post);

    globe_builder.BuildGlobe();
    printf("%lu %lu %lu\n",
           globe_builder.num_image_packets,
           globe_builder.num_terrain_packets,
           globe_builder.num_vector_packets);
    printf("%lu %lu %lu\n",
           globe_builder.image_size,
           globe_builder.terrain_size,
           globe_builder.vector_size);
    printf("%lu\n", globe_builder.total_size);
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
