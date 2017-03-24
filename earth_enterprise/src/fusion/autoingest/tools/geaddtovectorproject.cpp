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


bool IsNew = false;

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
          "\nusage: %s [options] -o <projectname> {[--template <filename>] <vectorresource>}...\n"
          "   Supported options are:\n"
          "      --help | -?:      Display this usage message\n"
          "   The --template option will apply to all <vectorresource>'s up until the\n"
          "next --template option.\n",
          progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];

    // figure out if this is the "newproject" variant
    IsNew = (progname.find("new") != std::string::npos);


    // process commandline options
    bool help = false;
    bool debug = false;
    std::string projectname;
    std::string templatefile;
    std::vector<VectorProjectAddItem> items;
    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", projectname);
    options.opt("template", templatefile, &khGetopt::FileExists);
    while (1) {
      const char *nextarg;
      if (!options.processOptionsOnly(argc, argv, nextarg)) {
        usage(argv[0]);
      } else if (nextarg) {
        if (templatefile.size()) {
          templatefile = khAbspath(templatefile);
          items.push_back
            (VectorProjectAddItem
             (AssetDefs::NormalizeAssetName(nextarg,
                                            AssetDefs::Vector,
                                            kProductSubtype),
              templatefile));
        } else {
          usage(progname, "No template file specified for %s",
                nextarg);
        }
      } else {
        break;
      }
    }
    if (help) {
      usage(progname);
    }

    // simple option validation
    if (projectname.empty()) {
      usage(progname, "<projectname> not specified");
    }
    projectname = AssetDefs::NormalizeAssetName(projectname,
                                                AssetDefs::Vector,
                                                kProjectSubtype);

    if (items.empty()) {
      if (IsNew) {
        notify(NFY_WARN,
               "No vector assets specified. Project will be empty.");
      } else {
        usage(progname, "No vector resources specified");
      }
    }

    // now send the request
    QString error;
    bool result = true;

    // figure out which cmd to run
    if (progname.find("new") != std::string::npos) {
      VectorProjectNewRequest req(projectname,
                                  items);
      if (debug) {
        std::string reqstr;
        req.SaveToString(reqstr, "");
        printf("%s\n", reqstr.c_str());
      } else {
        result = khAssetManagerProxy::VectorProjectNew(req, error);
      }
    } else if (progname.find("modify") != std::string::npos) {
      VectorProjectModifyRequest req(projectname, items);
      if (debug) {
        std::string reqstr;
        req.SaveToString(reqstr, "");
        printf("%s\n", reqstr.c_str());
      } else {
        result = khAssetManagerProxy::VectorProjectModify(req, error);
      }
    } else if (progname.find("addto") != std::string::npos) {
      VectorProjectModifyRequest req(projectname, items);
      if (debug) {
        std::string reqstr;
        req.SaveToString(reqstr, "");
        printf("%s\n", reqstr.c_str());
      } else {
        result = khAssetManagerProxy::VectorProjectAddTo(req, error);
      }
    } else {
      usage(argv[0], "Internal Error: Don't know which command to run.");
    }

    if (!result) {
      notify(NFY_FATAL, "%s", error.latin1());
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
