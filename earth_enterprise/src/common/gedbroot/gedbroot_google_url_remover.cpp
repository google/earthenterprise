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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdlib>

#include <string>

#include "common/gedbroot/dbroot_google_url_remover.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/khGetopt.h"  // NOLINT(*)
#include "common/notify.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

using std::string;


static
void usage(const char* progname, const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "Usage: %s [--output=outname] [--debug] <indbroot> // by default"
          " produces <indbroot>.cleaned\n"
          "  [--output=outname]\n"
          "    Eg %s --output=<outname> <indbroot> // writes cleaned <indbroot>"
          " to <outname>\n"
          "  [--debug] -- writes before and after text dbroot files,"
          " for verification.\n"
          "    Eg %s --debug <indbroot> // will produce\n"
          "        <indbroot>.before.txt, <indbroot>.cleaned, and"
          " <indbroot>.cleaned.txt\n"
          "\n"
          "  Fields that will be affected:\n"
          "    end_snippet.disable_authentication <- true\n"
          "  These below to \"\" if no they have no user-set data:\n"
          "    end_snippet.client_options.js_bridge_request_whitelist\n"
          "    end_snippet.autopia_options.metadata_server_url\n"
          "    end_snippet.autopia_options.depthmap_server_url\n"
          "    end_snippet.search_info.default_url\n"
          "    end_snippet.elevation_service_base_url\n"
          ,
          progname,
          progname,
          progname);
}

int main(int argc, char* argv[]) {
  std::string progname = argv[0];

  khGetopt options;
  bool help = false;
  bool want_debug = false;
  bool show_fields = false;
  (void)show_fields;

  string cleaned_output_fname;
  options.flagOpt("help", help);
  options.flagOpt("debug", want_debug);
  options.opt("output", cleaned_output_fname);

  int argn;
  // argn is the index of unprocesed arguments, of all sorts. Ie, typically
  // the start of a files list.
  if (!options.processAll(argc, argv, argn) || help || argc <= argn) {
    usage(progname.c_str());
    ::exit(1);
  }

  // The one filename
  string fname_dbroot(GetNextArg());

  if (cleaned_output_fname.empty()) {
    cleaned_output_fname = fname_dbroot + ".cleaned";
  }
  bool dbroot_is_valid = false;
  try {
    geProtoDbroot dbroot(fname_dbroot, geProtoDbroot::kEncodedFormat);

    dbroot_is_valid = dbroot.IsInitialized();
    if (!dbroot_is_valid) {
      notify(NFY_FATAL,
            "Given proto dbroot is invalid; refusing to touch it further");
      // Diagnose
      dbroot.CheckInitialized();
    } else {
      if (want_debug) {
        dbroot.Write(fname_dbroot + ".before.txt", geProtoDbroot::kTextFormat);
      }

      dbroot_is_valid = RemoveUnwantedGoogleUrls(&dbroot);
      dbroot.Write(cleaned_output_fname, geProtoDbroot::kEncodedFormat);

      if (want_debug) {
        dbroot.Write(cleaned_output_fname + ".txt", geProtoDbroot::kTextFormat);
      }
    }
  }
  catch (khSimpleException e) {
    notify(NFY_WARN, "%s", e.what());
  }

  google::protobuf::ShutdownProtobufLibrary();

  ::exit(dbroot_is_valid ? 0 : 1);
}
