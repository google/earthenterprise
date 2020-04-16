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
#include <set>

#include "common/notify.h"
#include "common/khConstants.h"
#include "common/khGetopt.h"
#include "common/khstl.h"
#include "common/khSpawn.h"
#include "fusion/autoingest/.idl/storage/AssetDefs.h"
#include "fusion/autoingest/khAssetManagerProxy.h"
#include "fusion/autoingest/plugins/VectorProjectAsset.h"
#include "common/quadtreepath.h"

void usage(const char *prog, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <assetname> [<version>]\n"
          "   <version> can be a number, 'current', or 'lastgood'\n"
          "      if ommited <assetname> is checked for '?version=...'\n"
          "   Supported options are:\n"
          "      --help | -?:    Display this usage message\n"
          "      --versions:     Display build versions and their status\n"
          "      --status:       Display status of specified version\n"
          "      --dependencies: Display dependencies of specified version\n"
          "      --maxdepth:     Maximum depth to show for dependencies\n"
          "      --infiles:      Display input filenames for specified version\n"
          "      --outfiles:     Display output filenames for specified version\n"
          "      --logfile:      Display filename of logfile for specified version\n"
          "      --showlog:      Display contents of logfile for specified version\n"
          "      --taillog:      'tail -f' logfile for specified version\n"
          "      --blockers:     Display sub-versions that block specified version\n",
          prog);
  exit(1);
}


#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)


typedef std::set<std::string> SeenSet;
static
void DisplayVersionDependencies(const AssetVersion &version,
                                unsigned int indent,
                                unsigned int maxdepth,
                                SeenSet &seen,
                                const std::string &prefix);
static
void DisplayBlockerDependencies(const AssetVersion &version,
                                SeenSet &seen,
                                const std::string &skipInputExt);
static
void DisplayRasterProjectProgress(const AssetVersion &version, SeenSet &seen);


