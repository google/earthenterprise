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

#include <stdio.h>
#include <string>
#include "common/gedbroot/proto_dbroot.h"
#include "common/khGetopt.h"
#include "common/notify.h"


void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nUsage: %s [options] --input=<clear text or clear binary dbroot> "
          "--output=<encrypted_dbroot>\n"
          "   Encodes the given dbroot\n"
          "   Supported options are:\n"
          "      --help | -?:  Display this usage message\n",
          progn.c_str());
  exit(1);
}


int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string dbroot_filename;
    std::string encrypted_filename;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);

    options.opt("input", dbroot_filename);
    options.opt("output", encrypted_filename);

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
    }

    if (dbroot_filename.empty()) {
      usage(progname, "No <clear binary or text dbroot> specified.");
    }

    if (encrypted_filename.empty()) {
      usage(progname, "No output <encrypted_dbroot> specified.");
    }

    // Tries all 3 formats in succession.
    try {
      // Encrypted -> encrypted; a no-op.
      geProtoDbroot dbroot(dbroot_filename, geProtoDbroot::kEncodedFormat);
      dbroot.Write(encrypted_filename, geProtoDbroot::kEncodedFormat);
    } catch(...) {
      try {
        // binary clear
        geProtoDbroot dbroot(dbroot_filename, geProtoDbroot::kProtoFormat);
        dbroot.Write(encrypted_filename, geProtoDbroot::kEncodedFormat);
      } catch(...) {
        // human-readable clear
        geProtoDbroot dbroot(dbroot_filename, geProtoDbroot::kTextFormat);
        dbroot.Write(encrypted_filename, geProtoDbroot::kEncodedFormat);
      }
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}
