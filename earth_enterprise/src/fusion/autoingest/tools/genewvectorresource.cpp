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
#include <algorithm>
#include <fstream>
#include "common/khConstants.h"
#include "common/khGetopt.h"
#include "common/khSpawn.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/notify.h"
#include "autoingest/.idl/storage/AssetDefs.h"
#include "autoingest/.idl/gstProvider.h"
#include "autoingest/khAssetManagerProxy.h"
#include <qtextcodec.h>


std::string GetSupportedEncodings(void) {
  std::vector<std::string> encodings;
  unsigned int longest = 0;

  // fetch QT's supported encodings & sort them alphbetically
  QTextCodec *codec;
  for (int i = 0; (codec = QTextCodec::codecForIndex(i)); i++) {
    std::string encoding = codec->name().constData();
    encodings.push_back(encoding);
    longest = std::max((size_t)(encoding.size()), (size_t)longest);
  }
  std::sort(encodings.begin(), encodings.end());

  // build a string with them arranged into a nice grid
  std::string retstr;
  unsigned int numcols = 73 / (longest + 1);
  unsigned int numrows = ((encodings.size() + numcols - 1) / numcols);

  for (unsigned int r = 0; r < numrows; ++r) {
    retstr += "      ";
    for (unsigned int c = 0; c < numcols; ++c) {
      unsigned int i = c * numrows + r;
      if (i < encodings.size()) {
        retstr += encodings[i] +
                  std::string(1+longest-encodings[i].size(), ' ');
      }
    }
    retstr += "\n";
  }

  return retstr;
}


