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

#include <stdio.h>
#include <notify.h>
#include <khConstants.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <khTileAddr.h>
#include <khException.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>
#include <autoingest/plugins/RasterProjectAsset.h>

const char * NEW_TERRAIN_USAGE = "\nusage: %s [options] -o <projectname> {[--maxlevel <level>] "
            "<insetresource>}...\n"
            "   Supported options are:\n"
            "      --help | -?:      Display this usage message\n"
            "      --mercator : use mercator projection\n"
            "      --flat : use flat(Plate Carre) projection (default)\n"
            "      --terrain_overlay : make this terrain project an overlay project\n"
            "      --start_level : the level from which to start building the\n"
            "                      terrain overlay project\n"
            "      --resource_min_level : the threshold level that separates\n"
            "                             fill terrain from overlay terrain\n"
            "      --maxleveloverride <level> : Set the max level for a resource.\n"
            "   By default, new terrain projects are NOT overlay projects"
            " unless --terrain_overlay is specified.\n";

const char * MODIFY_TERRAIN_USAGE = "\nusage: %s [options] -o <projectname> {[--maxlevel <level>] "
            "<insetresource>}...\n"
            "   Supported options are:\n"
            "      --help | -?:      Display this usage message\n"
            "      --mercator : use mercator projection\n"
            "      --flat : use flat(Plate Carre) projection (default)\n"
            "      --terrain_overlay : make this terrain project an overlay project\n"
            "      --no_terrain_overlay : make this terrain project a normal project\n"
            "      --start_level : the level from which to start building the\n"
            "                      terrain overlay project\n"
            "      --resource_min_level : the threshold level that separates\n"
            "                             fill terrain from overlay terrain\n"
            "      --maxleveloverride <level> : Set the max level for a resource.\n";

const char * NEW_IMAGERY_USAGE = "\nusage: %s [options] -o <projectname> {[--maxlevel <level>] "
            "<insetresource>}...\n"
            "   Supported options are:\n"
            "      --help | -?:      Display this usage message\n"
            "      --mercator : use mercator projection\n"
            "      --flat : use flat(Plate Carre) projection (default)\n"
            "      --historical_imagery : change this project to be a historical\n"
            "                             imagery project\n"
            "      --maxleveloverride <level> : Set the max level for a resource.\n"            
            "   By default, new projects are NOT time machine projects"
            " unless --historical_imagery is specified.\n";

const char * MODIFY_IMAGERY_USAGE = "\nusage: %s [options] -o <projectname> {[--maxlevel <level>] "
            "<insetresource>}...\n"
            "   Supported options are:\n"
            "      --help | -?:      Display this usage message\n"
            "      --mercator : use mercator projection\n"
            "      --flat : use flat(Plate Carre) projection (default)\n"
            "      --historical_imagery : change this project to be a historical\n"
            "                             imagery project\n"
            "      --no_historical_imagery : make this a normal project\n"
            "                         (i.e., not a historical imagery project)\n"
            "      --maxleveloverride <level> : Set the max level for a resource.\n";            

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

  bool is_new_project_command = progn.find("new") != std::string::npos;
  if (AssetDefs::Terrain == AssetType){
    fprintf(stderr,
            is_new_project_command ? NEW_TERRAIN_USAGE : MODIFY_TERRAIN_USAGE,
            progn.c_str());
  }
  else {
    fprintf(stderr,
            is_new_project_command ? NEW_IMAGERY_USAGE : MODIFY_IMAGERY_USAGE,
            progn.c_str());
  }
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
    
    // constants
    const unsigned int MINIMUM_LEVEL = 4;
    const unsigned int MAXIMUM_LEVEL = 24;

    // process commandline options
    bool help = false;
    bool debug = false;
    bool flat = false;
    bool mercator = false;
    bool enable_historical_imagery = false;
    bool disable_historical_imagery = false;
    bool enable_terrain_overlay = false;
    bool disable_terrain_overlay = false;
    
    unsigned int peergroup = 0;
    unsigned int overridemax_deprecated = 0;
    unsigned int overridemax = 0;
    unsigned int start_level = 0;          
    unsigned int resource_min_level = 0;  

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
    options.opt("maxlevel", overridemax_deprecated);
    options.opt("maxleveloverride", overridemax);
    options.opt("historical_imagery", enable_historical_imagery);
    options.opt("no_historical_imagery", disable_historical_imagery);
    options.opt("terrain_overlay", enable_terrain_overlay);
    options.opt("no_terrain_overlay", disable_terrain_overlay);
    options.opt("start_level", start_level, 
      &khGetopt::IsEvenNumberInRange<unsigned int, MINIMUM_LEVEL, MAXIMUM_LEVEL>); 
    options.opt("resource_min_level", resource_min_level, 
      &khGetopt::RangeValidator<unsigned int, MINIMUM_LEVEL, MAXIMUM_LEVEL>);


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
    
    if (overridemax && overridemax_deprecated) {
      throw khException("Cannot use maxlevel and maxleveloverride together, " 
                       "maxlevel is deprecated.\n");
    }

    if (overridemax) {
      if (AssetType == AssetDefs::Imagery) {
        overridemax = ImageryToProductLevel(overridemax);
      }
      else {
        overridemax = TmeshToProductLevel(overridemax);
      }
    }
    
    if (overridemax_deprecated) {
      overridemax = overridemax_deprecated;
      notify(NFY_WARN, "--maxlevel is deprecated, please use --maxleveloverride. "
		      "To move to the new command, subtract 8 from the level for imagery, and "
		      "subtract 5 from the level for terrain.");
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
    for(unsigned int i = 0; i < request_items.size(); ++i) {
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

    // Only pass along the overlay-related options if it's a terrain project
    if (AssetDefs::Terrain == AssetType){
      req.enable_terrain_overlay = enable_terrain_overlay;
      req.disable_terrain_overlay = disable_terrain_overlay;
      req.overlay_terrain_start_level = start_level;
      req.overlay_terrain_resources_min_level = resource_min_level;
    }

    if (req.items.empty()) {
      if (cmd == &khAssetManagerProxy::RasterProjectNew ||
          cmd == &khAssetManagerProxy::MercatorRasterProjectNew) {
        notify(NFY_WARN,
               "No insets specified. Project will be empty.");
      } else if (mercator ||
        !( (AssetDefs::Imagery == AssetType && (enable_historical_imagery || disable_historical_imagery)) ||
           (AssetDefs::Terrain == AssetType && 
              (enable_terrain_overlay || disable_terrain_overlay || 
              start_level != 0 || resource_min_level != 0))
        )){
        // For adding/modifying, we always need insets when:
        // - It's a mercator project
        // - OR it's a non-mercator project and the historical imagery flag is NOT being modified
        // - OR it's a terrain project and no overlay-related options are being modified
        // In those cases, show the usage text and exit.
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
