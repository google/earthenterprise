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


#include <sstream>
#include <khConstants.h>
#include <khGetopt.h>
#include <khSpawn.h>
#include <notify.h>
#include <fusion/autoingest/AssetVersion.h>
#include <dbgen/.idl/DBConfig.h>
#include <geindex/IndexBundle.h>
#include <geindex/IndexBundleReader.h>
#include <khEndian.h>
#include <filebundle/filebundlereader.h>
#include "common/khException.h"

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
    " Gather all the files necessary to create standalone dbs.\n"
    " This is useful for default fusion backgrounds, default server dbs,\n"
    " and shipping prebuilt dbs to customers.\n"
    " Options:\n"
    "  --help             Display this usage message.\n"
    "  --dbver <newname>=<dbverref>    [Required]\n"
    "                     Can be specified multiple times.\n"
    "                     <dbverref> - Which Database ver to gather.\n"
    " e.g /gevol/assets/db/test.kdatabase/gedb.kda/ver074/gedb\n"
    " e.g db/test.kdatabase (for latest version)\n"
    " e.g 'db/test.kdatabase?version=74' (for 74th version, quoting for ?)\n"
    "                     <newname>  - where to put it under <output>.\n"
    "  --output <dirname> Where to gather the files. [required]\n"
    "                     The directory must already exists and be empty.\n"
    "\n",
          progn.c_str());
  exit(1);
}


typedef std::map<std::string, std::string> StringMap;
typedef StringMap::const_iterator StringMapIterator;
void CopyGEDB(geFilePool &file_pool,
              const std::string &outdir, const std::string &newname,
              const std::string &oldpath);
void FixupBundleOrigPaths(geFilePool &file_pool,
                          const std::string &newindexpath);