void
usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] [--meta <key>=<value>]... -o <assetname>"
          " { --filelist <file> | <sourcefile> ...}\n"
          "   Source filenames may be specified on the commandline or"
          " in a filelist.\n"
          "   If no source files is specified, it will detect change of"
          " original source files, \n "
          "   and update the asset if necessary.\n"
          "   Supported options are:\n"
          "      --help | -?:               Display this usage message\n"
          "      --provider <key>:          Key from provider database\n"
          "      --sourcedate <string>:     String representing UTC date"
          " of source. Default: \"0000-00-00T00:00:00Z\" (YYYY-MM-DDTHH:MM:SSZ).\n"
          "      --layer <num>:             Use specified layer from"
          " source file(s)\n"
          "      --encoding <encodingname>: Use <encodingname> to interpret"
          " source file(s)\n"
          "      --scale <to-meters>:       Convert to meters while importing\n"
          "      --feet:                    Shorthand for --scale=0.3048\n"
          "      --force2D:                 Enforce 2D coordinates\n"
          "      --force25D:                Enforce 2.5D coordinates\n"
          "      --force3D:                 Enforce 3D coordinates\n"
          "      --ignore_bad_features:     Don't fail on bad features,"
          " skip them\n"
          "      --do_not_fix_invalid_geometries:     Don't Fix invalid"
          " geometries (coincident vertices, spikes)\n"
          "   To limit extent, the following options are also available:\n"
          "      --north_boundary <double> Crop to north_boundary"
          " (latitude in decimal deg)\n"
          "      --south_boundary <double> Crop to south_boundary"
          " (latitude in decimal deg)\n"
          "      --east_boundary <double> Crop to east_boundary"
          " (longitude in decimal deg)\n"
          "      --west_boundary <double> Crop to west_boundary"
          " (longitude in decimal deg)\n"
          "   Valid values for <encodingname> are:\n"
          "%s",
          progn.c_str(),
          GetSupportedEncodings().c_str());
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
    VectorProductImportRequest req;
    std::string providerkey;
    // By default set source date to all zeros (format: YYYY-MM-DD).
    std::string sourcedate = kUnknownDate;
    std::string filelistname;
    double north_boundary = 90.0;
    double south_boundary = -90.0;
    double east_boundary = 180.0;
    double west_boundary = -180.0;

    khGetopt options;
    options.flagOpt("debug", debug);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", req.assetname);
    options.opt("provider", providerkey);
    options.opt("sourcedate", sourcedate);
    options.mapOpt("meta", req.meta.map);
    options.opt("filelist", filelistname);
    options.opt("layer", req.config.layer);
    options.opt("encoding", req.config.encoding);
    options.opt("scale", req.config.scale);
    options.setOpt("feet", req.config.scale, 0.3048);
    options.opt("force2D", req.config.force2D);
    options.opt("force25D", req.config.force25D);
    options.opt("force3D", req.config.force3D);
    options.opt("ignore_bad_features", req.config.ignore_bad_features_);
    options.opt("do_not_fix_invalid_geometries",
                req.config.do_not_fix_invalid_geometries_);
    options.opt("north_boundary", north_boundary);
    options.opt("south_boundary", south_boundary);
    options.opt("east_boundary", east_boundary);
    options.opt("west_boundary", west_boundary);

    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // simple option validation
    if (req.assetname.empty()) {
      usage(progname, "<assetname> not specified");
    }
    if (north_boundary <= south_boundary) {
      notify(NFY_FATAL, "north_boundary <= south_boundary!.\n");
    }
    if (east_boundary <= west_boundary) {
      notify(NFY_FATAL, "east_boundary <= west_boundary!.\n");
    }
    req.config.north_boundary = north_boundary;
    req.config.south_boundary = south_boundary;
    req.config.east_boundary = east_boundary;
    req.config.west_boundary = west_boundary;

    req.assetname = AssetDefs::NormalizeAssetName(req.assetname,
                                                  AssetDefs::Vector,
                                                  kProductSubtype);

    if (providerkey.size()) {
      gstProviderSet providers;
      if (!providers.Load()) {
        notify(NFY_FATAL, "Unable to load provider list");
      }
      for (unsigned int i = 0 ; i < providers.items.size(); ++i) {
        if (providers.items[i].key == providerkey) {
          req.config.provider_id_ = providers.items[i].id;
        }
      }
      if (req.config.provider_id_ == 0) {
        notify(NFY_FATAL, "Provider '%s' not found", providerkey.c_str());
      }
    }
    if (sourcedate.size()) {
      // Parse "sourcedate" to standard UTC ISO 8601 time; "ParseUTCTime"
      // also accepts old time string such as "YYYY-MM-DD" in order not to
      // break customers' old scripts.
      struct tm ts;
      if (sourcedate == kUnknownDate) {
        // default input, set to standard metadata value.
        req.meta.SetValue("sourcedate", kUnknownDateTimeUTC);
      } else if (ParseUTCTime(sourcedate, &ts)) {
        // Reformat into standard UTC ISO 8601 time string.
        req.meta.SetValue("sourcedate", GetUTCTimeString(ts));
      } else {
        notify(NFY_FATAL, "Sourcedate '%s' is not a valid UTC ISO 8601 time"
            " string.", sourcedate.c_str());
      }
    }


    // get the source files
    khFileList filelist;
    try {
      if (filelistname.size())
        filelist.AddFromFilelist(filelistname);
      if (argn < argc)
        filelist.AddFiles(&argv[argn], &argv[argc]);
    } catch(const std::exception &e) {
      usage(progname, e.what());
    } catch(...) {
      usage(progname, "Unknown error with source files");
    }
    if (filelist.empty()) {
      bool updated = false;
      QString error;
      if (!khAssetManagerProxy::ProductReImport(req.assetname, updated,
                                                error)) {
        notify(NFY_FATAL, "%s", error.latin1());
      }
      if (updated) {
        printf("The asset is updated: %s\n", req.assetname.c_str());
      } else {
        printf("The asset is up-to-date: %s\n", req.assetname.c_str());
      }
      return 0;
    }

    // add the source files, will normallize & do existence validation
    req.sources.AddFilesAndOrURIs(filelist.begin(), filelist.end());

    // validate inputs files
    {
      CmdLine cmdline;
      cmdline << "/opt/google/bin/gevectorimport" << "--validate"
              << "--layer" << req.config.layer
              << "-o" << "foo.kvp";
      if ( req.config.encoding.size() ) {
        cmdline << "--encoding" << req.config.encoding;
      }
      if ( req.config.scale != 1.0 ) {
        cmdline << "--scale" << req.config.scale;
      }

      if (req.config.force2D) {
        assert(!(req.config.force25D || req.config.force3D));
        cmdline << "--force2D";
      }
      if (req.config.force25D) {
        assert(!(req.config.force2D || req.config.force3D));
        cmdline << "--force25D";
      }
      if (req.config.force3D) {
        assert(!(req.config.force2D || req.config.force25D));
        cmdline << "--force3D";
      }

      if (req.config.ignore_bad_features_) {
        cmdline << "--ignore_bad_features";
      }

      if (req.config.do_not_fix_invalid_geometries_) {
        cmdline << "--do_not_fix_invalid_geometries";
      }

      // write tmp filelist and pass files to gevectorimport
      // via --filelist
      khTmpFileGuard fileguard;
      {
        std::ofstream out(fileguard.name().c_str());
        for (std::vector<std::string>::const_iterator f
               = filelist.begin(); f != filelist.end(); ++f) {
          out << *f << std::endl;
        }
      }
      cmdline << "--filelist" << fileguard.name();

      if (!cmdline.System()) {
        (void)khUnlink(fileguard.name());
        notify(NFY_FATAL, "Unable to validate vector import");
      }
    }

    // now send the request
    if (debug) {
      std::string reqstr;
      req.SaveToString(reqstr, "");
      printf("%s\n", reqstr.c_str());
    } else {
      QString error;
      if (!khAssetManagerProxy::VectorProductImport(req, error)) {
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
