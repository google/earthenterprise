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


#include <fstream>
#include <map>
#include <stdio.h>
#include <khGetopt.h>
#include <notify.h>
#include <geFilePool.h>
#include <khProgressMeter.h>
#include <khFileUtils.h>
#include <khSimpleException.h>


typedef std::vector<std::pair<std::string, std::uint64_t> > FileList;
typedef std::map<std::string, bool> DBPathMap;

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
    "usage: %s [options]\n"
    " Clean a dbpath from a disconnected asset root.\n"
    " Options:\n"
    "  --help              Display this usage message.\n"
    "  --list <assetroot>  List all dbpaths currently in disconnected asset root\n"
    "  --dbpath <dbpath>   Which database path to clean.\n"
    "                      This must be a low level path to a database directory\n"
    "\n",
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
    std::string listroot;
    std::vector<std::string> dbpaths_opt;
    khGetopt options;
    options.flagOpt("help", help);
    options.vecOpt("dbpaths", dbpaths_opt);
    options.opt("list", listroot, &khGetopt::DirExists);

    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    if (!listroot.empty()) {
      std::string dbpaths_file = khComposePath(listroot, "dbpaths.list");
      if (!khExists(dbpaths_file)) {
        notify(NFY_FATAL, "Cannot find %s", dbpaths_file.c_str());
      }
      std::vector<std::string> dbpaths;
      khReadStringVectorFromFile(dbpaths_file, dbpaths);
      for (std::vector<std::string>::const_iterator dbpath = dbpaths.begin();
           dbpath != dbpaths.end(); ++dbpath) {
        printf("%s\n", dbpath->c_str());
      }
      exit(0);
    }


    // get the dbpaths that we want to clean
    khFileList to_clean;
    to_clean.AddFiles(dbpaths_opt.begin(), dbpaths_opt.end());
    if (to_clean.empty()) {
      usage(progname, "No dbpaths specified");
    }

    // use the first one to find the dbpaths.list file
    std::string assetroot = khDirname(to_clean[0]);
    while (!khExists(khComposePath(assetroot, "dbpaths.list"))) {
      if (assetroot == "/") {
        notify(NFY_FATAL,
               "Could not find dbpaths.list in a parent directory above %s",
               to_clean[0].c_str());
      }
      assetroot = khDirname(assetroot);
    }

    // load the existing dbpaths.list and put it in a set
    std::vector<std::string> old_dbpaths;
    khReadStringVectorFromFile(khComposePath(assetroot, "dbpaths.list"),
                               old_dbpaths);
    DBPathMap dbpath_map;
    for (std::vector<std::string>::const_iterator dbpath = old_dbpaths.begin();
         dbpath != old_dbpaths.end(); ++dbpath) {
      dbpath_map[*dbpath] = false;
    }

    // make sure that all requested dbpaths are in our set
    {
      unsigned int missing = 0;
      for (std::vector<std::string>::const_iterator dbpath = to_clean.begin();
           dbpath != to_clean.end(); ++dbpath) {
        if (dbpath_map.find(*dbpath) == dbpath_map.end()) {
          ++missing;
          notify(NFY_WARN, "%s is not listed in dbpaths.list",
                 dbpath->c_str());
        } else {
          // mark it as wanting to be cleaned
          dbpath_map[*dbpath] = true;
        }
      }
      if (missing > 0) {
        notify(NFY_FATAL, "Unable to proceed");
      }
    }


    // gather a map of all files belonging to dbpaths we are NOT cleaning
    std::set<std::string> needed_files;
    std::vector<std::string> keep_dbpaths;
    for (DBPathMap::const_iterator dbpath = dbpath_map.begin();
         dbpath != dbpath_map.end(); ++dbpath) {
      if (!dbpath->second) {
        keep_dbpaths.push_back(dbpath->first);

        // we're not cleaning this one
        std::vector<std::string> filelist;
        khReadStringVectorFromFile(khComposePath(dbpath->first, ".filelist"),
                                   filelist);
        for (std::vector<std::string>::const_iterator file = filelist.begin();
             file != filelist.end(); ++file) {
          needed_files.insert(*file);
        }
      }
    }

    // gather a map of all files belonging to dbpaths we ARE cleaning, and
    // that are not needed by other dbpaths
    // gather the parent dirs while we're at it
    std::vector<std::string> toclean_files;
    std::set<std::string> toclean_dirs;
    for (DBPathMap::const_iterator dbpath = dbpath_map.begin();
         dbpath != dbpath_map.end(); ++dbpath) {
      if (dbpath->second) {
        // we are cleaning this one
        std::vector<std::string> filelist;
        khReadStringVectorFromFile(khComposePath(dbpath->first, ".filelist"),
                                   filelist);
        for (std::vector<std::string>::const_iterator file = filelist.begin();
             file != filelist.end(); ++file) {
          if (needed_files.find(*file) == needed_files.end()) {
            toclean_files.push_back(*file);
            std::string dir = khDirname(*file);
            while ((dir != "/") &&
                   (toclean_dirs.find(dir) == toclean_dirs.end())) {
              toclean_dirs.insert(dir);
              dir = khDirname(dir);
            }
          }
        }
      }
    }


    // now go delete the files
    notify(NFY_NOTICE, "Need to delete %lu files",
           static_cast<long unsigned>(toclean_files.size()));
    {
      khProgressMeter progress(toclean_files.size(), "files");
      for (std::vector<std::string>::const_iterator file =
             toclean_files.begin(); file != toclean_files.end(); ++file) {
        if (khExists(*file)) {
          if (!khUnlink(*file)) {
            throw khSimpleErrnoException("Unable to delete ")
              << *file;
          }
        }
        progress.incrementDone();
      }
    }

    notify(NFY_NOTICE, "Cleaning empty directories");
    for (std::set<std::string>::const_reverse_iterator dir
           = toclean_dirs.rbegin(); dir != toclean_dirs.rend(); ++dir) {
      ::rmdir(dir->c_str());
    }


    // write the new dbpaths.listread old list of dbpaths
    std::string perm_file = khComposePath(assetroot, "dbpaths.list");
    khWriteStringVectorToFile(perm_file, keep_dbpaths);

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
