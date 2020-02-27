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
// This implements the missing genewmaplayer to complete the batch mode flow
// for map tile generation.
#include <stdio.h>
#include <notify.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <autoingest/.idl/storage/MapLayerConfig.h>
#include <autoingest/khAssetManagerProxy.h>
#include <vector>
#include <string>

void
usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

#define _indent_ "                             "
  fprintf(stderr,
      "\nusage: %s --legend=<legend_name> --output=<map_layer_name> [options]"
      " {[--template <filename>] <vectorresource>}...\n"
      "\n"
      "Required arguments are:\n"
      "  --legend=<legend_name>     Name of the layer as it appears in web"
      " browser.\n"
      "  --output=<map_layer_name>  Name of the map layer asset as it appears"
      "in asset\n"
      _indent_ "manager (Relative to asset_root and without\n"
      _indent_ "extension) e.g Resources/Vector/counties.\n"
      "\n"
      "Supported options are:\n"
      "  --help | -?:               Display this usage message.\n"
      "  --layericon=<icon_name>    Basename of icon png file (as it appears in"
      " the \n"
      _indent_ "icon browser). e.g cars.\n"
      "  --default_state_on         By default the layer is not checked"
      " on in the web\n"
      _indent_ "browser. By using this optional flag we set the\n"
      _indent_ "layer checked on by default in the web browser.\n"
      "  --template=<absoulte_kmdsp_file_name>\n"
      _indent_ "The --template option will apply to all\n"
      _indent_ "<vectorresource>'s up until the next --template\n"
      _indent_ "option.\n"
      "\n"
      _indent_ "The map layer consists of sublayers. Each sublayer\n"
      _indent_ "is a tuple of a vector resource and a set of\n"
      _indent_ "display rules for it. The vector resources are\n"
      _indent_ "relative to asset_root and without extension).\n"
      _indent_ "We take display rules input in kmdsp files. You can\n"
      _indent_ "create kmdsp files by entering details in Map Layer\n"
      _indent_ "widget of Fusion GUI and then exporting it. The\n"
      _indent_ "kmdsp files need to be absolute filenames.\n"
      _indent_ "e.g /home/doe/template_1.kmdsp.\n"
      _indent_ "Note: You can input more than one sublayers.\n"
      "\n"
      "example: %s --legend=cars --output=ca-car_map-layer --layericon=car"
      " --template=/home/doe/template_1.kmdsp Resources/Vector/cities"
      " Resources/Vector/villages --template=/home/doe/template_2.kmdsp"
      " Resources/Vector/counties\n"
      "\n"
      ,
      progn.c_str(), progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {
  try {
    std::string progname = argv[0];

    // figure out which cmd to run
    bool (*cmd)(const MapLayerEditRequest&, QString&, int) =
        &khAssetManagerProxy::MapLayerEdit;

    (void) cmd;

    // process commandline options
    bool help = false;
    bool debug = false;
    MapLayerEditRequest req;
    khGetopt options;
    options.opt("legend",
                req.config.legend.defaultLocale.name.GetMutableValue());
    std::string icon_name = kDefaultIconName;
    options.opt("layericon", icon_name);
    options.flagOpt("default_state_on",
        req.config.legend.defaultLocale.isChecked.GetMutableValue());
    options.opt("output", req.assetname);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("debug", debug);
    std::string template_file;
    options.opt("template", template_file, &khGetopt::FileExists);

    while (1) {
      const char *nextarg;
      if (!options.processOptionsOnly(argc, argv, nextarg)) {
        usage(progname);
      } else if (nextarg) {
        if (!template_file.empty()) {
          MapSubLayerConfig template_cfg;
          if (!template_cfg.Load(template_file)) {
            notify(NFY_FATAL,
                   "Unable to load Fusion Map Template file: \"%s\"\n.",
                   template_file.c_str());
          }
          req.config.subLayers.push_back(MapSubLayerConfig());
          req.config.subLayers.back().asset_ref = AssetDefs::NormalizeAssetName(
              nextarg, AssetDefs::Vector, kProductSubtype);
          req.config.subLayers.back().ApplyTemplate(template_cfg);
        } else {
          usage(progname, "No template file specified for %s",
                nextarg);
        }
      } else {
        break;
      }
    }

    if (help || req.config.legend.defaultLocale.name.GetValue().isEmpty() ||
        req.assetname.empty()) {
      usage(progname);
    }
    IconReference& icon =
        req.config.legend.defaultLocale.icon.GetMutableValue();
    IconReference::CheckIconExistence(icon_name, &icon.type);
    icon.href = icon_name.c_str();
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
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
