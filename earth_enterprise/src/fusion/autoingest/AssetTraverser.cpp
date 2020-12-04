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


#include "AssetTraverser.h"

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include <deque>

#include <khFileUtils.h>
#include <khGuard.h>
#include <notify.h>
#include <khConstants.h>
#include <khException.h>


class ChdirGuard {
 public:
  ChdirGuard(const std::string &restorepath) : restorepath_(restorepath) { }
  ~ChdirGuard(void) {
    if (::chdir(restorepath_.c_str()) == -1) {
      // we're in a desctructor, so we cannot throw
      // best we can do is log a message
      notify(NFY_WARN, "Unable to chdir to %s: %s",
             restorepath_.c_str(),
             khstrerror(errno).c_str());
    }
  }
 private:
  std::string restorepath_;

};



AssetTraverser::AssetTraverser(void) :
    assetroot_(AssetDefs::AssetRoot())
{

  // flag all known extentions to be ignored
  // we'll later remove the small number we actually want
  for (AssetDefs::Type type = AssetDefs::Vector; type <= AssetDefs::LastType;
       type = AssetDefs::Type(int(type)+1)) {
    IgnoreExtensions.insert(AssetDefs::FileExtension(type, "Source"));
    IgnoreExtensions.insert(AssetDefs::FileExtension(type, kProductSubtype));
    IgnoreExtensions.insert(
        AssetDefs::FileExtension(type, kMercatorProductSubtype));
    IgnoreExtensions.insert(AssetDefs::FileExtension(type, kProjectSubtype));
    IgnoreExtensions.insert(
        AssetDefs::FileExtension(type, kMercatorProjectSubtype));
    IgnoreExtensions.insert(AssetDefs::FileExtension(type, kLayer));
    IgnoreExtensions.insert(AssetDefs::FileExtension(type, kDatabaseSubtype));
    IgnoreExtensions.insert(
        AssetDefs::FileExtension(type, kMapDatabaseSubtype));
    IgnoreExtensions.insert(
        AssetDefs::FileExtension(type, kMercatorMapDatabaseSubtype));
  }
}

AssetTraverser::~AssetTraverser(void)
{
}

void AssetTraverser::RequestType(AssetDefs::Type type,
                                 const std::string &subtype)
{
  IgnoreExtensions.erase(AssetDefs::FileExtension(type, subtype));
  WantExtensions.insert(AssetDefs::FileExtension(type, subtype));
  wanted_.insert(std::make_pair(type, subtype));
}


void AssetTraverser::Traverse(void)
{
  // we have to pre-check the existence of the assetroot because
  // ContinueSearch ignores ENOTDIR failues
  if (!khDirExists(assetroot_)) {
    throw khException(kh::tr("%1 isn't a directory").arg(assetroot_.c_str()));
  }
  std::string cwd = khGetCwd();
  if (cwd.empty()) {
    throw khException(kh::tr("Cannot get current working directory"));
  }

  // open using low level ::open, ContinueSearch needs an fd
  // don't add O_NOFOLLOW since we want to allow symlinks in the toplevel
  // assetroot path, just not any below it.
  int fd = ::open(assetroot_.c_str(), O_LARGEFILE | O_RDONLY);
  if (fd < 0) {
    throw khErrnoException(kh::tr("Unable to open %1")
                           .arg(assetroot_.c_str()));
  }
  khReadFileCloser close_guard(fd);

  ContinueSearch(fd, "", assetroot_, cwd);
}


