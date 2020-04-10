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
#include <stdio.h>
#include <khGetopt.h>
#include <notify.h>
#include <geFilePool.h>
#include <khProgressMeter.h>
#include <khFileUtils.h>
#include <config/geConfigUtil.h>
#include <config/gefConfigUtil.h>

typedef std::vector<std::pair<std::string, std::uint64_t> > FileList;

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
    " Copy disconnectedpublish media to mock assetroot.\n"
    " Options:\n"
    "  --help              Display this usage message.\n"
    "  --input <dirname>  Where to gather the files. [required]\n"
    "                     Typically will be the mountpoint of a harddrive\n"
    "\n",
          progn.c_str());
  exit(1);
}


void RemoveDuplicates(const std::vector<std::string> &in,
                      std::vector<std::string> &out) {
  std::set<std::string> have;
  for (std::vector<std::string>::const_iterator i = in.begin();
       i != in.end(); ++i) {
    if (have.find(*i) == have.end()) {
      out.push_back(*i);
      have.insert(*i);
    }
  }
}

int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];

    // process commandline options
    int argn;
    bool help = false;
    std::string input;
    khGetopt options;
    options.flagOpt("help", help);
    options.opt("input", input, &khGetopt::DirExists);
    options.setRequired("input");

    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    // Make sure I'm root, everything is shut down,
    // and switch to fusion user
    AssertRunningAsRoot();
    std::string username = FusionUserNameServerOnly();
    std::string groupname = UserGroupnameServerOnly();
    printf("Switching to (%s, %s)\n", username.c_str(), groupname.c_str());
    SwitchToUser(username, groupname);

    // get the lists from the input dir
    std::vector<std::string> files;
    khReadStringVectorFromFile(khComposePath(input, "files.list"), files);
    std::vector<std::string> dbpaths;
    khReadStringVectorFromFile(khComposePath(input, "dbpaths.list"), dbpaths);

    // Get the size info for each file to be copied
    FileList to_copy;
    to_copy.reserve(files.size());
    std::uint64_t totalsize = 0;
    for (std::vector<std::string>::const_iterator file = files.begin();
         file != files.end(); ++file) {
      std::string infile = khComposePath(input, *file);
      std::uint64_t filesize = khGetFileSizeOrThrow(infile);
      to_copy.push_back(std::make_pair(*file, filesize));
      totalsize += filesize;
    }

    notify(NFY_NOTICE, "Need to receive %lu files",
           static_cast<long unsigned>(to_copy.size()));
    {
      khProgressMeter progress(totalsize, "bytes");
      for (FileList::const_iterator file = to_copy.begin();
           file != to_copy.end(); ++file) {
        std::string dest = file->first;
        std::string src  = khComposePath(input, dest);
        if (!khLinkOrCopyFile(src, dest)) {
          notify(NFY_FATAL, "Unable to proceed");
        }
        progress.incrementDone(file->second);
      }
    }

    // copy filelists to each dbpath
    notify(NFY_NOTICE, "Copying filelists");
    for (std::vector<std::string>::const_iterator dbpath = dbpaths.begin();
         dbpath != dbpaths.end(); ++dbpath) {
      std::string dest = khComposePath(*dbpath, ".filelist");
      std::string src  = khComposePath(input, dest);
      if (!khLinkOrCopyFile(src, dest)) {
        notify(NFY_FATAL, "Unable to proceed");
      }
    }

    // read assetroot from input dir
    std::string assetroot;
    if (!khReadStringFromFile(khComposePath(input, ".assetroot"),
                              assetroot)) {
      notify(NFY_FATAL, "Unable to proceed");
    }

    // read old list of dbpaths
    std::vector<std::string> old_dbpaths;
    std::string perm_file = khComposePath(assetroot, "dbpaths.list");
    notify(NFY_NOTICE, "Updating %s", perm_file.c_str());
    if (khExists(perm_file)) {
      khReadStringVectorFromFile(perm_file, old_dbpaths);
    }
    std::copy(dbpaths.begin(), dbpaths.end(), back_inserter(old_dbpaths));
    std::vector<std::string> new_dbpaths;
    RemoveDuplicates(old_dbpaths, new_dbpaths);
    khWriteStringVectorToFile(perm_file, new_dbpaths);

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
