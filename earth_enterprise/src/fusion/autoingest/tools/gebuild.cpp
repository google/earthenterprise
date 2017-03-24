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
#include <notify.h>
#include <khGetopt.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>

void
usage(const char *prog, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <assetname>\n"
          "   Supported options are:\n"
          "      --help | -?:    Display this usage message\n",
          prog);
  exit(1);
}


#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)

int
main(int argc, char *argv[]) {

  try {
    // process commandline options
    int argn;
    bool help = false;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    if (!options.processAll(argc, argv, argn)) {
      usage(argv[0]);
    }
    if (help) {
      usage(argv[0]);
    }

    // process rest of commandline arguments
    const char *assetname = GetNextArg();
    if (!assetname) {
      usage(argv[0], "<assetname> not specified");
    }

    std::string assetref = AssetDefs::GuessAssetRef(assetname);

    // now send the request
    bool needed;
    QString error;
    if (!khAssetManagerProxy::BuildAsset(assetref, needed, error)) {
      notify(NFY_FATAL, "%s", error.latin1());
    } else if (!needed) {
      notify(NFY_NOTICE, "Nothing to do. Already up to date.");
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
