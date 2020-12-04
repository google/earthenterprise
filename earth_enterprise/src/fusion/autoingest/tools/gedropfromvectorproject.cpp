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
#include <khFileUtils.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>


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

  fprintf(stderr,
          "\nusage: %s [options] -o <projectname> {--layer <layername> | <vectorresource>}...\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "   You can specify the layer to drop by name or by vectorresource\n"
          "   NOTE: Layer names may contain layer group names separated by the\n"
          "   '|' character (e.g. --layer 'folder1|folder2|layerA'). Don't\n"
          "   forget the quotes to protect the '|' from the shell.\n",
          progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];


    // process commandline options
    int argn;
    bool help = false;
    bool debug = false;
    std::vector<std::string> layernames;
    VectorProjectDropFromRequest req;
    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", req.assetname);
    options.vecOpt("layer", layernames);
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // simple option validation
    if (req.assetname.empty()) {
      usage(progname, "<projectname> not specified");
    }
    req.assetname = AssetDefs::NormalizeAssetName(req.assetname,
                                                  AssetDefs::Vector,
                                                  kProjectSubtype);

    // get vector assets from cmdline args
    for (; argn < argc; ++argn) {
      req.items.push_back
        (VectorProjectDropFromRequest::Item
         (QString(),                              // layerName
          AssetDefs::NormalizeAssetName(argv[argn],
                                        AssetDefs::Vector,
                                        kProductSubtype))); // assetRef
    }

    for (const auto& ln : layernames) {
      req.items.push_back
        (VectorProjectDropFromRequest::Item
         (ln.c_str(),                // layerName
          std::string()));    // assetRef
    }


    if (req.items.empty()) {
      usage(progname, "No layers or vector resources specified");
    }

    // now send the request
    if (debug) {
      std::string reqstr;
      req.SaveToString(reqstr, "");
      printf("%s\n", reqstr.c_str());
    } else {
      QString error;
      if (!khAssetManagerProxy::VectorProjectDropFrom(req, error)) {
        notify(NFY_FATAL, "%s", error.latin1());
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