void AssetTraverser::ContinueSearch(int fd,
                                    const std::string &parent,
                                    const std::string &relpath,
                                    const std::string &restorepath) {
  static const std::string thisdir(".");
  static const std::string parentdir("..");
  static const std::string khassetfile("khasset.xml");
  std::string fulldir = parent + relpath + "/";

  struct stat64 stat_info;
  if (::fstat64(fd, &stat_info) < 0) {
    throw khErrnoException(kh::tr("Unable to get info for %1")
                           .arg(fulldir.c_str()));
  }
  if (!S_ISDIR(stat_info.st_mode)) {
    // we don't check if relpath is a dir before calling this routine
    // because it would just cost us another stat.
    // so if this isn't a dir, it's not an error, we just don't have anything
    // to do
    return;
  }

  // Make sure we don't duplicate work
  DirId dir_id = GetDirId(stat_info);
  if (visited_dirs_.find(dir_id) != visited_dirs_.end()) {
    // we have already visited this dir, short-circuit
    notify(NFY_DEBUG, "SKIPPING %s (revisit)", fulldir.c_str());
    return;
  }
  visited_dirs_.insert(dir_id);


  // try to cd
  if (::fchdir(fd) == -1) {
    throw khErrnoException(kh::tr("Unable to change dir to %1")
                           .arg(fulldir.c_str()));
  }
  ChdirGuard chdir_guard(restorepath);


  notify(NFY_DEBUG, "chdir %s", fulldir.c_str());

  std::deque<std::string> subdirs;
  {
    DIR *dir = ::opendir(".");
    if (!dir) {
      throw khErrnoException(kh::tr("Unable to open dir %1")
                             .arg(fulldir.c_str()));
    }
    khDIRCloser dirguard(DIR);

    struct dirent64 *entry;
    while ((entry = ::readdir64(dir))) {
      std::string fname = entry->d_name;

      // skip . and ..
      if ((fname == thisdir) || (fname == parentdir)) {
        continue;
      }

      // if this dir has khasset.xml, we are already in an asset dir, so we
      // should abort this dir. This normally won't happen unless
      // new types get added that we don't know how to ignore with
      // IgnoreExtensions
      if (fname == khassetfile) {
        return;
      }

      std::string ext = khExtension(fname);

      // is it one we want
      if (WantExtensions.find(ext) != WantExtensions.end()) {
        HandleAssetFile(fulldir, fname);
        continue;
      }

      // is it one we should ignore
      if (IgnoreExtensions.find(ext) != IgnoreExtensions.end()) {
        continue;
      }

      // #ifdef _DIRENT_HAVE_D_TYPE

      // if the directory entry stores the filetype use it to short circuit
      if ((entry->d_type != DT_UNKNOWN) &&
          (entry->d_type != DT_DIR)) {
        continue;
      }

      // #endif

      subdirs.push_back(fname);
    }
  }

  for (const auto& dir : subdirs) {
    bool is_symlink = false;

    // open using low level ::open
    // we need to stat and chdir. Doing the raw open once, we can pass the fd
    // to fstate and fchdir and save one set of filename traversals
    int fd = ::open(dir.c_str(),
                    O_LARGEFILE | O_RDONLY | O_NOFOLLOW);
    if ((fd < 0) && (errno == ELOOP)) {
      is_symlink = true;
      fd = ::open(dir.c_str(),
                  O_LARGEFILE | O_RDONLY);
    }

    if (fd < 0) {
      throw khErrnoException(kh::tr("Unable to open %1")
                             .arg(std::string(fulldir + dir).c_str()));
    }
    khReadFileCloser close_guard(fd);

    ContinueSearch(fd, fulldir, dir,
                   is_symlink ? fulldir : parentdir);
  }
}


void AssetTraverser::HandleAssetFile(const std::string &dir,
                                     const std::string &fname)
{
  std::string assetref = AssetDefs::FilenameToAssetPath(dir+fname);
  Asset asset(assetref);

  if (!asset) {
    throw khException(kh::tr("Unable to load asset %1").arg(assetref.c_str()));
  }

  if (wanted_.find(std::make_pair(asset->type, asset->subtype)) !=
      wanted_.end()) {
    HandleAsset(asset);
  } else {
    notify(NFY_DEBUG, "IGNORING %s (type/subtype)", assetref.c_str());
  }
}

AssetTraverser::DirId AssetTraverser::GetDirId(const struct stat64 &stat_info) {
  return std::make_pair(stat_info.st_dev, stat_info.st_ino);
}
