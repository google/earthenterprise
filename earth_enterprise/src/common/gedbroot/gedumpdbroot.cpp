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

//

#include <stdio.h>
#include <qstring.h>

#include <iostream>
#include <exception>
#include "common/notify.h"
#include "common/khGetopt.h"
#include "common/khFileUtils.h"
#include "common/khSimpleException.h"
#include "common/gedbroot/eta_dbroot.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/eta2proto_dbroot.h"
#include "common/khException.h"

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <binary_or_ascii_dbroot>\n"
          "   Decodes and displays the contents of the given dbroot\n"
          "   Supported options are:\n"
          "      --epoch:  Specify the epoch (database version).\n"
          "      --help | -?:  Display this usage message\n",
          progn.c_str());
  exit(1);
}





int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options.
    int argn;
    bool help = false;

    // "epoch" (database version) is initialized as value "1".
    // If input is a binary ETA dbroot file or a proto dbroot file, this value
    // will be updated from the file; if input is an ascii ETA dbroot file,
    // user will have the option to specify a different value.
    int epoch = 1;

    khGetopt options;
    options.opt("epoch", epoch);
    options.flagOpt("help", help);
    options.flagOpt("?", help);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);

    // Validate commandline options.
    std::string dbroot_filename;
    if (argn < argc) {
      dbroot_filename = GetNextArgAsString();
    }
    if (dbroot_filename.empty()) {
      usage(progname, "No <binary_dbroot> specified");
    }

    // for ETA dbroot
    geProtoDbroot dbroot;
    gedbroot::geEta2ProtoDbroot eta2proto(&dbroot);
    if (eta2proto.ConvertFile(dbroot_filename, epoch)) {
      // Check whether proto dbroot is valid.
      const bool EXPECT_EPOCH = true;
      if (!dbroot.IsValid(EXPECT_EPOCH)) {
        return 0;
      }
      // Print ETA and transformed proto-dbroot contents.
      printf("Format: ETA\n");
      printf("Epoch (database version): %u\n",
             dbroot.database_version().quadtree_version());
      printf("-- begin eta dbroot contents --\n%s",
             eta2proto.EtaContent().c_str());
      printf("-- end eta dbroot contents --\n\n");
      printf("-- begin proto dbroot contents --\n%s",
             dbroot.ToTextString().c_str());
      printf("-- end proto dbroot contents --\n\n");
      return 0;
    } else {  // for proto dbroot
      std::string output;
      try {
        geProtoDbroot dbroot(dbroot_filename, geProtoDbroot::kEncodedFormat);
        printf("Format: encoded protobuf\n");
        output = dbroot.ToTextString();
      } catch(...) {
        try {
          geProtoDbroot dbroot(dbroot_filename, geProtoDbroot::kProtoFormat);
          printf("Format: binary protobuf\n");
          output = dbroot.ToTextString();
        } catch(...) {
          geProtoDbroot dbroot(dbroot_filename, geProtoDbroot::kTextFormat);
          printf("Format: text protobuf\n");
          output = dbroot.ToTextString();
        }
      }
      printf("-- begin dbroot contents --\n%s", output.c_str());
      printf("-- end dbroot contents --\n");
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}