int main(int argc, char *argv[]) {
  try {
    // process commandline options
    int argn;
    bool help = false;
    bool versions     = false;
    bool status       = false;
    bool dependencies = false;
    bool infiles      = false;
    bool outfiles     = false;
    bool logfile      = false;
    bool showlog      = false;
    bool taillog      = false;
    bool didsomething = false;
    unsigned int maxdepth     = uint(-1);
    bool showblockers = false;
    bool rasterprojprog = false;
    bool reloadconfig = false;
    std::string quadpath;
    std::string lrc;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("versions", versions);
    options.flagOpt("status", status);
    options.flagOpt("dependencies", dependencies);
    options.flagOpt("deps", dependencies);  // common misspelled abbrev
    options.flagOpt("infiles", infiles);
    options.flagOpt("outfiles", outfiles);
    options.flagOpt("logfile", logfile);
    options.flagOpt("showlog", showlog);
    options.flagOpt("taillog", taillog);
    options.flagOpt("tailog", taillog);  // common misspelling
    options.opt("maxdepth", maxdepth);
    options.flagOpt("blockers", showblockers);
    //      options.flagOpt("rasterprojprog", rasterprojprog);
    options.flagOpt("reloadconfig", reloadconfig);
    options.opt("quadpath", quadpath);
    options.opt("lrc", lrc);
    if (!options.processAll(argc, argv, argn)) {
      usage(argv[0]);
    }
    if (help) {
      usage(argv[0]);
    }

    if (reloadconfig) {
      QString error;
      if (!khAssetManagerProxy::ReloadConfig("dummy", error)) {
        notify(NFY_FATAL, "%s", error.latin1());
      }
      exit(0);
    }

    if (!lrc.empty()) {
      std::vector<std::string> parts;
      split(lrc, ",", back_inserter(parts));
      unsigned int level, row, col;
      if (parts.size() == 3) {
        FromString(parts[0], level);
        FromString(parts[1], row);
        FromString(parts[2], col);
        QuadtreePath qpath(level, row, col);
        printf("%u,%u,%u = %s\n", level, row, col, qpath.AsString().c_str());
      } else {
        usage(argv[0], "--lrc expects 'level,row,col'");
      }
      exit(0);
    }

    if (!quadpath.empty()) {
      QuadtreePath qpath(quadpath);
      std::uint32_t level, row, col;
      qpath.GetLevelRowCol(&level, &row, &col);
      printf("%s = %u,%u,%u\n", qpath.AsString().c_str(), level, row, col);
      exit(0);
    }

    // process rest of commandline arguments
    const char *assetname = GetNextArg();
    const char *verstr = GetNextArg();
    if (!assetname) {
      usage(argv[0], "<assetname> not specified");
    }
    if (!verstr) verstr = "";
    AssetVersionRef verref(AssetDefs::GuessAssetVersionRef
                           (assetname, verstr).Bind());

    Asset asset(verref.AssetRef());
    AssetVersion version(verref);


    if (versions) {
      didsomething = true;
      if (asset->versions.size()) {
        for (AssetStorage::VersionList::const_iterator v =
               asset->versions.begin();
             v != asset->versions.end(); ++v) {
          AssetVersion ver(*v);
          printf("%s: %s\n", v->c_str(),
                 ver->PrettyState().c_str());
        }
      } else {
        printf("NO VERSIONS\n");
      }
    }

    if (status) {
      didsomething = true;
      printf("%s\n", version->PrettyState().c_str());
    }

    if (dependencies) {
      didsomething = true;
      SeenSet alreadySeen;
      DisplayVersionDependencies(version, 0, maxdepth,
                                 alreadySeen, std::string());
    }

    if (showblockers) {
      didsomething = true;
      SeenSet alreadySeen;
      DisplayBlockerDependencies(version, alreadySeen, std::string());
    }

    if (rasterprojprog) {
      didsomething = true;
      SeenSet alreadySeen;
      DisplayRasterProjectProgress(version, alreadySeen);
    }

    if (infiles) {
      didsomething = true;
      std::vector<std::string> infnames;
      version->GetInputFilenames(infnames);
      for (std::vector<std::string>::const_iterator f = infnames.begin();
           f != infnames.end(); ++f) {
        printf("%s\n", f->c_str());
      }
    }

    if (outfiles) {
      didsomething = true;
      std::vector<std::string> outfnames;
      version->GetOutputFilenames(outfnames);
      for (std::vector<std::string>::const_iterator f =
             outfnames.begin();
           f != outfnames.end(); ++f) {
        printf("%s\n", f->c_str());
      }
    }


    if (logfile || showlog || taillog) {
      didsomething = true;
      if (version->IsLeaf()) {
        std::string logfile = version->Logfile();
        if (khExists(logfile)) {
          printf("%s\n", logfile.c_str());
          CmdLine cmdline;
          if (taillog) {
            cmdline << "tail" << "-f" << logfile;
          } else if (showlog) {
            const char *pager = getenv("PAGER");
            if (!pager) pager = "more";
            cmdline << pager << logfile;
          }
          (void)cmdline.System();
        } else {
          printf("No logfile (yet)\n");
        }
      } else {
        printf("No logfile (ever)\n");
      }
    }


    if (!didsomething) {
      usage(argv[0], "No query specified");
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}


static
void DisplayVersionDependencies(const AssetVersion &version,
                                unsigned int indent,
                                unsigned int maxdepth,
                                SeenSet &seen,
                                const std::string &prefix) {
  if (indent > maxdepth) return;
  bool printLegend = (indent == 0);
  std::string statestr = version->PrettyState();
  if (seen.find(version->GetRef()) != seen.end()) {
    printf("R  %s%s%s: %s\n",
           std::string((indent-1)*3, ' ').c_str(),
           prefix.c_str(),
           version->GetRef().toString().c_str(),
           statestr.c_str());
  } else {
    seen.insert(version->GetRef());
    printf("%s%s%s: %s\n",
           std::string(indent*3, ' ').c_str(),
           prefix.c_str(),
           version->GetRef().toString().c_str(),
           statestr.c_str());
    ++indent;
    if (version->inputs.size()) {
      for (const auto &input : version->inputs) {
        DisplayVersionDependencies(AssetVersion(input), indent,
                                   maxdepth,
                                   seen, "< ");
      }
    }
    if (!version->IsLeaf()) {
      for (const auto &child : version->children) {
        DisplayVersionDependencies(AssetVersion(child), indent,
                                   maxdepth,
                                   seen, "+ ");
      }
    }
  }
  if (printLegend) {
    printf("\n");
    printf("< -- input\n");
    printf("+ -- subpart\n");
    printf("R -- repeat item, not expanded again\n");
  }
}


static
void DisplayBlockerDependencies(const AssetVersion &version,
                                SeenSet &seen,
                                const std::string &skipInputExt) {
  if (seen.find(version->GetRef()) == seen.end()) {
    // we have not seen this one yet
    seen.insert(version->GetRef());

    if (version->state & (AssetDefs::Canceled |
                          AssetDefs::Failed |
                          AssetDefs::Offline |
                          AssetDefs::Bad)) {
      printf("%s: %s\n", version->GetRef().toString().c_str(),
             version->PrettyState().c_str());
    } else if (version->state == AssetDefs::Blocked) {
      std::string myskipext = skipInputExt;
      if ((version->subtype == kProjectSubtype) &&
          ((version->type == AssetDefs::Imagery) ||
           (version->type == AssetDefs::Terrain))) {
        if (version->children.size()) {
          if (version->type == AssetDefs::Imagery) {
            myskipext = kImageryAssetSuffix;
          } else {
            myskipext = kTerrainAssetSuffix;
          }
        }
      } else if ((version->subtype == kMercatorProjectSubtype) &&
                 (version->type == AssetDefs::Imagery) &&
                 (version->children.size() != 0)) {
        myskipext = kMercatorImageryAssetSuffix;
      }

      if (version->inputs.size()) {
        if (myskipext.empty()) {
          for (const auto &input : version->inputs) {
            DisplayBlockerDependencies(AssetVersion(input),
                                       seen, myskipext);
          }
        } else {
          for (const auto &input : version->inputs) {
            if (!EndsWith(input, myskipext)) {
              DisplayBlockerDependencies(AssetVersion(input),
                                         seen, myskipext);
            }
          }
        }
      }
      if (!version->IsLeaf()) {
        for (const auto &child : version->children) {
          DisplayBlockerDependencies(AssetVersion(child),
                                     seen, myskipext);
        }
      }
    }
  }
}

static
void DisplayRasterProjectProgress(const AssetVersion &version,
                                  SeenSet &seen) {
  throw khException(kh::tr("DisplayRasterProjectProgress not finished"));


  if (((version->type != AssetDefs::Imagery) &&
       (version->type != AssetDefs::Terrain)) ||
      (version->subtype != kProjectSubtype)) {
    throw khException(kh::tr("AssetVersion is not a RasterProject"));
  }

  if (version->children.size()) {
    // We're already have our children, by definition our inputs must
    // be good. (RasterProject has DelayedBuildChildren)
  } else {
    // We don't have children, so all me need to look at is our inputs
    //for (const auto &input : version->inputs) {
      // TODO: implementation.
    //}
  }
}
