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


#include <set>
#include <khGetopt.h>
#include <khstl.h>
#include <autoingest/plugins/SourceAsset.h>
#include <autoingest/khVolumeManager.h>

void
usage(const std::string &progn, const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <volume>...\n"
          "   Find sources that should reside on the specified volume(s).\n"
          "   Only the most recent sources for each asset will be listed.\n"
          "   Supported options are:\n"
          "      --help | -?:               Display this usage message\n"
          "      --assetroot <assetroot>:   Use alternate asset root\n",
          progn.c_str());
  exit(1);
}


void FindSources(const std::string &assetroot,
                 const std::set<std::string> &checkvols,
                 std::vector<std::string> &found);


int
main(int argc, char *argv[]) {
  try {
    khGetopt options;
    bool help = false;
    int argn;
    std::string assetroot;
    std::set<std::string> checkvols;

    std::string progname(argv[0]);
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt<std::string>("assetroot", assetroot);

    if (!options.processAll(argc, argv, argn) || help) {
      usage(progname);
    }


    // override the assetroot first. That way theVolumeManager will
    // load the correct volume definitions
    if (assetroot.length()) {
      AssetDefs::OverrideAssetRoot(assetroot);
    } else {
      assetroot = AssetDefs::AssetRoot();
    }

    // Double check the supplied volume list
    if (argn == argc) {
      usage(progname, "No volumes specified");
    } else {
      unsigned int numbad = 0;
      while (argn < argc) {
        std::string vol(argv[argn]);
        if (theVolumeManager.GetVolumeDef(vol) == 0) {
          notify(NFY_WARN, "No volume named '%s' found",
                 vol.c_str());
          ++numbad;
        } else {
          checkvols.insert(vol);
        }
        ++argn;
      }
      if (numbad) {
        notify(NFY_FATAL, "Unable to proceed");
      }
    }

    // go do the real work
    std::vector<std::string> found;
    FindSources(assetroot, checkvols, found);

    for (std::vector<std::string>::const_iterator aref = found.begin();
         aref != found.end(); ++aref) {
      Asset asset(*aref);
      if (!asset) {
        notify(NFY_WARN, "Unable to load asset for %s",
               aref->c_str());
      } else {
        std::vector<std::string> infiles;
        asset->GetInputFilenames(infiles);
        fprintf(stdout, "%s\n", aref->c_str());
        for (std::vector<std::string>::const_iterator i =
               infiles.begin();
             i != infiles.end(); ++i) {
          fprintf(stdout, "\t%s\n", i->c_str());
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



void
FindSources(const std::string &assetroot,
            const std::set<std::string> &checkvols,
            std::vector<std::string> &found)
{
  if (chdir(assetroot.c_str()) != 0) {
    notify(NFY_FATAL, "Unable to chdir to %s: %s",
           assetroot.c_str(), khstrerror(errno).c_str());
  }

  notify(NFY_NOTICE, "Finding sources on volume%s %s ...",
         (checkvols.size() == 1) ? "" : "s",
         join(checkvols.begin(), checkvols.end(), ", ").c_str());


  FILE *findPipe =
    popen("find . \\( -name source.kisource -o "
          "           -name source.ktsource -o "
          "           -name source.kvsource \\) -print", "r");
  if (!findPipe) {
    notify(NFY_FATAL, "Unable to invoke find: %s",
           khstrerror(errno).c_str());
  }

  unsigned int numchecked = 0;
  char buf[1024];
  while (fgets(buf, sizeof(buf), findPipe)) {
    std::string dir(TrimTrailingWhite(buf));
    dir.erase(0, 2); // remove leading "./"

    // Source assets always have a version made immediately.
    // We don't have to worry about the case where the asset has been made
    // but not built.
    SourceAssetVersion sourcever(dir);
    if (!sourcever) {
      notify(NFY_WARN, "\nUnable to load assetversion %s", dir.c_str());
    } else if (sourcever->state == AssetDefs::Succeeded) {
      // Just check the first source file. Technically the sources could
      // be on separate volumes, but we NEVER do it that way. It would
      // be a headache to manage.
      ++numchecked;
      khFusionURI uri(sourcever->config.sources[0].uri);
      if (uri.Volume().empty()) {
        // Non-file source. We can ignore this here.
      } else if (checkvols.find(uri.Volume()) != checkvols.end()) {
        // One of the volumes we're looking for
        found.push_back(khDirname(dir));
      } else {

      }
      if (numchecked % 10 == 0) {
        fprintf(stderr, "\rNum found/checked: %u/%u",
                (unsigned int)found.size(), numchecked);
        fflush(stderr);
      }
    }
  }

  (void)pclose(findPipe);

  fprintf(stderr, "\rNum found/checked: %u/%u\n",
          (unsigned int)found.size(), numchecked);
  fflush(stderr);


}
