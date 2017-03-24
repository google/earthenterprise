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


#include <set>
#include <string>
#include <iostream>  // NOLINT(readability/streams)
#include <fstream>  // NOLINT(readability/streams)
#include <khGetopt.h>  // NOLINT(*)
#include "common/gedbroot/proto_dbroot.h"
#include "fusion/portableglobe/createmetadbroot.h"


void usage(const std::string& progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nUsage: %s \\\n"
          "    --layers=<source file name> -- defaults to './layers.txt'\\\n"
          "    --output=<dmetabroot file name> -- defaults to './dbRoot'\\\n"
          "    [--has_base_imagery] -- defaults to false \\\n"
          "    [--has_proto_imagery] -- defaults to false \\\n"
          "    [--has_terrain] -- defaults to false \\\n"
          "    [--file_format=text/proto/encoded] -- defaults to encoded\\\n"
          "    [--base_dbroot=<base dbrootfile>] -- starter dbroot"
          " in text format (mostly for dev.)\\\n"
          "\n"
          "E.g. createmetadbroot "
          "--layers=\\\n"
          "--output=\\\n"
          "\n"
          " Creates a meta dbroot\n"
          "\n"
          " Required:\n"
          "    --layers: Source file.\n"
          "    --output: Meta dbroot.\n",

          progn.c_str());

  exit(1);
}

// eg:
// gecreatemetadbroot --layers=/home/$USER/layer_info.txt
//   --output=/home/$USER/created-dbroot
//   --has_base_imagery --has_terrain

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    bool help = false;
    bool has_base_imagery = false;
    bool has_proto_imagery = false;
    bool has_terrain = false;
    std::string layers_fname = "./layer_info.txt";
    std::string output_fname = "./dbRoot";
    std::string base_dbroot_fname;
    geProtoDbroot::FileFormat file_format = geProtoDbroot::kEncodedFormat;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("has_base_imagery", has_base_imagery);
    options.flagOpt("has_proto_imagery", has_proto_imagery);
    options.flagOpt("has_terrain", has_terrain);
    options.choiceOpt(
        "file_format", file_format,
        makemap(std::string("text"), geProtoDbroot::kTextFormat,
                std::string("proto"), geProtoDbroot::kProtoFormat,
                std::string("encoded"), geProtoDbroot::kEncodedFormat));
    options.opt("layers", layers_fname);
    options.opt("output", output_fname);
    options.opt("base_dbroot", base_dbroot_fname);

    int argn;
    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
    }

    std::ifstream fin_layers;
    fin_layers.open(layers_fname.c_str());
    if (!fin_layers) {
      fprintf(stderr, "Cannot find layers file:[%s]; exiting.\n",
              layers_fname.c_str());
      exit(1);
    }

    fusion_portableglobe::MetaDbRootWriter dbroot_writer(
        has_base_imagery, has_proto_imagery, has_terrain,
        fin_layers, base_dbroot_fname);
    try {
      // kTextFormat, kProtoFormat, kEncodedFormat
      dbroot_writer.SaveTo(output_fname, file_format);
    } catch(const khSimpleException& e) {
      std::cout << "khSimpleException: " << e.what() << std::endl;
    }

    notify(NFY_INFO, "Created meta dbroot.");
  } catch(const std::exception& e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
