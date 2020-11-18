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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <set>
#include <fstream>
#include "common/khConstants.h"
#include "common/khGetopt.h"
#include "common/notify.h"
#include "common/geFilePool.h"
#include "fusion/autoingest/AssetVersion.h"
#include "fusion/dbmanifest/dbmanifest.h"
#include "common/khSpawn.h"
#include "common/khProgressMeter.h"
#include "common/khException.h"


namespace {

typedef std::pair<std::string, std::uint64_t> NameSizePair;
typedef std::map<std::string, NameSizePair> FileMap;
typedef std::vector<std::pair<std::string, NameSizePair> > FileList;

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
    "Example: gedisconnectedsend --sendpath /gevol/assets/Databases/world.kdat"
    "abase/gedb.kda/ver004/gedb --output /tmp/x\n"
    "Example: gedisconnectedsend --sendver \"Databases/world.kdatabase?version"
    "=004\" --report_size_only\n"
    "usage: %s [options]\n"
    " Gather all the files necessary to do a disconnected publish.\n"
    " Options:\n"
    "  --help              Display this usage message.\n"
    "  --sendver <dbver>   Which database version to send.\n"
    "                      This must be in the \"?version=...\" syntax\n"
    " e.g 'db/test.kdatabase?version=74' (for 74th version, quoting for ?)\n"
    " e.g db/test.kdatabase (for latest version)\n"
    "  --sendpath <dbpath> Which database path to send.\n"
    "                   This must be a low level path to a database directory\n"
    "                   You can get this from 'gequery --outfiles <dbver>'\n"
    "                e.g /gevol/assets/db/test.kdatabase/gedb.kda/ver074/gedb\n"
    "  --havepath <dbpath> Which database path exists on target machine.\n"
    "                   This must be a low level path to a database directory\n"
    "                   May be specified more than once.\n"
    "  --havepathfile <file>  File containing list of existing database paths\n"
    "                   (copy <assetroot>/dbpaths.list from remote machine)\n"
    "  --output <dirname> | --report_size_only  | --manifest_only [required]\n"
    "                   Where to gather the files.\n"
    "                   The directory must already exists and be empty.\n"
    "                   Typically will be the mountpoint of a harddrive.\n"
    "                   If report_size_only is used, only size will be\n"
    "                   reported. Else if manifest_only is used, only \n"
    "                   manifest will be printed.\n"
    "  --extra <filename>  Extra file to package.\n"
    "                   Typically used to repair broken files.\n"
    "\n",
          progn.c_str());
  exit(1);
}


void ValidateDBVersions(const std::vector<std::string> &invec,
                        khFileList &dbpaths) {
  for (std::vector<std::string>::const_iterator s = invec.begin();
       s != invec.end(); ++s) {
    const std::string& database_name = *s;
    std::string gedb_path;
    if (!AssetVersionImpl::GetGedbPath(database_name, &gedb_path)) {
      notify(NFY_FATAL, "Unable to interpret DB '%s'.", database_name.c_str());
    }
    dbpaths.AddFile(gedb_path);
  }
}


void ValidateDBPaths(const std::vector<std::string>& db_paths) {
  size_t num_paths = db_paths.size();
  std::string db_type;
  for (size_t i = 0; i < num_paths; ++i) {
    if (!khIsAbsoluteDbName(db_paths[i], &db_type)) {
      notify(NFY_FATAL, "Unable to interpret DB '%s'.", db_paths[i].c_str());
    }
  }
}


void RemoveDuplicates(const khFileList &in,
                      std::vector<std::string> &out) {
  std::set<std::string> have;
  for (khFileList::const_iterator i = in.begin(); i != in.end(); ++i) {
    if (have.insert(*i).second) {
      out.push_back(*i);
    }
  }
}


void PopulateFileMap(geFilePool &file_pool,
                     std::vector<std::string> *dbpaths,
                     FileMap &files,
                     std::vector<std::vector<std::string> > *filelists,
                     const std::string& tmp_dir) {
  try {
    if (filelists) {
      filelists->resize(dbpaths->size());
    }
    for (unsigned int i = 0; i < dbpaths->size(); ++i) {
      // Get the manifest object. May throw an exception.
      DbManifest dbmanifest(&(*dbpaths)[i]);

      std::vector<ManifestEntry> manifest;
      notify(NFY_DEBUG, "Building manifest for: %s", (*dbpaths)[i].c_str());
      dbmanifest.GetDisconnectedManifest(file_pool, manifest, tmp_dir);
      if (filelists) {
        (*filelists)[i].reserve(manifest.size());
      }
      for (std::vector<ManifestEntry>::const_iterator entry = manifest.begin();
           entry != manifest.end(); ++entry) {
        NameSizePair* name_size = &files[entry->current_path];
        *name_size = std::make_pair(entry->orig_path, entry->data_size);
        if (filelists) {
          (*filelists)[i].push_back(entry->orig_path);
        }
        for(size_t idx = 0; idx < entry->dependents.size(); ++idx)
        {
          const ManifestEntry& dep_entry = entry->dependents[idx];
          NameSizePair* dep_name_size = &files[dep_entry.current_path];
          *dep_name_size = std::make_pair(dep_entry.orig_path, dep_entry.data_size);
          if (filelists) {
            (*filelists)[i].push_back(dep_entry.orig_path);
          }
        }
      }
      notify(NFY_DEBUG, "Done building file map for: %s", (*dbpaths)[i].c_str());
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  }
}

}  // namespace


