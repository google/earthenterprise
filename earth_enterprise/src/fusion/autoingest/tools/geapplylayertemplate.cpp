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
#include <khGetopt.h>
#include <khFileUtils.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>
#include <autoingest/plugins/VectorProjectAsset.h>


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
          "\nusage: %s [options] --project <projectname> {--layer <layername> | --layerid <channelid>} --template <templatefile>\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "      --displayrules:   Apply the display rules portion of the template\n"
          "      --legend:         Apply the legend portion of the template\n"
          "   NOTE: Layer names may contain layer group names separated by the\n"
          "   '|' character (e.g. --layer 'folder1|folder2|layerA'). Don't\n"
          "   forget the quotes to protect the '|' from the shell. \n"
          "\n"
          "   NOTE: '/' is allowed in layername or can be replaced with \"&#47;\"\n"
          ,
          progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];


    // process commandline options
    int argn;
    bool debug = false;
    bool help = false;
    std::string layername;
    unsigned int layerid = 0;
    std::string project;
    std::string templatename;
    bool        applyDisplayRules = false;
    bool        applyLegend = false;

    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("template", templatename, &khGetopt::FileExists);
    options.opt("layer", layername);
    options.opt("layerid", layerid);
    options.opt("project", project);
    options.flagOpt("displayrules", applyDisplayRules);
    options.flagOpt("legend", applyLegend);
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // simple option validation
    if (!applyDisplayRules && !applyLegend) {
      usage(progname, "--displayrules and/or --legend must be specified");
    }
    if (layername.empty() && (layerid == 0)) {
      usage(progname, "--layername or --layerid must be specified");
    }
    if (!layername.empty() && (layerid != 0)) {
      usage(progname, "--layername and --layerid may not both be specified");
    }
    QString qlayername = QString::fromUtf8(layername.c_str());
    qlayername.replace("&#47;", "/");
    layername = qlayername.toUtf8().constData();
    if (project.empty()) {
      usage(progname, "--project not specified");
    }
    if (templatename.empty()) {
      usage(progname, "--template not specified");
    }
    project = AssetDefs::NormalizeAssetName(project,
                                            AssetDefs::Vector,
                                            kProjectSubtype);
    VectorProjectAsset vproj(project);
    if (!vproj) {
      notify(NFY_FATAL, "Unable to load vector project '%s'", project.c_str());
    }
    LayerConfig tmplConfig;
    if (!tmplConfig.Load(templatename)) {
      notify(NFY_FATAL, "Unable to proceed");
    }

    // make a copy we can modify
    VectorProjectConfig vconfig = vproj->config;

    // find the layer we want to modify
    for (std::vector<LayerConfig>::iterator layer = vconfig.layers.begin();
         layer != vconfig.layers.end(); ++layer) {
      if (((layerid != 0) && (layer->channelId == layerid)) ||
          (layer->DefaultNameWithPath() == layername.c_str())) {

        layer->ApplyTemplate(tmplConfig, applyDisplayRules, applyLegend);

        VectorProjectEditRequest request(project, vconfig, vproj->meta);
        if (debug) {
          std::string reqstr;
          request.SaveToString(reqstr, "");
          printf("%s\n", reqstr.c_str());
        } else {
          QString error;
          if (!khAssetManagerProxy::VectorProjectEdit(request, error)) {
            notify(NFY_WARN, "Unable to save project: %s", project.c_str());
            notify(NFY_FATAL, "  REASON: %s", error.latin1());
          }
        }
        return 0;
      }
    }
    if (layerid != 0) {
      notify(NFY_FATAL, "Unable to find layerid %u in vector project '%s'",
             layerid, project.c_str());
    } else {
      notify(NFY_FATAL, "Unable to find layer '%s' in vector project '%s'",
             layername.c_str(), project.c_str());
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
