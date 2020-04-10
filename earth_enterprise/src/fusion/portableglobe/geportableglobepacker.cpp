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


// Packs up globe files into a single ".glb" file.

#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include "common/khAbortedException.h"
#include "common/khFileUtils.h"
#include "common/khGetopt.h"
#include "common/khSimpleException.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/notify.h"
#include "fusion/portableglobe/shared/file_packer.h"
#include "fusion/portableglobe/shared/packetbundle.h"

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "E.g. geportableglobepacker \\\n"
          "        --globe_directory /tmp/my_portable_globe \\\n"
          "        --output /tmp/my_portable_globe.glb \n"
          "\nusage: %s\n"
          "   --globe_directory: Directory where portable globe should be\n"
          "                    built.\n"
          "   --output:        File to save packed globe into.\n"
          "\n"
          " Packs up all globe files into a single file. If make_copy is set\n"
          " to True, the globe directory is undisturbed. If not, then the\n"
          " globe directory is rendered unusable, but the command can run\n"
          " considerably faster.\n"
          "\n"
          "\nOptions:\n"
          "   --make_copy:     Make a copy of all files so that the globe\n"
          "                    directory is not disturbed.\n"
          "                    Default is false.\n"
          "   --is_2d:         Are we packaging a 2d map.\n"
          "                    Default is false.\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    bool make_copy = false;
    bool is_2d = false;
    std::string source;
    std::string globe_directory;
    std::string output;
    std::string build_file;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("make_copy", make_copy);
    options.flagOpt("is_2d", is_2d);
    options.opt("globe_directory", globe_directory);
    options.opt("output", output);
    options.setRequired("output", "globe_directory");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    {
      if (khExists(output)) {
        notify(NFY_FATAL, "%s already exists.", output.c_str());
      }

      std::string data_dir = "/data";
      if (is_2d) {
        data_dir = "/mapdata";
      }

      size_t prefix_len = globe_directory.size();
      if (make_copy) {
        build_file = output;
      } else {
        build_file = globe_directory + data_dir + "/pbundle_0000";
      }

      int file_id = 0;
      char packet_index[8];
      snprintf(packet_index, sizeof(packet_index), "%04d", file_id++);
      std::string path_prefix =
          globe_directory + data_dir +
          fusion_portableglobe::PacketBundle::kDirectoryDelimiter +
          fusion_portableglobe::PacketBundle::kPacketbundlePrefix;
      fusion_portableglobe::FilePacker packer(
          build_file, path_prefix + packet_index, prefix_len);
      // Add data packet bundles.
      while (true) {
        snprintf(packet_index, sizeof(packet_index), "%04d", file_id++);
        if (!khExists(path_prefix + packet_index)) {
          break;
        } else {
          packer.AddFile(path_prefix + packet_index, prefix_len);
          std::cout << "Adding: " << path_prefix + packet_index << std::endl;
          // If we are not trying to make a copy (preserve the packet bundles),
          // then delete all of the packet bundles.
          // Note that packet bundle 0 is the eventual packed file we are
          // generating when we are not making a copy. So don't delete it!
          if (!make_copy && (file_id != 0)) {
            khPruneFileOrDir(path_prefix + packet_index);
          }
        }
      }
      packer.AddFile(globe_directory + data_dir + "/index", prefix_len);
      packer.AddAllFiles(globe_directory + "/qtp", prefix_len);
      packer.AddAllFiles(globe_directory + "/earth", prefix_len);
      packer.AddAllFiles(globe_directory + "/maps", prefix_len);
      packer.AddAllFiles(globe_directory + "/icons", prefix_len);
      packer.AddAllFiles(globe_directory + "/js", prefix_len);
      packer.AddAllFiles(globe_directory + "/kml", prefix_len);
      packer.AddAllFiles(globe_directory + "/search_db", prefix_len);
      packer.AddAllFiles(globe_directory + "/dbroot", prefix_len);
    }

  // Rename the packed file to be the desired output file.
  if (!make_copy) {
    khRename(build_file, output);
  }

  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
