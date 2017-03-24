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
#include <khConstants.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>
#include <autoingest/plugins/RasterProjectAsset.h>


AssetDefs::Type AssetType = AssetDefs::Invalid;

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
          "\nusage: %s [options] -o <projectname> {[--maxlevel <level>] "
          "<insetresource>}...\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "      --mercator : use mercator projection\n"
          "      --flat : use flat(Plate Carre) projection (default)\n"
          "      --historical_imagery : change this project to be a historical\n"
          "                             imagery project\n"
          "      --no_historical_imagery : make this a normal project\n"
          "                         (i.e., not a historical imagery project)\n"
          "   By default, new projects are NOT time machine projects"
          " unless --historical_imagery is specified.\n",
          progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    // figure out if I'm doing Imagery or Terrain
    std::string progname = argv[0];
    if (progname.find("terrain") != std::string::npos)
      AssetType = AssetDefs::Terrain;
    else
      AssetType = AssetDefs::Imagery;

    // process commandline options
    bool help = false;
    bool debug = false;
    bool flat = false;
    bool mercator = false;
    bool enable_historical_imagery = false;
    bool disable_historical_imagery = false;
    uint peergroup = 0;
    uint overridemax = 0;

    RasterProjectModifyRequest req(AssetType);

    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("mercator", mercator);
    options.flagOpt("flat", flat);
    options.setExclusive("flat", "mercator");
    options.opt("output", req.assetname);
    options.opt("peergroup", peergroup);
    options.opt("maxlevel", overridemax);
    options.opt("historical_imagery", enable_historical_imagery);
    options.opt("no_historical_imagery", disable_historical_imagery);

    // While processing the command line args, we must record the request items
    // which are a variable length list of strings.
    std::vector<std::string> request_items;
    while (1) {
      const char *nextarg;
      if (!options.processOptionsOnly(argc, argv, nextarg)) {
        usage(argv[0]);
      } else if (nextarg) {
        request_items.push_back(nextarg);
      } else {
        break;
      }
    }
    if (help) {
      usage(progname);
    }

    // Default to flat unless mercator imagery is specified.
    if ((!flat && !mercator) || AssetType == AssetDefs::Terrain) {
      flat = true;
    }

    // figure out which cmd to run
    bool (*cmd)(const RasterProjectModifyRequest &, QString&, int) = 0;
    if (progname.find("new") != std::string::npos) {
      cmd = mercator ? &khAssetManagerProxy::MercatorRasterProjectNew
                     : &khAssetManagerProxy::RasterProjectNew;
    } else if (progname.find("modify") != std::string::npos) {
      cmd = mercator ? &khAssetManagerProxy::MercatorRasterProjectModify
                     : &khAssetManagerProxy::RasterProjectModify;
    } else if (progname.find("addto") != std::string::npos) {
      cmd = mercator ? &khAssetManagerProxy::MercatorRasterProjectAddTo
                     : &khAssetManagerProxy::RasterProjectAddTo;
    } else {
      usage(argv[0], "Internal Error: Don't know which command to run.");
    }
    if (enable_historical_imagery && mercator) {
      usage(argv[0], "--historical_imagery is not a valid option for mercator "
          "imagery projects.");
    }

    // Process the request items, which are a variable length list of strings.
    for(uint i = 0; i < request_items.size(); ++i) {
      req.items.push_back(
        RasterProjectModifyRequest::Item(
            AssetDefs::NormalizeAssetName(request_items[i], AssetType,
                mercator ? kMercatorProductSubtype : kProductSubtype),
            overridemax, peergroup));
      // clear these after each inset
      overridemax = 0;
      peergroup = 0;
    }

    // simple option validation
    if (req.assetname.empty()) {
      usage(progname, "<projectname> not specified");
    }
    req.assetname = AssetDefs::NormalizeAssetName(
        req.assetname, AssetType,
        (mercator? kMercatorProjectSubtype : kProjectSubtype));

    // TimeMachine only applies to non-mercator
     if (!mercator) {
      req.enable_timemachine = enable_historical_imagery;
      req.disable_timemachine = disable_historical_imagery;
    }

    if (req.items.empty()) {
      if (cmd == &khAssetManagerProxy::RasterProjectNew ||
          cmd == &khAssetManagerProxy::MercatorRasterProjectNew) {
        notify(NFY_WARN,
               "No insets specified. Project will be empty.");
      } else if (mercator ||
            !(enable_historical_imagery || disable_historical_imagery)) {
        // For adding/modifying, we always need insets except:
        // non-mercator when historical_imagery/no_historical_imagery is
        // specified.
        usage(progname, "No insets specified");
      }
    }

    // now send the request
    if (debug) {
      std::string reqstr;
      req.SaveToString(reqstr, "");
      printf("%s\n", reqstr.c_str());
    } else {
      QString error;
      if (!(*cmd)(req, error, 0 /* timeout */)) {
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
