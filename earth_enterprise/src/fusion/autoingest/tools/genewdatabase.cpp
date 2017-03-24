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
          "\nusage: %s [options] [--meta <key>=<value>]... -o <dbname> [--imagery <imagery project>] [--vector <vector project>] [--terrain <terrain project>]\n"
          "   Supported options are:\n"
          "      --help | -?:               Display this usage message\n",
          progn.c_str());
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];

    // figure out which cmd to run
    bool (*cmd)(const DatabaseEditRequest &, QString&, int) = 0;
    if (progname.find("new") != std::string::npos) {
      cmd = &khAssetManagerProxy::DatabaseNew;
    } else if (progname.find("modify") != std::string::npos)
      cmd = &khAssetManagerProxy::DatabaseModify;
    else {
      usage(argv[0], "Internal Error: Don't know which command to run.");
    }



    // process commandline options
    int argn;
    bool help = false;
    bool debug = false;
    DatabaseEditRequest req;
    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", req.assetname);
    options.mapOpt("meta", req.meta.map);
    options.opt("imagery", req.config.imageryProject);
    options.opt("vector",  req.config.vectorProject);
    options.opt("terrain", req.config.terrainProject);

    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // simple option validation
    if (req.assetname.empty()) {
      usage(progname, "<dbname> not specified");
    }
    req.assetname = AssetDefs::NormalizeAssetName(req.assetname,
                                               AssetDefs::Database,
                                               kDatabaseSubtype);

    if (req.config.imageryProject.size()) {
      req.config.imageryProject =
        AssetDefs::NormalizeAssetName(req.config.imageryProject,
                                      AssetDefs::Imagery,
                                      kProjectSubtype);
    }
    if (req.config.vectorProject.size()) {
      req.config.vectorProject =
        AssetDefs::NormalizeAssetName(req.config.vectorProject,
                                      AssetDefs::Vector,
                                      kProjectSubtype);
    }
    if (req.config.terrainProject.size()) {
      req.config.terrainProject =
        AssetDefs::NormalizeAssetName(req.config.terrainProject,
                                      AssetDefs::Terrain,
                                      kProjectSubtype);
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
