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
#include <autoingest/plugins/MapProjectAsset.h>
#include <autoingest/.idl/storage/MapProjectConfig.h>
#include <autoingest/plugins/MapLayerAsset.h>
#include <common/khStringUtils.h>


namespace {
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

  if (progn.find("dropfrom") != std::string::npos) {
    fprintf(stderr, "\nusage: %s [options] -o <projectname> layername\n",
            progn.c_str());
  } else {
    fprintf(stderr,
            "\nusage: %s [options] -o <projectname> <layername|"
            "\"layername [--legend=legend_string] [--layericon=icon_name] "
            "[--nodefault_state_on|--default_state_on]\">...\n",
            progn.c_str());
  }
  fprintf(stderr, "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n");
  exit(1);
}

// Override default locale (legend, icon, Inital State(on/off)) for a layer
// based on user options. Used to enable overriding a layer's default locale
// during its usage (in mapproject here).
void OverrideDefaultLocaleOfLayer(const std::string& layer_options,
                                  const std::vector<std::string>& tokens,
                                  MapProjectConfig::LayerItem* item) {
  Defaultable<LayerLegend>& legend = item->legend;
  MapLayerAsset layer_asset(item->assetRef);
  if (layer_asset) {  // If the layer exists then default legend value from that
    legend = layer_asset->config.legend;
  }
  // Now read the optional legend redefining params and modify legend
  khGetopt options;
  LegendLocale& legend_locale = legend.GetMutableValue().defaultLocale;
  options.opt("legend", legend_locale.name.GetMutableValue());
  std::string icon_name = kDefaultIconName;
  options.opt("layericon", icon_name);
  options.flagOpt("default_state_on",
                  legend_locale.isChecked.GetMutableValue());
  const int num_tokens = static_cast<int>(tokens.size());
  const char* char_tokens[num_tokens + 1];  // 1 extra space for NULL
  for (int i = 0; i < num_tokens; ++i) {
    char_tokens[i] = tokens[i].c_str();
  }
  char_tokens[num_tokens] = 0;
  int argn;
  const bool status = options.processAll(num_tokens,
                                         const_cast<char**>(char_tokens), argn);
  if (!status || num_tokens != argn) {
    std::string msg = std::string("\nThe layer description \"")
        + layer_options + "\" is invalid.";
    if (status) {
      msg.append("\nThe following layer options are not acceptable:\n");
      for (; argn < num_tokens; ++argn) {
        msg.append(char_tokens[argn]);
        msg.append("\n");
      }
    }
    usage(tokens[0], msg.c_str());
  }
  IconReference& icon = legend_locale.icon.GetMutableValue();
  if (icon_name != icon.href.toUtf8().constData()) {  // If icon name has been changed
    IconReference::CheckIconExistence(icon_name, &icon.type);
    icon.href = icon_name.c_str();
  }
}
}  // end namespace

int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];

    // process commandline options
    int argn;
    bool help = false;
    bool debug = false;
    std::string projectref;
    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", projectref);
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // pull the layer details off the commandline
    std::vector<MapProjectConfig::LayerItem> layer_items;
    char *arg;
    while ((arg = GetNextArg())) {
      std::string this_layer(arg);
      std::vector<std::string> tokens;
      TokenizeString(this_layer, tokens, " \f\n\r\v");
      if (tokens.size() == 0) {
        usage(progname, "<layername> cannot be an empty string");
      }
      std::string lref = AssetDefs::NormalizeAssetName(
          tokens[0], AssetDefs::Map, kLayer);
      MapProjectConfig::LayerItem item(lref);
      // more than one token => locale re-defining options are there
      if (tokens.size() > 1) {
        // First token needs to be program name (rather than layer name) for
        // correct error message
        tokens[0] = progname;
        OverrideDefaultLocaleOfLayer(this_layer, tokens, &item);
      }
      layer_items.push_back(item);
    }

    // normalize the projectref
    if (projectref.empty()) {
      usage(progname, "<projectname> not specified");
    }
    projectref = AssetDefs::NormalizeAssetName(projectref, AssetDefs::Map,
                                               kProjectSubtype);

    // load the existing project & make a copy of the config
    MapProjectConfig config;
    khMetaData meta;
    if (progname.find("new") == std::string::npos) {
      MapProjectAsset project(projectref);
      if (!project) {
        notify(NFY_FATAL, "Unable to load map project '%s'",
               projectref.c_str());
      }
      config = project->config;
      meta = project->meta;
    }

    // modify the config based on the name of the program
    if ((progname.find("new") != std::string::npos) ||
        (progname.find("modify") != std::string::npos)) {
      config.layers.clear();
      for (std::vector<MapProjectConfig::LayerItem>::const_iterator litem =
           layer_items.begin(); litem != layer_items.end(); ++litem) {
        config.AddLayer(*litem);
      }
    } else if (progname.find("addto") != std::string::npos) {
      if (layer_items.empty()) {
        usage(progname, "No layers specified. Nothing to do.");
      }
      for (std::vector<MapProjectConfig::LayerItem>::const_iterator litem =
           layer_items.begin(); litem != layer_items.end(); ++litem) {
        config.AddLayer(*litem);
      }
    } else if (progname.find("dropfrom") != std::string::npos) {
      if (layer_items.empty()) {
        usage(progname, "No layers specified. Nothing to do.");
      }
      for (std::vector<MapProjectConfig::LayerItem>::const_iterator litem =
           layer_items.begin(); litem != layer_items.end(); ++litem) {
        config.DeleteLayer(*litem);
      }
    } else {
      usage(progname, "Internal Error: Don't know which command to run.");
    }


    // now send the request
    MapProjectEditRequest req(projectref, config, meta);
    if (debug) {
      std::string reqstr;
      req.SaveToString(reqstr, "");
      printf("%s\n", reqstr.c_str());
    } else {
      QString error;
      if (!khAssetManagerProxy::MapProjectEdit(req, error)) {
        notify(NFY_WARN, "Unable to save project: %s", projectref.c_str());
        notify(NFY_FATAL, "  REASON: %s", error.latin1());
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
