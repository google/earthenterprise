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
          "\n"
          "usage: %s [options] --project <projectname> --layer <layername> -o <templatefile>\n"
          "           - or -\n"
          "       %s [options] --project <projectname> --alllayers -o <outdir>\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "   NOTE: Layer names may contain layer group names separated by the\n"
          "   '|' character (e.g. --layer 'folder1|folder2|layerA'). Don't\n"
          "   forget the quotes to protect the '|' from the shell.\n"
          "\n"
          "   NOTE: '/' is allowed in layername or can be replaced with \"&#47;\"\n"
          "\n"
          "   The --alllayers variant exports templates for all the layers as\n"
          "   separate files under <outdir>\n"
          ,
          progn.c_str(), progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];


    // process commandline options
    int argn;
    bool help = false;
    std::string layername;
    std::string project;
    std::string output;
    bool alllayers = false;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", output);
    options.opt("layer", layername);
    options.opt("project", project);
    options.flagOpt("alllayers", alllayers);
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // simple option validation
    if (layername.empty() && !alllayers) {
      usage(progname, "You must specify --layer or --alllayers");
    } else if (!layername.empty() && alllayers) {
      usage(progname, "You may not specify both --layer and --alllayers");
    }
    if (project.empty()) {
      usage(progname, "--project not specified");
    }
    if (output.empty()) {
      usage(progname, "--output not specified");
    }

    // load the project
    project = AssetDefs::NormalizeAssetName(project,
                                            AssetDefs::Vector,
                                            kProjectSubtype);
    VectorProjectAsset vproj(project);
    if (!vproj) {
      notify(NFY_FATAL, "Unable to load vector project '%s'", project.c_str());
    }


    // prepare the output
    if (!alllayers) {
      output = khEnsureExtension(output, ".khdsp");
    } else if (khExists(output)) {
      notify(NFY_FATAL, "%s already exists", output.c_str());
      if (!khMakeDir(output)) {
        notify(NFY_FATAL, "Unable to create output directory");
      }
    }

    if (!alllayers) {
      // Export a single layer template
      for (std::vector<LayerConfig>::const_iterator layer =
             vproj->config.layers.begin();
           layer != vproj->config.layers.end(); ++layer) {
        if (layer->DefaultNameWithPath() == layername.c_str()) {
          if (!layer->Save(output)) {
            // error message already emitted
            notify(NFY_FATAL, "Could not save template");
          } else {
            return 0;
          }
        }
      }
      notify(NFY_FATAL, "Unable to find layer '%s' in vector project '%s'",
             layername.c_str(), project.c_str());
    } else {
      // Export all the layer templates
      for (std::vector<LayerConfig>::const_iterator layer =
             vproj->config.layers.begin();
           layer != vproj->config.layers.end(); ++layer) {
        QString layer_name = layer->DefaultNameWithPath();
        layer_name.replace("/", "&#47;");
        std::string outfile = khComposePath(output,
                                            (const char *)layer_name.utf8()) +
                              ".khdsp";
        if (!layer->Save(outfile)) {
          // error message already emitted
          notify(NFY_FATAL, "Could not save template %s", outfile.c_str());
        }
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
