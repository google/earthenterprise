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
#include <khstl.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khVolumeManager.h>
#include <autoingest/AssetVersion.h>
#include "common/khException.h"

void
usage(const char *prog, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] {--vol=<volume>}... {--assetver=<assetver>}... \n"
          "   Find all the low level files needed for assetver that are supposed to be on the supplied volume.\n"
          "   Multiple volumes and assetvers may be specified.\n"
          "   Supported options are:\n"
          "      --help | -?:    Display this usage message\n"
          "      --interval <num>: Display a progress tick every <num> vers checked\n",
          prog);
  exit(1);
}


class RebuildNode {
 public:
  typedef std::vector<RebuildNode*> CurrentStack;
  typedef std::set<RebuildNode*>    UserSet;

  std::string verref;
  int buildindex;
  UserSet users;
  bool rebuild;

  RebuildNode(const std::string &ref,
              const CurrentStack &currstack) :
      verref(ref),
      buildindex(-1),
      rebuild(false)
  {
    if (currstack.size()) {
      users.insert(currstack.back());
    }
  }
  void AddUser(const CurrentStack &currstack) {
    if (currstack.size()) {
      users.insert(currstack.back());
    }
  }
  void AdjustBuildIndex(int childi) {
    int targetindex = rebuild ? childi+1 : childi;
    if (targetindex > buildindex) {
      buildindex = targetindex;
      for (UserSet::iterator u = users.begin(); u != users.end(); ++u) {
        (*u)->AdjustBuildIndex(buildindex);
      }
    }
  }
};

class CurrentStackGuard
{
 public:
  RebuildNode::CurrentStack &currStack;
  RebuildNode *node;

  CurrentStackGuard(RebuildNode::CurrentStack &currStack_,
                    RebuildNode *node_)
      : currStack(currStack_), node(node_) {
    currStack.push_back(node);
  }
  ~CurrentStackGuard(void) {
    currStack.pop_back();
  }
};



typedef std::map<std::string, RebuildNode*> SeenVers;
typedef std::set<RebuildNode*> RebuildVers;


void FindFilesOnVol(const std::string &verref,
                    const std::set<std::string> &volset,
                    SeenVers &seen,
                    RebuildVers &found,
                    RebuildNode::CurrentStack &currentStack,
                    int &count, int interval);

void
ValidateVolume(const std::string &volname)
{
  if (theVolumeManager.GetVolumeDef(volname) == 0) {
    throw khException(kh::tr("Unknown volume '%1' specified with --vol")
                      .arg(volname.c_str()));
  }
}

void
ValidateAssetVer(const std::string &verstr)
{
  AssetVersionRef verref(AssetDefs::GuessAssetVersionRef
                         (verstr, std::string()).Bind());
  AssetVersion assetver(verref);
  if (!assetver) {
    throw khException(kh::tr("Unknown asset version '%1' specified with --assetver")
                      .arg(verstr.c_str()));
  }
}



int
main(int argc, char *argv[]) {

  try {
    // process commandline options
    int argn;
    bool help = false;
    std::vector<std::string> vols;
    std::vector<std::string> verstrs;
    int interval = 0;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.vecOpt("vol", vols, ValidateVolume);
    options.vecOpt("assetver", verstrs, ValidateAssetVer);
    options.opt("interval", interval);
    if (!options.processAll(argc, argv, argn)) {
      usage(argv[0]);
    }
    if (help) {
      usage(argv[0]);
    }

    // sanity check the inputs
    if (vols.empty()) {
      usage(argv[0], "You must specify at least one --vol");
    }
    if (verstrs.empty()) {
      usage(argv[0], "You must specify at least one --assetver");
    }

    // convert the vols vector to a set
    std::set<std::string> volset(vols.begin(), vols.end());


    // traverse assetvers & dependencies looking for outfiles on a volume
    // specified in volset
    SeenVers seen;
    RebuildVers found;
    int count = 0;
    for (std::vector<std::string>::const_iterator verstr = verstrs.begin();
         verstr != verstrs.end(); ++verstr) {
      AssetVersionRef verref(AssetDefs::GuessAssetVersionRef
                             (*verstr, std::string()).Bind());
      AssetVersion assetver(verref);
      assert(assetver);  // ValidateAssetVer should guarantee this

      RebuildNode::CurrentStack currentStack;

      FindFilesOnVol(verref, volset, seen, found, currentStack,
                     count, interval);
    }

    if (interval) {
      fprintf(stderr, "\r%d\n", count);
    }

    if (found.empty()) {
      notify(NFY_NOTICE, "No succeeded outfiles found on the supplied volumes");
    } else {
      for (RebuildVers::const_iterator f = found.begin();
           f != found.end(); ++f) {
        (*f)->AdjustBuildIndex(-1);
      }
      for (RebuildVers::const_iterator f = found.begin();
           f != found.end(); ++f) {
        printf("%02d %s\n", (*f)->buildindex, (*f)->verref.c_str());
      }
    }

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}


void
FindFilesOnVol(const std::string &verref,
               const std::set<std::string> &volset,
               SeenVers &seen,
               RebuildVers &found,
               RebuildNode::CurrentStack &currentStack,
               int &count, int interval)
{
  if (seen.find(verref) != seen.end()) {
    seen[verref]->AddUser(currentStack);
    return;
  }

  AssetVersion version(verref);
  if (!version) {
    notify(NFY_WARN, "Unable to load asset version %s",
           version.Ref().toString().c_str());
  }

  RebuildNode *node = new RebuildNode(verref, currentStack);
  CurrentStackGuard stackGuard(currentStack, node);

  seen.insert(make_pair(verref, node));

  if (interval && (++count % interval == 0)) {
    fprintf(stderr, "\r%d", count);
  }


  if (version->IsLeaf() && (version->state == AssetDefs::Succeeded)) {

    std::vector<std::string> outfiles;
    version->GetOutputFilenames(outfiles);

    for (std::vector<std::string>::const_iterator outfile =
           outfiles.begin(); outfile != outfiles.end(); ++outfile) {
      khFusionURI uri(theVolumeManager.DeduceURIFromPath(*outfile));
      if (uri.Volume().empty()) {
        // non-file outfile (GFS?). Do nothing.
      } else if (volset.find(uri.Volume()) != volset.end()) {
        // This outfile is on a volume we care about. Remember the
        // versionref.

        node->rebuild = true;
        found.insert(node);

        // once we find even one outfile, we're done. We'll have to run
        // the whole command anyway. No sense checking other outfiles.
        break;
      } else {
        // On a volume we don't care about. Do Nothing
      }
    }
  }

  for (const auto &input : version->inputs) {
    FindFilesOnVol(input, volset, seen,
                   found, currentStack,
                   count, interval);
  }
  if (!version->IsLeaf()) {
    for (const auto &child: version->children) {
      FindFilesOnVol(child, volset, seen,
                     found, currentStack,
                     count, interval);
    }
  }
}