int main(int argc, char *argv[]) {
  try {
    std::string progname = argv[0];

    // Process commandline options.
    int argn;
    bool help = false;
    bool report_size_only = false;
    bool manifest_only = false;
    std::vector<std::string> sendvers;
    std::vector<std::string> sendpaths_opt;
    std::vector<std::string> havepaths_opt;
    std::vector<std::string> extrafiles;
    std::string havepathfile;
    std::string output;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("report_size_only", report_size_only);
    options.flagOpt("manifest_only", manifest_only);
    options.vecOpt("sendver",  sendvers);
    options.vecOpt("sendpath", sendpaths_opt);
    options.vecOpt("havepath", havepaths_opt);
    options.opt("havepathfile", havepathfile);
    options.vecOpt("extra", extrafiles);
    options.opt("output", output, &khGetopt::DirExists);
    options.setExclusive("report_size_only", "manifest_only");

    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }
    if (report_size_only || manifest_only) {
      if (!output.empty()) {
        notify(
            NFY_NOTICE,
            "Ignoring output option as %s option has been used.",
            report_size_only ? "report_size_only" : "manifest_only");
      }
    } else {
      if (output.empty()) {
        notify(NFY_WARN, "Either output or report_size_only or manifest_only "
                         "option is a must.");
        usage(progname);
      }
    }

    // Verify that the output directory is empty.
    if (!(report_size_only || manifest_only) && khCountFilesInDir(output)) {
      notify(NFY_FATAL, "%s must be empty", output.c_str());
    }

    geFilePool file_pool;

    // Get the list of send_paths.
    // Note: Since GEE-5.1.2 we create database manifest files list when
    // building database, and do not delete it when cleaning. It allows us to
    // get a database manifest for a cleaned database version.
    // Do not check files existence in case of manifest_only-query. The database
    // version may be cleaned, but we may still provide manifest files list.
    const bool kCheckExistanceForSend = !manifest_only;
    khFileList send_paths(kCheckExistanceForSend);
    ValidateDBVersions(sendvers, send_paths);
    ValidateDBPaths(sendpaths_opt);
    send_paths.AddFiles(sendpaths_opt.begin(), sendpaths_opt.end());
    std::vector<std::string> to_send;
    RemoveDuplicates(send_paths, to_send);
    if (to_send.empty()) {
      usage(progname, "You must specify at least one --sendver or --sendpath");
    }


    // Get the list of havepaths.
    // Note: Do not check existence since database version may be cleaned.
    // Since GEE-5.1.2 the database version may be cleaned, but we can provide
    // manifest files list.
    const bool kCheckExistanceForHave = false;
    khFileList have_paths(kCheckExistanceForHave);
    have_paths.AddFiles(havepaths_opt.begin(), havepaths_opt.end());
    if (!havepathfile.empty()) {
      have_paths.AddFromFilelist(havepathfile);
    }
    std::vector<std::string> already_have;
    RemoveDuplicates(have_paths, already_have);

    // Get the filelists (manifest) from each dbpath.
    // Create temporary directory for the DB and Index manifests.
    const std::string tmp_dir = khCreateTmpDir("gedisconnectsend.");
    if (tmp_dir.empty()) {
      throw khErrnoException(kh::tr("Unable to create temporary directory"));
    }
    // Create a guard object that ensures that the temporary directory gets
    // deleted automatically when we return.
    khCallGuard<const std::string&, bool> prune_guard(khPruneDir, tmp_dir);

    FileMap send_files;
    std::vector<std::vector<std::string> > send_filelists;
    PopulateFileMap(file_pool, &to_send, send_files, &send_filelists,
                    manifest_only ? "" : tmp_dir);
    FileMap have_files;
    PopulateFileMap(file_pool, &already_have, have_files, 0, "");

    // Figure out which of the files I really need to send.
    std::uint64_t totalsize = 0;
    FileList needed_files;

    for (const auto& maybe : send_files) {
      if (have_files.find(maybe.first) == have_files.end()) {
        totalsize += maybe.second.second;
        needed_files.push_back(maybe);
      }
    }

    // Now add the extra files.
    for (const auto& extra : extrafiles) {
      std::uint64_t fsize;
      time_t ftime;
      if (!khGetFileInfo(extra, fsize, ftime)) {
        throw khErrnoException(kh::tr("Unable to get filesize for %1")
                               .arg(extra.c_str()));
      }
      totalsize += fsize;
      needed_files.push_back(std::make_pair(extra, std::make_pair(extra, fsize)));
    }

    if (needed_files.size()) {
      if (manifest_only) {  // Print the files.
        const size_t list_size = needed_files.size();
        for (size_t i = 0; i < list_size; ++i) {
          printf("%s\n", needed_files[i].first.c_str());
        }
        exit(0);
      }
      // Add space for files.list.
      for (size_t i = 0; i < needed_files.size(); ++i) {
        // The file name with '\n' in files.list
        totalsize += (needed_files[i].second.first.size() + 1);
      }
      // Add space for .filelist.
      for (size_t i = 0; i < send_filelists.size(); ++i) {
        for (size_t j = 0; j < send_filelists[i].size(); ++j) {
          totalsize += send_filelists[i][j].size() + 1;  // file_name + '\n'
        }
        totalsize += (khComposePath(to_send[i], ".filelist").size() + 1);
      }
      // Add space for dbpaths.list.
      for (size_t i = 0; i < to_send.size(); ++i) {
          totalsize += to_send[i].size() + 1;  // file_name + '\n'
      }
      // Add space for asset_root.
      totalsize += AssetDefs::AssetRoot().size();
      // Add space for directories.
      {
        std::set<std::string> directories;
        char dir_buff[4096];
        struct stat64 sb;
        for (size_t i = 0; i < needed_files.size(); ++i) {
          strncpy(dir_buff, needed_files[i].second.first.c_str(),
                  sizeof(dir_buff));
          for (char* dir_name = dirname(dir_buff);;
                     dir_name = dirname(dir_name)) {
            const std::string dir_name_str(dir_name);
            if (directories.insert(dir_name_str).second) {
              if (stat64(dir_name, &sb) == 0) {
                totalsize += sb.st_size;
              }
            }
            if (dir_name[1] == 0) {
              break;
            }
          }
        }
      }

      const size_t side_files = to_send.size()  // .filelist
                                + 1             // files.list
                                + 1             // dbpaths.list
                                + 1;            // .assetroot
      const size_t human_readable_size[] = { 1UL, 1024UL,
          1024UL * 1024, 1024UL * 1024 * 1024, 1024UL * 1024 * 1024 * 1024,
          1024UL * 1024 * 1024 * 1024 * 1024 };
      const char   human_readable_char[] = { ' ', 'K', 'M', 'G', 'T', 'P' };
      int human_readable_index = 1;
      for ( ; totalsize > human_readable_size[human_readable_index]; ) {
        ++human_readable_index;
      }
      --human_readable_index;
      size_t totalsize_human_readable = (totalsize +
          human_readable_size[human_readable_index] - 1) /
          human_readable_size[human_readable_index];
      notify(NFY_NOTICE, "Need to send %lu + %lu files, %lu%c (%lu bytes)",
             static_cast<long unsigned>(needed_files.size()), side_files,
             totalsize_human_readable,
             human_readable_char[human_readable_index], totalsize);
      if (report_size_only) {
        exit(0);
      }
      {
        khProgressMeter progress(totalsize, "bytes");
        std::ofstream filelist(khComposePath(output, "files.list").c_str());
        for (FileList::const_iterator file = needed_files.begin();
             file != needed_files.end(); ++file) {
          std::string src = file->first;  // current path
          std::string dest = khComposePath(output, file->second.first);  // orig
          if (!khLinkOrCopyFile(src, dest)) {
            notify(NFY_FATAL, "Unable to proceed");
          }
          filelist << file->second.first << std::endl;
          progress.incrementDone(file->second.second);
        }
      }

      // Add the filelists to each dbpath.
      notify(NFY_NOTICE, "Writing filelists");
      for (unsigned int s = 0; s < to_send.size(); ++s) {
        std::string orig_flist = khComposePath(
            to_send[s], kDbManifestFilesListFile);
        std::string flist = khComposePath(output, orig_flist);
        std::vector<std::string> file_names;
        const size_t num_files = send_filelists[s].size();
        file_names.reserve(num_files + 1);
        for (size_t i = 0; i < num_files; ++i) {
          file_names.push_back(send_filelists[s][i]);
        }
        file_names.push_back(orig_flist);
        khWriteStringVectorToFile(flist, file_names);
      }

      std::ofstream dbpathlist(khComposePath(output, "dbpaths.list").c_str());
      for (std::vector<std::string>::const_iterator s = to_send.begin();
           s != to_send.end(); ++s) {
        dbpathlist << *s << std::endl;
      }

      // Write .assetroot file.
      std::string file_name = khComposePath(output, ".assetroot");
      if (!khWriteStringToFile(file_name, AssetDefs::AssetRoot())) {
        notify(NFY_FATAL, "Unable to write to '%s'", file_name.c_str());
      }
    } else {
      notify(NFY_NOTICE, "Nothing to send\n");
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
