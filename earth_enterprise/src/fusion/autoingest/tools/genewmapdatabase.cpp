// Copyright 2017 Google Inc.
// Copyright 2018 the Open GEE Contributors
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
          "\nusage: %s [options] [--meta <key>=<value>]... -o <dbname> [--imagery <imagery project>] [--map <map project>] [--mercator : use mercator projection] [--flat : use flat(Plate Carre) projection (default)]\n"
          "   Supported options are:\n"
          "      --help | -?:               Display this usage message\n",
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
    bool flat = false;
    bool mercator = false;
    MapDatabaseEditRequest req;
    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", req.assetname);
    options.mapOpt("meta", req.meta.map);
    options.opt("imagery", req.config.imageryProject);
    options.opt("map",  req.config.mapProject);
    options.flagOpt("mercator", mercator);
    options.flagOpt("flat", flat);
    options.setExclusive("flat", "mercator");


    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }
    if (!flat && !mercator) {
      flat = true;
    }

    // figure out which cmd to run
    bool (*cmd)(const MapDatabaseEditRequest &, QString&, int) = 0;
    if (progname.find("new") != std::string::npos) {
      cmd = mercator ? &khAssetManagerProxy::MercatorMapDatabaseNew
                     : &khAssetManagerProxy::MapDatabaseNew;
    } else if (progname.find("modify") != std::string::npos) {
      cmd = mercator ? &khAssetManagerProxy::MercatorMapDatabaseModify
                     : &khAssetManagerProxy::MapDatabaseModify;
    } else {
      usage(argv[0], "Internal Error: Don't know which command to run.");
    }

    // simple option validation
    if (req.assetname.empty()) {
      usage(progname, "<dbname> not specified");
    }
    req.assetname = AssetDefs::NormalizeAssetName(
        req.assetname, AssetDefs::Database,
        mercator ? kMercatorMapDatabaseSubtype : kMapDatabaseSubtype);

    // If the database is mercator, prioritize mercator imagery projects if
    // there is a naming collision. Flat databases can only use flat imagery
    // projects.
    if (req.config.imageryProject.size()) {
      if (mercator) {
        req.config.imageryProject =
          AssetDefs::NormalizeAssetName(
              req.config.imageryProject, AssetDefs::Imagery,
              kMercatorProjectSubtype);
        if (!khDirExists(AssetDefs::AssetPathToFilename(req.config.imageryProject))) {
          req.config.imageryProject =
            AssetDefs::NormalizeAssetName(
                khDropExtension(req.config.imageryProject), AssetDefs::Imagery,
                kProjectSubtype);
        }
      }
      else {
        req.config.imageryProject =
          AssetDefs::NormalizeAssetName(
              req.config.imageryProject, AssetDefs::Imagery, kProjectSubtype);
      }
    }

    if (req.config.mapProject.size()) {
      req.config.mapProject =
        AssetDefs::NormalizeAssetName(req.config.mapProject,
                                      AssetDefs::Map,
                                      kProjectSubtype);
    }

    req.config.is_mercator = mercator;
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
