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
#include <stdio.h>
#include <notify.h>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <khGuard.h>
#include <autoingest/.idl/Locale.h>
#include <autoingest/.idl/storage/MapLayerJSConfig.h>
#include <iconutil/iconutil.h>
#include <khConstants.h>
#include "common/khException.h"

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
          "\nusage: %s [options] --config <configfile> <outdir>\n"
          "   Generates the required layers.js files under <outdir>\n"
          "   Supported options are:\n"
          "      --help | -?:  Display this usage message\n",
          progn.c_str());
  exit(1);
}


void EmitLayersJSFile(const MapLayerJSConfig &config,
                      const std::string &outdir,
                      const QString &locale,
                      std::set<IconReference> &used_icons);


int
main(int argc, char **argv)
{
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string configfile;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("config", configfile, &khGetopt::FileExists);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);

    // Validate commandline options
    if (argn >= argc) {
      usage(progname, "No <outfile> specified");
    }
    if (!configfile.size()) {
      usage(progname, "No <configfile> specified");
    }

    std::string outdir = GetNextArgAsString();
    if (outdir.empty()) {
      usage(progname, "No <outdir> specified");
    }


    // load my config
    MapLayerJSConfig config;
    if (!config.Load(configfile)) {
      // specific message already emitted
      notify(NFY_FATAL, "Unable to load config file %s", configfile.c_str());
    }

    // Load locales
    LocaleSet localeSet;
    if (!localeSet.Load()) {
      throw khException(kh::tr("Unable to load locales"));
    }

    // Make a clean directory
    if (!khMakeCleanDir(outdir)) {
      // message already emitted
      throw khException(kh::tr("Unable to continue"));
    }

    // make a place to remember which icons we've used
    std::set<IconReference> used_icons;

    // emit the default layers.js
    EmitLayersJSFile(config, outdir, kDefaultLocaleSuffix.c_str(), used_icons);

    // emit localized layer.js files
    for (std::vector<QString>::const_iterator locale =
           localeSet.supportedLocales.begin();
         locale !=  localeSet.supportedLocales.end(); ++locale) {
      EmitLayersJSFile(config, outdir, *locale, used_icons);
    }

    // extract used icons from icon manager into icons/ directory
    std::string icondir = khComposePath(outdir,  "icons");
    if (!khMakeDir(icondir)) {
      throw khException(kh::tr("Unable to create icon directory"));
    }
    for (std::set<IconReference>::const_iterator icon =
           used_icons.begin(); icon != used_icons.end(); ++icon) {
      std::string src = icon->SourcePath();
      std::string destbase = icondir + "/" + icon->LegendHref();

      iconutil::Extract(src, iconutil::Legend, destbase);
    }


  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}



void EmitImageryLayer(FILE* fp, const LegendLocale &legend_locale,
                      const MapLayerJSConfig::Layer& layer,
                      std::set<IconReference> &used_icons)
{
  used_icons.insert(legend_locale.icon.GetValue());
  QString icon_name("icons/");
  icon_name += legend_locale.icon.GetValue().LegendHref().c_str();

  fprintf(fp, "tile_layer_defs.push(\n");
  fprintf(fp, "{\n");
  fprintf(fp, "  img: stream_url + \"/query?request=Icon&icon_path=%s\",\n",
          (const char *)icon_name.utf8());
  fprintf(fp, "  txt: \"%s\",\n",
          (const char *)legend_locale.name.GetValue().utf8());
  fprintf(fp, "  initial_state: %s,\n",
          legend_locale.isChecked.GetValue() ? "true" : "false");
  fprintf(fp, "  opacity: 1.0,\n");
  fprintf(fp, "  is_png: false,\n");
  fprintf(fp, "  fetch_func: function(addr, level) {\n");
  fprintf(fp, "    return stream_url + \"/query?request=ImageryMaps\"\n");
  fprintf(fp, "           + \"&level=\" + level\n");
  fprintf(fp, "           + \"&row=\" + addr.y\n");
  fprintf(fp, "           + \"&col=\" + addr.x\n");
  fprintf(fp, "           + \"&channel=%d\"\n", layer.channel_id_);
  fprintf(fp, "           + \"&version=%d\";\n", layer.index_version_);
  fprintf(fp, "  }\n");
  fprintf(fp, "} );\n");
}

void EmitVectorRasterLayer(FILE* fp, const LegendLocale &legend_locale,
                           const MapLayerJSConfig::Layer& layer,
                           std::set<IconReference> &used_icons)
{
  used_icons.insert(legend_locale.icon.GetValue());
  QString icon_name("icons/");
  icon_name += legend_locale.icon.GetValue().LegendHref().c_str();

  fprintf(fp, "tile_layer_defs.push(\n");
  fprintf(fp, "{\n");
  fprintf(fp, "  img: stream_url + \"/query?request=Icon&icon_path=%s\",\n",
          (const char *)icon_name.utf8());
  fprintf(fp, "  txt: \"%s\",\n",
          (const char *)legend_locale.name.GetValue().utf8());
  fprintf(fp, "  initial_state: %s,\n",
          legend_locale.isChecked.GetValue() ? "true" : "false");
  fprintf(fp, "  opacity: 1.0,\n");
  fprintf(fp, "  is_png: true,\n");
  fprintf(fp, "  fetch_func: function(addr, level) {\n");
  fprintf(fp, "    return stream_url + \"/query?request=VectorMapsRaster\"\n");
  fprintf(fp, "           + \"&level=\" + level\n");
  fprintf(fp, "           + \"&row=\" + addr.y\n");
  fprintf(fp, "           + \"&col=\" + addr.x\n");
  fprintf(fp, "           + \"&channel=%d\"\n", layer.channel_id_);
  fprintf(fp, "           + \"&version=%d\";\n", layer.index_version_);
  fprintf(fp, "  }\n");
  fprintf(fp, "} );\n");
}


void EmitLayersJSFile(const MapLayerJSConfig &config,
                      const std::string &outdir,
                      const QString &locale,
                      std::set<IconReference> &used_icons)
{
  // open the file for writing
  std::string outfile =
    khComposePath(outdir, kLayerDefsPrefix + "." + locale.latin1());
  FILE* fp = ::fopen(outfile.c_str(), "w");
  if (!fp) {
    throw khErrnoException(kh::tr("Unable to open %1 for writing")
                           .arg(outfile.c_str()));
  }
  khFILECloser closer(fp);

  typedef std::vector<MapLayerJSConfig::Layer> LayerVec;
  typedef LayerVec::const_iterator LayerIterator;

  for (LayerIterator layer = config.layers_.begin();
       layer != config.layers_.end(); ++layer) {
    LegendLocale legend_locale = (locale == kDefaultLocaleSuffix.c_str())
                                 ? layer->legend_.defaultLocale
                                 : layer->legend_.GetLegendLocale(locale);
    switch (layer->type_) {
      case MapLayerJSConfig::Layer::Imagery:
        EmitImageryLayer(fp, legend_locale, *layer, used_icons);
        break;
      case MapLayerJSConfig::Layer::Vector:
        throw khException(kh::tr("Vector layers not yet implemented"));
      case MapLayerJSConfig::Layer::VectorRaster:
        EmitVectorRasterLayer(fp, legend_locale, *layer, used_icons);
        break;
      case MapLayerJSConfig::Layer::UndefinedType:
        throw khException(kh::tr("Undefined layer type"));
      default:
        throw khException(kh::tr("Invalid layer type"));
    }
  }
}