int
main(int argc, char *argv[]) {

  try {
    std::string progname = argv[0];

    // process commandline options
    int argn;
    bool help = false;
    std::map<std::string, std::string> dbvers;
    std::string output;
    khGetopt options;
    options.flagOpt("help", help);
    options.mapOpt("dbver", dbvers);
    options.opt("output", output, &khGetopt::DirExists);
    options.setRequired("dbver", "output");

    if (!options.processAll(argc, argv, argn) || help) {
      usage(progname);
    }

    // verify that the output directory is empty
    if (khCountFilesInDir(output)) {
      notify(NFY_FATAL, "%s must be empty", output.c_str());
    }

    geFilePool file_pool;

    // normalize the dbvers, find their gedb paths, and call to have
    // them copied
    for (StringMapIterator i = dbvers.begin(); i != dbvers.end(); ++i) {
      const std::string& database_name = i->second;
      std::string gedb_path;
      if (!AssetVersionImpl::GetGedbPath(database_name, &gedb_path)) {
        notify(NFY_FATAL, "Unable to interpret DB '%s'.",
               database_name.c_str());
      }

      CopyGEDB(file_pool, output, i->first, gedb_path);
    }

    // prune cache.pack files
    std::ostringstream out;
    out << "find " << output << "/.files/packets "
        << "-name cache.pack -exec rm -rf '{}' \\; -prune";
    (void)system(out.str().c_str());

    std::vector<std::string> packetdirs;
    khFindFilesInDir(khComposePath(output, ".files/packets"),
                     packetdirs);
    for (std::vector<std::string>::const_iterator p = packetdirs.begin();
         p != packetdirs.end(); ++p) {
      FixupBundleOrigPaths(file_pool, *p);
    }

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}


void Cp(const std::string &src, const std::string &dest) {
  CmdLine cmd;
  if (!khEnsureParentDir(dest)) {
    // more specific message already emitted
    throw khException(kh::tr("Unable to ensure parent dir for %1")
                      .arg(dest.c_str()));
  }
  cmd << "cp" << "-r" << src << dest;
  if (!cmd.System()) {
    // more specific message already emitted
    throw khException(kh::tr("Unable to cp -r %1 %2")
                      .arg(src.c_str()).arg(dest.c_str()));
  }
}




void CopyFilelistPreserveBasename(std::vector<std::string> &filelist,
                                  const std::string &relpath,
                                  const std::string &newpath) {
  for (std::vector<std::string>::iterator file = filelist.begin();
       file != filelist.end(); ++file) {
    std::string basename = khBasename(*file);
    std::string newfile = khComposePath(newpath, basename);
    std::string relfile = khRelativePath(relpath, newfile);
    Cp(*file, newfile);
    *file = relfile;
  }
}

void CopyFilelistNewNames(std::vector<std::string> &filelist,
                          const std::string &relpath,
                          const std::string &newpath,
                          StringMap &seenfiles) {
  std::string basename = khBasename(newpath);
  for (std::vector<std::string>::iterator file = filelist.begin();
       file != filelist.end(); ++file) {
    if (!file->empty()) {
      StringMapIterator found = seenfiles.find(*file);
      std::string newfile;
      if (found == seenfiles.end()) {
        newfile =
          khComposePath(newpath, khMakeNumericFilename(basename,
                                                       seenfiles.size()));
        seenfiles[*file] = newfile;
        Cp(*file, newfile);
      } else {
        newfile = found->second;
      }

      std::string relfile = khRelativePath(relpath, newfile);
      *file = relfile;
    }
  }
}


void CopyGEDB(geFilePool &file_pool,
              const std::string &outdir, const std::string &newname,
              const std::string &oldpath) {
  static unsigned int indexcount = 0;
  static StringMap iconmap;
  static StringMap packetmap;
  static StringMap poimap;

  std::string newpath    = khComposePath(outdir, newname);
  std::string tocpath    = khComposePath(newpath, "tocs");
  std::string indexpath  = khComposePath(outdir,  ".files/indexes");
  std::string iconpath   = khComposePath(outdir,  ".files/icons");
  std::string packetpath = khComposePath(outdir,  ".files/packets");
  std::string poipath    = khComposePath(outdir,  ".files/pois");
  khMakeDirOrThrow(newpath);
  khMakeDirOrThrow(tocpath);
  khMakeDirOrThrow(indexpath);
  khMakeDirOrThrow(iconpath);
  khMakeDirOrThrow(packetpath);
  khMakeDirOrThrow(poipath);

  DbHeader header;
  if (!header.Load(khComposePath(oldpath, kHeaderXmlFile))) {
    // more specific message already emitted
    throw khException(kh::tr("Unable to load old header"));
  }


  // handle index - it's special
  {
    // copy the index dir
    std::string newindexpath =
      khComposePath(indexpath,
                    khMakeNumericFilename("index", indexcount++));
    Cp(header.index_path_, newindexpath);
    header.index_path_ = khRelativePath(newpath, newindexpath);

    // copy the dated_channels_map (if it exists)
    std::string timemachine_dated_channels_map_path =
      khComposePath(header.index_path_, kDatedImageryChannelsFileName);

    // load the new index header - copy all referenced packetfiles
    {
      geindex::IndexBundleReader reader(file_pool, newindexpath);
      CopyFilelistNewNames(reader.header.PacketfileVectorRef(),
                           newindexpath, packetpath, packetmap);

      // Write modified index header
      LittleEndianWriteBuffer buf;
      buf << reader.header;
      file_pool.WriteSimpleFileWithCRC(
          geindex::Header::HeaderPath(newindexpath), buf);
    }

    // fixup the orig_paths in the index filebundle
    FixupBundleOrigPaths(file_pool, newindexpath);
  }

  // handle other file lists
  CopyFilelistPreserveBasename(header.toc_paths_, newpath, tocpath);
  CopyFilelistNewNames(header.poi_file_paths_, newpath, poipath, poimap);
  CopyFilelistNewNames(header.icons_dirs_, newpath, iconpath, iconmap);


  if (!header.Save(khComposePath(newpath, kHeaderXmlFile))) {
    // more specific message already emitted
    throw khException(kh::tr("Unable to save new header"));
  }
}


void FixupBundleOrigPaths(geFilePool &file_pool,
                          const std::string &path) {
  FileBundleReader bundle_reader(file_pool, path);
  bundle_reader.ClearOrigPaths();
  LittleEndianWriteBuffer buffer;
  bundle_reader.BuildHeaderWriteBuffer(buffer);
  file_pool.WriteSimpleFileWithCRC(bundle_reader.HeaderPath(), buffer, 0444);
}
