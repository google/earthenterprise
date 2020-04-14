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



#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <notify.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <linux/unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/khGuard.h"
#include "common/khMisc.h"
#include "common/khSimpleException.h"
#include "common/khStringUtils.h"
#include "common/khstl.h"

#undef ENABLE_GFS_HACKS


#ifdef ENABLE_GFS_HACKS
#include "common/khSpawn.h"

inline bool
IsGFSPath(const std::string &path)
{
  return ((path.length() >= 5) &&
          (memcmp(&path[0], "/gfs/", 5) == 0));
}
#endif


bool khHasExtension(const std::string &filename,
                    const std::string &ext)
{
  return EndsWith(filename, ext);
}

std::string khEnsureExtension(const std::string &filename,
                              const std::string &ext)
{
  return EndsWith(filename, ext) ? filename : filename + ext;
}
std::string khComposePath(const std::string &dir, const std::string &file) {
  if (file.empty()) {
    return dir;
  } else if (dir.empty()) {
    return file;
  }

  static const std::string sep = "/";
  if (EndsWith(dir, sep) || StartsWith(file, sep)) {
    return dir + file;
  } else {
    return dir + sep + file;
  }
}

std::string khComposePath(const std::string &parentdir,
                          const std::string &childdir,
                          const std::string &file) {
  return khComposePath(khComposePath(parentdir, childdir),
                       file);
}

// Compose time machine folders as "YYYY/MM/DD" if MILLISECONDS == 0
// or "YYYY/MM/DD/MILLISECONDS" if MILLISECONDS > 0.
std::string khComposeTimePath(const struct tm& time) {
  const std::string sep = "/";
  const char fill = '0';
  std::stringstream ss;
  // Definition of "tm_year" is offset since 1900.
  ss << std::setfill(fill) << std::setw(4) << (time.tm_year + 1900) << sep;
  // Definition of "tm_mon" starts with "0".
  ss << std::setfill(fill) << std::setw(2) << (time.tm_mon + 1) << sep;
  ss << std::setfill(fill) << std::setw(2) << time.tm_mday;

  // Maintain the compatiability with older version: if milliseconds is 0,
  // then don't use the folder representing milliseconds.
  std::uint32_t msFromMidnight = MillisecondsFromMidnight(time);
  if (msFromMidnight) {
    ss << sep << msFromMidnight;
  }

  return ss.str();
}

std::string
khBasename(const std::string &filename)
{
  size_t buflen = filename.size()+1;
  char buf[buflen];
  strncpy(buf, filename.c_str(), buflen);
  return basename(buf);
}

std::string
khDirname(const std::string &filename)
{
  int buflen = filename.size()+1;
  char buf[buflen];
  strncpy(buf, filename.c_str(), buflen);
  return dirname(buf);
}

std::string khExtension(const std::string &filename, bool includeDot) {
  std::string::size_type dot = filename.rfind('.');
  if (dot != std::string::npos) {
    if (includeDot) {
      return filename.substr(dot);
    } else if (dot+1 < filename.size()) {
      return filename.substr(dot+1);
    }
  }
  return std::string();
}


std::string
khDropExtension(const std::string &filename)
{
  std::string::size_type dot = filename.rfind('.');
  if (dot != std::string::npos) {
    // we found a '.'
    if (filename.find("/", dot+1) == std::string::npos) {
      // the '.' doesn't belong to a parent dir
      return filename.substr(0, dot);
    }
  }
  return filename;
}

std::string
khReplaceExtension(const std::string &filename, const std::string &newExt)
{
  std::string::size_type dot = filename.rfind('.');
  if (dot != std::string::npos) {
    return filename.substr(0, dot)+newExt;
  } else {
    return filename + newExt;
  }
}

bool
khIsAbspath(const std::string &filename)
{
  if (filename.empty()) {
    return false;
  }
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || \
  defined(_WIN64) || defined(__WIN64__) || defined(WIN64) || \
  defined(__MINGW32__) || defined(__MINGW64__)
  return ((isalpha((unsigned char)filename[0]) && filename[1] == ':' &&
           (filename[2] == '/' || filename[2] == '\\')) ||
          (filename[0] == '\\' && filename[1] == '\\'));
#else
  return (filename[0] == '/');
#endif
}


std::string khAbspath(const std::string &filename)
{
  if (filename[0] != '/') {
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX)) {
      return std::string(cwd) + '/' + filename;
    } else {
      notify(NFY_WARN, "Unable to getcwd. Leaving filename relative.");
    }
  }
  return filename;
}

std::string
khNormalizeDir(const std::string &dirname_,
               bool add_trailing_slash)
{
  // degenerate case - don't barf on it
  if (dirname_.empty()) return dirname_;

  // make a copy we can futz with
  std::string dirname = dirname_;

  // make sure it ends with a '/'
  if (dirname[dirname.size()-1] != '/') {
    dirname += '/';
  }

  // fix all double '/' by erasing the first one
  std::string::size_type pos = 0;
  while ((pos = dirname.find("//", pos)) != std::string::npos) {
    dirname.erase(pos, 1);
  }

  // remove all /./ - replace with /
  pos = 0;
  while ((pos = dirname.find("/./", pos)) != std::string::npos) {
    dirname.erase(pos, 2);
  }

  // remove embedded /../
  pos = 0;
  while ((pos = dirname.find("/../", pos)) != std::string::npos) {
    if (pos == 0) {
      // /../ at the beginning of a path is entirely redundant
      // convert it to / by deleteing the first three characters
      dirname.erase(0, 3);
    } else {
      // we found one in the middle so lets see if we can find the
      // preceeding dirname like foo/../ or /foo/../

      // find the previous '/'
      std::string::size_type ppos = dirname.rfind('/', pos-1);
      if (ppos == std::string::npos) {
        // we're at the front of the string "foo/../aaa"
        ppos = 0;
      } else {
        // we're embedded inside somewhere "aaaaaaa/foo/../aaa"
        ppos = ppos + 1;
      }

      // check if the previous dir is ".."  This can only happen if
      // the previous ".." wasn't able to reduce because it is at the
      // front. Or follows another that was at the front.
      // like "../../aaaa" or "../../../aaaa"
      if (stdstrcompare(dirname, ppos, pos - ppos, "..") == 0) {
        // skip this /../
        pos += 3;
      } else {
        // erase the foo/../
        dirname.erase(ppos, pos - ppos + 4);
        // we need to start looking with the previous '/'
        // that we'd found. But we may have hit the beginning,
        // at which point we just start there
        pos = (ppos == 0) ? 0 : ppos - 1;
      }
    }
  }

  // prune trailing '/' if caller doesn't want it
  if ((dirname.size() > 1) && !add_trailing_slash) {
    dirname.resize(dirname.size()-1);
  }

  return dirname;
}


std::string
khRelativePath(const std::string &absfrom, const std::string &absto)
{
  if (absfrom.empty() || absto.empty() ||
      (absfrom[0] != '/') || (absto[0] != '/')) {
    //      notify(NFY_WARN, "khRelativeDir: invalid arguments");
    return std::string();
  }

  // nomalize the input dirs and split them into path components
  std::vector<std::string> from;
  std::vector<std::string> to;
  split(khNormalizeDir(absfrom), "/", back_inserter(from));
  split(khNormalizeDir(absto), "/", back_inserter(to));

  // remove final empty string from both lists
  // this gets inserted since khNormalizeDir makes sure that
  // the string ends with '/'
  from.pop_back();
  to.pop_back();

  // find the common prefix. This will also take care of the leading
  // empty string in both that came from the leading '/' in the filenames
  unsigned int fromLen = from.size();
  unsigned int toLen   = to.size();
  unsigned int prefixLen = 0;
  while ((prefixLen < fromLen) && (prefixLen < toLen) &&
         (from[prefixLen] == to[prefixLen])) {
    ++prefixLen;
  }

  // figure out how much space to reserve ( no sense thrashing memory )
  std::string relpath;
  unsigned int numback = fromLen - prefixLen;
  unsigned int rellen = numback * 3;
  for (unsigned int i = prefixLen; i < toLen; ++i) {
    rellen += to[i].size() + 1;
  }
  relpath.reserve(rellen);

  // build the relpath
  // if both paths were the same, relpath will end up empty (which is ok)
  for (unsigned int i = 0; i < numback; ++i) {
    relpath += "../";
  }
  for (unsigned int i = prefixLen; i < toLen; ++i) {
    relpath += to[i];
    relpath += '/';
  }

  // trim trailing '/'
  if (relpath.size() > 1) {
    relpath.resize(relpath.size()-1);
  }

  return relpath;
}


bool
khExists(const std::string &filename)
{
  struct stat64 sb;
  return (stat64(filename.c_str(), &sb) == 0);
}


bool khIsExecutable(const std::string &filename) {
  struct stat64 sb;
  if (stat64(filename.c_str(), &sb) == 0) {
    return (S_IXUSR & sb.st_mode);
  }
  return false;
}


bool
khIsFileReadable(const std::string &filename)
{
  FILE* fp = NULL;
  fp = fopen64(filename.c_str(), "r");
  if (fp != NULL) {
    fclose(fp);
    return true;
  }
  return false;
}

bool
khDirExists(const std::string &dirname)
{
  struct stat64 sb;
  return ((stat64(dirname.c_str(), &sb) == 0) && S_ISDIR(sb.st_mode));
}

bool
khSymlinkExists(const std::string &filename)
{
  struct stat64 sb;
  return ((lstat64(filename.c_str(), &sb) == 0) && S_ISLNK(sb.st_mode));
}

void khFindFilesInDir(const std::string &dirname,
                      std::vector<std::string> &files_return,
                      const std::string &ext)
{
  bool check_ext = !ext.empty();

  DIR *dir = opendir(dirname.c_str());
  if (!dir) {
    // dir doesn't exist, no files can either
    return;
  }
  struct dirent64 *entry;
  while ((entry = readdir64(dir))) {
    std::string dname = entry->d_name;
    if (dname == ".") continue;
    if (dname == "..") continue;
    if (check_ext && !khHasExtension(dname, ext)) continue;
    files_return.push_back(khComposePath(dirname, dname));
  }
  closedir(dir);
}

void khFindBasenamesInDir(const std::string &dirname,
                          const std::string &ext,
                          std::vector<std::string> *files_return) {
  bool check_ext = !ext.empty();

  DIR *dir = opendir(dirname.c_str());
  if (!dir) {
    // dir doesn't exist, no files can either
    return;
  }
  struct dirent64 *entry;
  while ((entry = readdir64(dir))) {
    std::string dname = entry->d_name;
    if (dname == ".") continue;
    if (dname == "..") continue;
    if (check_ext && !khHasExtension(dname, ext)) continue;
    files_return->push_back(dname);
  }
  closedir(dir);
}

 std::uint32_t khCountFilesInDir(const std::string &dirname) {
  std::uint32_t count = 0;
  DIR *dir = opendir(dirname.c_str());
  if (dir) {
    struct dirent64 *entry;
    while ((entry = readdir64(dir))) {
      std::string dname = entry->d_name;
      if (dname == ".") continue;
      if (dname == "..") continue;
      ++count;
    }
    closedir(dir);
  }
  return count;
}


bool
khReadSymlink(const std::string &filename, std::string &target)
{
  char buf[PATH_MAX+1];
  int ret = readlink(filename.c_str(), buf, PATH_MAX);
  if (ret < 0) {
    notify(NFY_WARN, "Unable to readlink(%s): %s", filename.c_str(),
           khstrerror(errno).c_str());
    return false;
  }

  // null terminate
  buf[ret] = 0;

  target = buf;
  return true;
}

const std::vector<std::string> fileutilArgs =
makevec3(std::string("--nobinarylog"),
         std::string("--minloglevel=2"),
         std::string("--logtostderr"));


#ifdef ENABLE_GFS_HACKS
bool
khGFSPruneDir(const std::string &dirname)
{
  CmdLine cmdline;
  cmdline << "fileutil" << fileutilArgs << "rm" << "-R" << "-f" << dirname;
  if (!cmdline.System()) {
    // We don't (and can't) know what the error was. So just assume it was
    // a permissions problem. 90+% of the time that's what it will be
    // anyway. Hopefully fileutil will emit a message that a user could
    // track down in a log file somewhere.
    errno = EPERM;
    return false;
  }
  return true;
}
#endif


bool
khPruneDir(const std::string &dirname)
{
  if (!khDirExists(dirname)) return false;
  if (khSymlinkExists(dirname)) {
    return khUnlink(dirname);
  } else {
#ifdef ENABLE_GFS_HACKS
    if (IsGFSPath(dirname)) {
      return khGFSPruneDir(dirname);
    }
#endif

    DIR *dir = opendir(dirname.c_str());
    if (!dir) {
      notify(NFY_WARN, "Unable to opendir(%s): %s", dirname.c_str(),
             khstrerror(errno).c_str());
      return false;
    }
    struct dirent64 *entry;
    while ((entry = readdir64(dir))) {
      if (strcmp(entry->d_name, ".") == 0) continue;
      if (strcmp(entry->d_name, "..") == 0) continue;
      std::string child = dirname + "/" + entry->d_name;
      if (khDirExists(child)) {
        if (!khPruneDir(child)) {
          closedir(dir);
          return false;
        }
      } else {
        if (unlink(child.c_str()) != 0) {
          notify(NFY_WARN, "Unable to unlink(%s): %s", child.c_str(),
                 khstrerror(errno).c_str());
          closedir(dir);
          return false;
        }
      }
    }
    closedir(dir);
    int ret = rmdir(dirname.c_str());
    if (ret < 0) {
      notify(NFY_WARN, "Unable to rmdir(%s): %s", dirname.c_str(),
             khstrerror(errno).c_str());
      return false;
    } else {
      return true;
    }
  }
}

bool
khMakeCleanDir(const std::string &dirname, mode_t mode)
{
  if (khDirExists(dirname) && !khPruneDir(dirname)) {
    return false;
  }
  return khMakeDir(dirname, mode);
}


bool
khPruneFileOrDir(const std::string &path)
{
  if (khDirExists(path)) {
    return khPruneDir(path);
  } else {
    return khUnlink(path);
  }
}


bool
khMakeDir(const std::string &dirname, mode_t mode)
{
  try {
    khMakeDirOrThrow(dirname, mode);
    return true;
  } catch (const std::exception &e) {
    if (khDirExists(dirname)) {
      // If we are getting this warning, we could consider returning true here
      // since the directory exists and everything is ok.
      notify(NFY_WARN, "MkDir: Directory %s already exists.", dirname.c_str());
    }
    notify(NFY_WARN, "%s", e.what());
  }
  return false;
}

void
khMakeDirOrThrow(const std::string &dirname, mode_t mode)
{
  if (!khDirExists(dirname)) {
    std::string parent = khDirname(dirname);
    if (!khDirExists(parent)) {
      khMakeDirOrThrow(parent, mode);
    }
    int ret = mkdir(dirname.c_str(), mode);
    if (ret < 0) {
      throw khSimpleErrnoException("Unable to mkdir ") << dirname;
    }
  }
}

bool
khChmod(const std::string &filename, mode_t mode)
{
  int ret = chmod(filename.c_str(), mode);
  if (ret < 0) {
    notify(NFY_WARN, "Unable to chmod(%s): %s", filename.c_str(),
           khstrerror(errno).c_str());
    return false;
  } else {
    return true;
  }
}

bool
khChangeDir(const std::string &filename)
{
  int ret = ::chdir(filename.c_str());
  if (ret < 0) {
    notify(NFY_WARN, "Unable to chdir(%s): %s", filename.c_str(),
           khstrerror(errno).c_str());
    return false;
  } else {
    return true;
  }
}



#ifdef ENABLE_GFS_HACKS
bool
khGFSRename(const std::string &oldname, const std::string &newname)
{
  CmdLine cmdline;
  cmdline << "fileutil" << fileutilArgs << "mv" << oldname << newname;
  if (!cmdline.System()) {
    // We don't (and can't) know what the error was. So just assume it was
    // a permissions problem. 90+% of the time that's what it will be
    // anyway. Hopefully fileutil will emit a message that a user could
    // track down in a log file somewhere.
    errno = EPERM;
    return false;
  }
  return true;
}
#endif


bool
khRename(const std::string &oldname, const std::string &newname)
{
#ifdef ENABLE_GFS_HACKS
  if (IsGFSPath(oldname) || IsGFSPath(newname)) {
    return khGFSRename(oldname, newname);
  }
#endif
  int ret = rename(oldname.c_str(), newname.c_str());
  if (ret < 0) {
    notify(NFY_WARN, "Unable to rename(%s -> %s): %s",
           oldname.c_str(), newname.c_str(), khstrerror(errno).c_str());
    return false;
  } else {
    return true;
  }
}



bool
khMove(const std::string &oldname, const std::string &newname)
{
#ifdef ENABLE_GFS_HACKS
  if (IsGFSPath(oldname) || IsGFSPath(newname)) {
    return khGFSRename(oldname, newname);
  }
#endif
  int ret = rename(oldname.c_str(), newname.c_str());
  if (errno == EXDEV) {
    // oldpath and newpath are not on the same mounted file system.
    // Use khCopyFile and (iff it succeeds) khUnlink instead.
    if (khCopyFile(oldname, newname)) {
      return khUnlink(oldname);
    } else {
      return false;
    }
  } else if (ret < 0) {
    notify(NFY_WARN, "Unable to rename(%s -> %s): %s",
           oldname.c_str(), newname.c_str(), khstrerror(errno).c_str());
    return false;
  } else {
    return true;
  }
}



#ifdef ENABLE_GFS_HACKS
bool
khGFSUnlink(const std::string &path)
{
  CmdLine cmdline;
  cmdline << "fileutil" << fileutilArgs << "rm" << "-f" << path;
  if (!cmdline.System()) {
    // We don't (and can't) know what the error was. So just assume it was
    // a permissions problem. 90+% of the time that's what it will be
    // anyway. Hopefully fileutil will emit a message that a user could
    // track down in a log file somewhere.
    errno = EPERM;
    return false;
  }
  return true;
}
#endif

bool
khUnlink(const std::string &path)
{
#ifdef ENABLE_GFS_HACKS
  if (IsGFSPath(path)) {
    return khGFSUnlink(path);
  }
#endif
  int ret = unlink(path.c_str());
  if (ret < 0) {
    notify(NFY_WARN, "Unable to unlink(%s): %s",
           path.c_str(), khstrerror(errno).c_str());
    return false;
  } else {
    return true;
  }
}

bool
khReplace(const std::string &targetfile, const std::string &newsuffix,
          const std::string &backupsuffix)
{
  std::string newfile    = targetfile + newsuffix;
  std::string backupfile = targetfile + backupsuffix;

  if (!khExists(newfile)) {
    notify(NFY_WARN, "%s doesn't exist. Can't rename to %s",
           newfile.c_str(), targetfile.c_str());
    return false;
  }

  if (khExists(targetfile)) {
    if (!khExists(backupfile) || khPruneFileOrDir(backupfile)) {
      if (khRename(targetfile, backupfile)) {
        if (khRename(newfile, targetfile)) {
          return true;
        } else {
          // put the original targetfile back before we error out
          khRename(backupfile, targetfile);
        }
      }
    }
  } else {
    if (khExists(backupfile)) {
      // Since we didn't make the backup, get rid of any existing one.
      // Otherwise a rollback might stick it back and that would just be
      // wrong.
      (void)khPruneFileOrDir(backupfile);
    }
    if (khRename(newfile, targetfile)) {
      return true;
    }
  }

  // everything should be back to how it was
  // with the exception that the backupfile may be irrevocably lost
  return false;
}

bool
khEnsureParentDir(const std::string &filename, mode_t mode)
{
  std::string parent = khDirname(filename);
  return (khDirExists(parent) || khMakeDir(parent, mode));
}

std::string khTmpDirPath()
{
  // TODO: consider replacing with tempnam(NULL, NULL)
  std::string tmpDir;
  char *tmpdirstr = getenv("TMPDIR");
  if (tmpdirstr && khDirExists(tmpdirstr)) {
    tmpDir = tmpdirstr;
  } else if (P_tmpdir) {
    tmpDir = P_tmpdir;  // from stdio.h
  } else {
    tmpDir = "/tmp";
  }

  return tmpDir;
}

std::string
khTmpFilename(const std::string &basename, mode_t mode)
{
  std::string fname(basename);
  if (fname.empty()) {
    fname = khTmpDirPath() + "/khtmp.";
  }
  khEnsureParentDir(fname);

  // must make a writable copy for mkstemp to play with
  size_t bufsize = fname.size() + 7;
  char tmpname[bufsize];
  snprintf(tmpname, bufsize, "%sXXXXXX", fname.c_str());
  int fd = mkstemp(tmpname);

  if (fd == -1) {
    return std::string();
  } else {
    (void) fchmod(fd, mode);
    (void) close(fd);
    return tmpname;
  }
}


std::string
khCreateTmpDir(const std::string &prefix)
{
  std::string dirname = khTmpDirPath() + "/" + prefix;

  // must make a writable copy for mkstemp to play with
  size_t bufsize = dirname.size() + 7;
  char tmpname[bufsize];
  snprintf(tmpname, bufsize, "%sXXXXXX", dirname.c_str());
  char *result = mkdtemp(tmpname);

  if (result == NULL) {
    return std::string();
  } else {
    return tmpname;
  }
}


void
khGetNumericFilenames(const std::string &pattern,
                      std::vector<std::string>& out)
{
  std::string dirname = khDirname(pattern);
  std::string basename = khBasename(pattern) + ".";

  DIR *dir = opendir(dirname.c_str());
  if (!dir) {
    notify(NFY_WARN, "Unable to opendir(%s): %s", dirname.c_str(),
           khstrerror(errno).c_str());
    return;
  }
  khDIRCloser closer(dir);

  std::map<unsigned int, std::string> files;
  struct dirent64 *entry;
  while ((entry = readdir64(dir))) {
    std::string fname(entry->d_name);
    if (StartsWith(fname, basename)) {
      if (fname.size() > basename.size()) {
        std::string ext = fname.substr(basename.length(),
                                       std::string::npos);
        if (ext.find_first_not_of("0123456789") == std::string::npos) {
          // the extension is all numeric, remember it
          files.insert(make_pair(atoi(ext.c_str()), fname));
        }
      }
    }
  }

  // copy the full filenames to the out vector
  for (std::map<unsigned int, std::string>::const_iterator f = files.begin();
       f != files.end(); ++f) {
    out.push_back(dirname + "/" + f->second);
  }
}

// In future, may want to do this with real regex matching, but currently not
// required.
void
khGetFilenamesMatchingPattern(const std::string& prefix,
                              const std::string& suffix,
                              std::vector<std::string>& out)
{
  std::string dirname = khDirname(prefix);
  std::string basename = khBasename(prefix);

  DIR *dir = opendir(dirname.c_str());
  if (!dir) {
    notify(NFY_WARN, "Unable to opendir(%s): %s", dirname.c_str(),
           khstrerror(errno).c_str());
    return;
  }
  khDIRCloser closer(dir);

  std::map<unsigned int, std::string> files;
  struct dirent64 *entry;
  while ((entry = readdir64(dir))) {
    std::string fname(entry->d_name);
    // For matches, copy the full filenames to the out vector.
    if (StartsWith(fname, basename) && EndsWith(fname, suffix)) {
      out.push_back(dirname + "/" + fname);
    }
  }
}

// In future, may want to do this with real regex matching,
// but currently not required.
void khGetBasenamesMatchingPattern(const std::string &dirname,
                                   const std::string& prefix,
                                   const std::string& suffix,
                                   std::vector<std::string>* out) {
  DIR *dir = opendir(dirname.c_str());
  if (!dir) {
    notify(NFY_WARN, "Unable to opendir(%s): %s", dirname.c_str(),
           khstrerror(errno).c_str());
    return;
  }
  khDIRCloser closer(dir);

  std::map<unsigned int, std::string> files;
  struct dirent64 *entry;
  while ((entry = readdir64(dir))) {
    std::string fname(entry->d_name);
    // For matches, copy the full filenames to the out vector.
    if (StartsWith(fname, prefix) && EndsWith(fname, suffix)) {
      out->push_back(fname);
    }
  }
}

std::string
khMakeNumericFilename(const std::string &base, std::uint64_t num, unsigned int numPlaces)
{
  std::ostringstream out;
  out << base << '.' << std::setw(numPlaces) << std::setfill('0') << num;
  return out.str();
}



bool
khIsURI(const std::string &filename)
{
  // ***********************************************
  // *** we're looking for ^[a-zA-Z][a-zA-Z0-9+-.]*:
  // ***********************************************

  // skip for leading alpha
  if (filename.empty() || !isalpha(filename[0]))
    return false;

  // skip rest of chars that are valid in a URI scheme
  unsigned int i = 1;
  for (; ((i < filename.size()) &&
          (isalpha(filename[i]) ||
           isdigit(filename[i]) ||
           (filename[i] == '+') ||
           (filename[i] == '-') ||
           (filename[i] == '.'))); ++i) {}

  // check for : immediately after the scheme
  return ((i < filename.size()) && (filename[i] == ':'));
}


bool
khSymlink(const std::string &oldpath, const std::string &newpath)
{
  if (khEnsureParentDir(newpath)) {
    (void) ::unlink(newpath.c_str());
    int ret = ::symlink(oldpath.c_str(), newpath.c_str());
    if (ret < 0) {
      notify(NFY_WARN, "Unable to symlink(%s -> %s): %s",
             newpath.c_str(), oldpath.c_str(), khstrerror(errno).c_str());
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

bool
khLink(const std::string &oldpath, const std::string &newpath, bool silent)
{
  if (khEnsureParentDir(newpath)) {
    (void) ::unlink(newpath.c_str());
    int ret = ::link(oldpath.c_str(), newpath.c_str());
    if (ret < 0) {
      if (!silent) {
        notify(NFY_WARN, "Unable to link(%s -> %s): %s",
               newpath.c_str(), oldpath.c_str(),
               khstrerror(errno).c_str());
      }
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}



 std::uint64_t khGetFileSizeOrThrow(const std::string &fname) {
  struct stat64 sb;
  if (stat64(fname.c_str(), &sb) != 0) {
    throw khSimpleErrnoException("Unable to get file size for ")
      << fname;
  }
  return sb.st_size;
}

bool
khGetFileInfo(const std::string &fname, std::uint64_t &size, time_t &mtime)
{
  struct stat64 sb;
  if (stat64(fname.c_str(), &sb) == 0) {
    size = sb.st_size;
    mtime = sb.st_mtime;
    return true;
  } else {
    return false;
  }
}

bool
khGetFileInfo(const int fd, std::uint64_t &size, time_t &mtime) {
  struct stat64 sb;
  if (fstat64(fd, &sb) == 0) {
    size = sb.st_size;
    mtime = sb.st_mtime;
    return true;
  } else {
    return false;
  }
}

std::uint64_t
khDiskUsage(const std::string& path) {
  std::uint64_t size = 0;
  struct stat64 sb;
  if (stat64(path.c_str(), &sb) == 0) {
    size += sb.st_size;
    // For directories, must traverse downward...skip symlinks though.
    if (S_ISDIR(sb.st_mode) && !S_ISLNK(sb.st_mode)) {
      DIR *dir = opendir(path.c_str());
      if (dir) {
        khDIRCloser closer(dir);
        struct dirent64 *entry;
        while ((entry = readdir64(dir))) {
          std::string dname = entry->d_name;
          if (dname == "." || dname == "..") continue;
          std::string fname(khComposePath(path, dname));
          // recursive call
          size += khDiskUsage(fname);
        }
      }
    }
  }
  return size;
}

 std::uint32_t khDiskUsage32(const std::string& path) {
  std::uint64_t size = khDiskUsage(path);
  return static_cast<std::uint32_t>(size);
}

// Given an input "size" of bytes create a double precision value for the
// size, a suffix and precision for printing sizes of the form
// 1.X GB, 1.X MB, 1.X KB, 1.X bytes most appropriate to the size.
void
DiskSizeFormatted(std::uint64_t size, double* size_double, std::string* size_suffix,
                  uint* precision) {
  *size_double = static_cast<double>(size);
  *size_suffix = "";
  *precision = 0;
  if (size > 0) {
    *size_suffix = " bytes";
    if (*size_double >= 1e9) {
      *size_double /= 1e9;
      *size_suffix = " GB";
      *precision = 2;
    } else if (*size_double >= 1e6) {
      *size_double /= 1e6;
      *size_suffix = " MB";
      *precision = 1;
    } else if (*size_double >= 1e3) {
      *size_double /= 1e3;
      *size_suffix = " KB";
      *precision = 1;
    }
  }
}

void
khPrintFileSizes(const std::string& title, std::vector<std::string> paths) {
  // Set the iostream formatters.
  unsigned int original_precision = std::cout.precision();
  unsigned int precision = 0;
  std::cout << title << std::endl;
  std::string size_suffix("");
  std::uint64_t total_size = 0;
  double size_double;
  for (unsigned int i = 0; i < paths.size(); ++i) {
    if (paths[i].empty())
      continue;  // skip empty paths
    std::uint64_t size = khDiskUsage(paths[i]);
    total_size += size;
    if (size > 0) {
      DiskSizeFormatted(size, &size_double, &size_suffix, &precision);
      std::cout.precision(precision);
      std::cout << "  " << std::fixed << size_double << size_suffix << " : ";
    } else {
      std::cout << "  " << "Could not stat file: ";
    }
    std::cout << paths[i] << std::endl;
  }
  if (total_size > 0) {
    DiskSizeFormatted(total_size, &size_double, &size_suffix, &precision);
    std::cout.precision(precision);
    std::cout << "  " << std::fixed << size_double << size_suffix << " : TOTAL "
      << title << std::endl;
  } else {
    std::cout << "  None found" << std::endl;
  }
  // Reset the iostream formatters.
  std::cout.precision(original_precision);
}

bool
khGetFilesystemFreeSpace(const std::string &path, std::uint64_t &free)
{
#ifdef ENABLE_GFS_HACKS
  if (IsGFSPath(path)) {
    // Right now GFS access is through g2fuse which doesn't support statfs
    // but that's OK since most of our GFS cells should be HUGE and
    // growable. We'll just always say there's a ton of space left.

    // 10TB should do for now. :-)
    free = 10ULL * 1024 * 1024 * 1024 * 1024;
    return true;
  }
#endif


  // f_bsize  -> block size
  // f_blocks -> total blocks
  // f_bfree  -> # free blocks
  // f_bavail -> # free blocks avail to non-superuser

  struct statfs64 rec;
  if (statfs64(path.c_str(), &rec) == 0) {
    free = rec.f_bsize * rec.f_bavail;
    return true;
  } else {
    return false;
  }
}

void
khGetOverflowFilenames(const std::string &filename,
                       std::vector<std::string>& overflowFiles)
{
  std::string ext = khExtension(filename);
  std::string sansext = khDropExtension(filename);

  if (ext == ".img") {
    overflowFiles.push_back(sansext + ".ige");
  }
  // TODO: flesh this out
}


std::uint32_t
khMaxOpenFiles(void)
{
  unsigned int fileLimit = 256;
  struct rlimit limit;
  if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
    notify(NFY_WARN, "Unable to determine open file limit, assuming %u",
           fileLimit);
  } else {
    if ((limit.rlim_cur == RLIM_INFINITY) ||
        (limit.rlim_cur > std::numeric_limits<std::uint32_t>::max())) {
      fileLimit = std::numeric_limits<std::uint32_t>::max();
    } else {
      fileLimit = limit.rlim_cur;
    }
  }
  return fileLimit;

}

bool
khReadAll(int fd, void *buf, size_t count)
{
  while (count) {
    ssize_t numread;
    do {
      numread = ::read(fd, buf, count);
    } while ((numread == -1) && (errno == EINTR));

    if (numread == -1) {
      return false;
    } else if (numread == 0) {
      errno = ENODATA;
      return false;
    } else {
      count -= numread;
      buf = (char*)buf + numread;
    }
  }
  return true;
}

bool
khPreadAll(int fd, void *buf, size_t count, off64_t offset)
{
  while (count) {
    ssize_t numread;
    do {
      numread = ::pread64(fd, buf, count, offset);
    } while ((numread == -1) && (errno == EINTR));

    if (numread == -1) {
      return false;
    } else if (numread == 0) {
      errno = ENODATA;
      return false;
    } else {
      count -= numread;
      buf = (char*)buf + numread;
      offset += numread;
    }
  }
  return true;
}

bool
khWriteAll(int fd, const void *buf, size_t count)
{
  while (count) {
    ssize_t numwritten;
    do {
      numwritten = ::write(fd, buf, count);
    } while ((numwritten == -1) && (errno == EINTR));

    if (numwritten > 0) {
      count -= numwritten;
      buf = (char*)buf + numwritten;
    } else {
      return false;
    }
  }
  return true;
}

bool
khPwriteAll(int fd, const void *buf, size_t count, off64_t offset)
{
  while (count) {
    ssize_t numwritten;
    do {
      numwritten = ::pwrite64(fd, buf, count, offset);
    } while ((numwritten == -1) && (errno == EINTR));

    if (numwritten > 0) {
      count -= numwritten;
      buf = (char*)buf + numwritten;
      offset += offset;
    } else {
      if (numwritten < 0) {
        notify(NFY_DEBUG, "khPwriteAll: Failure to write: errno = %d\n", errno);
      }
      return false;
    }
  }
  return true;
}

void
WaitIfFileIsTooNew(const std::string &filename, unsigned int delaySec)
{
  if (delaySec == 0) return;

#ifdef ENABLE_GFS_HACKS
  if (IsGFSPath(filename)) {
    // Right now we access GFS via g2fuse. Apparently we can't rely on
    // valid mtimes coming back from g2fuse (GFS limitation?)

    // We only use this to work around the Delayed NFS visibility bug
    // and GFS writes are supposed to be synchronous. So don't bother
    // checking or waiting;
    return;
  }
#endif

  std::uint64_t fileSize;
  time_t mtime;
  if (!khGetFileInfo(filename, fileSize, mtime)) {
    throw khSimpleErrnoException("Unable to get modification time for ")
      << filename;
  }

  time_t now = time(0);
  if (mtime > now) {
    notify(NFY_NOTICE,
           "%s has timestamp in the future."
           " Waiting for %u seconds before reading.",
           filename.c_str(), delaySec);
    sleep(delaySec);
  } else if (now - mtime < (time_t)delaySec) {
    time_t waittime = (time_t)delaySec - (now - mtime);
    notify(NFY_NOTICE,
           "%s is very new. Waiting for %lld seconds before reading.",
           filename.c_str(), static_cast<long long int>(waittime));
    sleep(waittime);
  }
}

// ****************************************************************************
// ***  khFileList
// ****************************************************************************
void
khFileList::AddFile(const std::string &fname_)
{
  std::string fname = TrimWhite(fname_);
  if (khIsURI(fname)) {
    if (allow_uri_) {
      push_back(fname);
    } else {
      throw khSimpleException("URIs not supported");
    }
  } else {
    if (!check_existence_ || khExists(fname)) {
      push_back(want_abspath_ ? khAbspath(fname) : fname);
    } else {
      throw khSimpleErrnoException("Can't access '") << fname << "'";
    }
  }
}

void
khFileList::AddFromFilelist(const std::string &filelistname)
{
  std::ifstream in(filelistname.c_str());
  if (!in) {
    throw khSimpleException("Unable to open '") << filelistname << "'";
  }
  std::string line;
  while (std::getline(in, line)) {
    line = TrimWhite(line);
    line = TrimQuotes(line);
    AddFile(line);
  }
}



// ****************************************************************************
// ***  khFilesTransaction
// ****************************************************************************
khFilesTransaction::~khFilesTransaction(void) throw()
{
  for (std::vector<std::string>::const_iterator f = newlist.begin();
       f != newlist.end(); ++f) {
    // anything that's in my newlist should still exist only as the new
    // file. Commit()'s error handling should clean up all else
    (void)khPruneFileOrDir(*f + newext);
  }

  for (std::vector<std::string>::const_iterator d = deletelist.begin();
       d != deletelist.end(); ++d) {
    // anything that's in my deletelist should be restored
    std::string backup = *d + backupext;
    if (!khRename(backup, *d)) {
      notify(NFY_WARN, "Unable to restore %s -> %s",
             backup.c_str(), d->c_str());
    }
  }
}


void
khFilesTransaction::AddNewPath(const std::string &path)
{
  // make sure it has the new extension
  assert(khHasExtension(path, newext));

  // strip off the newext and push the target path onto our list
  newlist.push_back(path.substr(0, path.size() - newext.size()));
}


void
khFilesTransaction::DeletePath(const std::string &path)
{
  if (khExists(path)) {
    std::string backup = path + backupext;
    // prune any existing backup
    if (khExists(backup)) {
      (void)khPruneFileOrDir(backup);
    }

    if (!khRename(path, backup)) {
      throw khSimpleErrnoException("Unable rename '")
        << path << "' to '" << backup << "'";
    }

    // add the "deleted" file to my list
    deletelist.push_back(path);
  }
}


bool
khFilesTransaction::Commit(void)
{
  for (std::vector<std::string>::const_iterator f = newlist.begin();
       f != newlist.end(); ++f) {
    if (!khReplace(*f, newext, backupext)) {
      // undo the work we've already done
      for (std::vector<std::string>::const_iterator u = newlist.begin();
           u != f; ++u) {
        (void)khPruneFileOrDir(*u);
        std::string backup = *u + backupext;
        if (khExists(backup)) {
          if (!khRename(backup, *u)) {
            notify(NFY_WARN, "Unable to restore %s -> %s",
                   backup.c_str(), u->c_str());
          }
        }
      }

      // clean up the rest of the files that we haven't done yet
      // (this includes the one that just failed)
      for ( ; f != newlist.end(); ++f) {
        (void)khPruneFileOrDir(*f + newext);
      }

      // now clear the list so I don't try to clean it up again
      newlist.clear();

      return false;
    }
  }

  // all the new ones have been moved into place, now clean up the
  // backup copies
  for (std::vector<std::string>::const_iterator f = newlist.begin();
       f != newlist.end(); ++f) {
    std::string backup = *f + backupext;
    if (khExists(backup)) {
      (void)khPruneFileOrDir(backup);
    }
  }

  // now clean up the backup copies for the deletes
  for (std::vector<std::string>::const_iterator d = deletelist.begin();
       d != deletelist.end(); ++d) {
    (void)khPruneFileOrDir(*d + backupext);
  }

  // now empty my lists
  newlist.clear();
  deletelist.clear();

  return true;
}


// if srcSize == 0, stat the file to get the size
bool
khAppendFile(const std::string &srcFilename,
             const std::string &destFilename,
             std::uint64_t &offsetReturn)
{
  std::uint64_t srcSize = 0;
  std::uint64_t destSize = 0;
  time_t mtime;

  // get the src file ready
  if (!khGetFileInfo(srcFilename, srcSize, mtime)) {
    notify(NFY_WARN, "Unable to get file size for %s: %s",
           srcFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }
  khReadFileCloser srcfile(::open(srcFilename.c_str(),
                                  O_LARGEFILE | O_RDONLY));
  if (!srcfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           srcFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }


  // get the dest file ready
  // if this fails it's OK. It just means the file doesn't exist yet
  (void) khGetFileInfo(destFilename, destSize, mtime);

  if (!khEnsureParentDir(destFilename)) {
    // warning already emitted
    return false;
  }
  khWriteFileCloser destfile(::open(destFilename.c_str(),
                                    O_LARGEFILE | O_WRONLY | O_APPEND |
                                    O_CREAT,
                                    0666));
  if (!destfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           destFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }


  static const std::uint32_t bufsize = 1024 * 1024;
  khDeleteGuard<char, ArrayDeleter> buf(TransferOwnership(new char[bufsize]));
  while (srcSize) {
    std::uint32_t readSize = (std::uint32_t)std::min((std::uint64_t)bufsize, srcSize);
    if (!khReadAll(srcfile.fd(), &*buf, readSize)) {
      notify(NFY_WARN, "Unable to read %d bytes from %s: %s",
             readSize, srcFilename.c_str(),
             khstrerror(errno).c_str());
      return false;
    }
    if (!khWriteAll(destfile.fd(), &*buf, readSize)) {
      notify(NFY_WARN, "Unable to write %d bytes to %s: %s",
             readSize, destFilename.c_str(),
             khstrerror(errno).c_str());
      return false;
    }
    srcSize -= readSize;
  }

  offsetReturn = destSize;
  return true;
}

bool khCopyFileHelper(khReadFileCloser* srcfile, const char* const srcFname,
                      khWriteFileCloser* destfile, const char* const dstFname,
                      size_t srcSize) {
  static const size_t bufsize = 1024 * 1024;
  khDeleteGuard<char, ArrayDeleter> buf(TransferOwnership(new char[bufsize]));
  while (srcSize) {
    size_t readSize = (std::min(bufsize, srcSize));
    if (!khReadAll(srcfile->fd(), &*buf, readSize)) {
      notify(NFY_WARN, "Unable to read %ld bytes from %s: %s",
             readSize, srcFname, khstrerror(errno).c_str());
      return false;
    }
    if (!khWriteAll(destfile->fd(), &*buf, readSize)) {
      notify(NFY_WARN, "Unable to write %ld bytes to %s: %s",
             readSize, dstFname, khstrerror(errno).c_str());
      return false;
    }
    srcSize -= readSize;
  }

  return true;
}

bool
khCopyFile(const std::string &srcFilename,
           const std::string &destFilename)
{
  std::uint64_t srcSize = 0;
  time_t mtime;

  // get the src file ready
  if (!khGetFileInfo(srcFilename, srcSize, mtime)) {
    notify(NFY_WARN, "Unable to get file size for %s: %s",
           srcFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }
  khReadFileCloser srcfile(::open(srcFilename.c_str(),
                                  O_LARGEFILE | O_RDONLY));
  if (!srcfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           srcFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }


  if (!khEnsureParentDir(destFilename)) {
    // warning already emitted
    return false;
  }
  khWriteFileCloser destfile(::open(destFilename.c_str(),
                                    O_LARGEFILE | O_WRONLY |
                                    O_CREAT | O_TRUNC,
                                    0666));
  if (!destfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           destFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }

  return khCopyFileHelper(&srcfile, srcFilename.c_str(),
                          &destfile, destFilename.c_str(), srcSize);
}


bool khCopyOpenFile(const int srcFd, const char* const srcFname,
                    const int dstFd, const char* const dstFname) {
  std::uint64_t srcSize = 0;
  time_t mtime;

  // Check src file
  khReadFileCloser srcfile(srcFd);
  if (!srcfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           srcFname, khstrerror(errno).c_str());
    return false;
  }

  // get the src file ready
  if (!khGetFileInfo(srcFd, srcSize, mtime)) {
    notify(NFY_WARN, "Unable to get file size for %s: %s",
           srcFname, khstrerror(errno).c_str());
    return false;
  }

  // Check dst file
  khWriteFileCloser destfile(dstFd);
  if (!destfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           dstFname, khstrerror(errno).c_str());
    return false;
  }

  // TODO: Consider refactoring the auto-closing of the files.
  return khCopyFileHelper(&srcfile, srcFname, &destfile, dstFname, srcSize);
}

bool
khLinkOrCopyFile(const std::string &srcFilename,
                 const std::string &destFilename)
{
  if (!khEnsureParentDir(destFilename)) {
    // warning already emitted
    return false;
  }
  if (!khDirExists(srcFilename) &&
      !khSymlinkExists(srcFilename) &&
      khLink(srcFilename, destFilename, true /* silent */)) {
    return true;
  } else {
    return khCopyFile(srcFilename, destFilename);
  }
}


void khLinkOrCopyDirContents(const std::string &srcDir,
                             const std::string &destDir)
{
  DIR *dir = ::opendir(srcDir.c_str());
  if (!dir) {
    throw khSimpleErrnoException("Unable to opendir ") << srcDir;
  }
  khDIRCloser closer(dir);
  struct dirent64 *entry;
  while ((entry = ::readdir64(dir))) {
    if (strcmp(entry->d_name, ".") == 0) continue;
    if (strcmp(entry->d_name, "..") == 0) continue;
    std::string src_file  = khComposePath(srcDir, entry->d_name);
    std::string dest_file = khComposePath(destDir, entry->d_name);
    if (khDirExists(src_file)) {
      khLinkOrCopyDirContents(src_file, dest_file);
    } else if (!khLinkOrCopyFile(src_file, dest_file)) {
      throw khSimpleException("Unable to copy ")
        << src_file << " to " << dest_file;
    }
  }
}


bool
khMakeEmptyFile(const std::string &destFilename)
{
  if (!khEnsureParentDir(destFilename)) {
    // warning already emitted
    return false;
  }
  khWriteFileCloser destfile(::open(destFilename.c_str(),
                                    O_LARGEFILE | O_WRONLY | O_CREAT |
                                    O_TRUNC,
                                    0666));
  if (!destfile.valid()) {
    notify(NFY_WARN, "Unable to open %s: %s",
           destFilename.c_str(), khstrerror(errno).c_str());
    return false;
  }
  return true;
}


bool
khFillFile(int fd, char fill, std::uint64_t size)
{
  static const std::uint32_t bufsize = 1024 * 1024;
  khDeleteGuard<char, ArrayDeleter> buf(TransferOwnership(new char[bufsize]));
  memset(&*buf, fill, bufsize);
  while (size) {
    std::uint32_t writeSize = (std::uint32_t)std::min((std::uint64_t)bufsize, size);
    if (!khWriteAll(fd, &*buf, writeSize)) {
      notify(NFY_WARN, "Unable to write %d bytes: %s",
             writeSize, khstrerror(errno).c_str());
      return false;
    }
    size -= writeSize;
  }
  return true;
}

bool
khWriteStringToFile(const std::string &filename, const std::string &str)
{
  return khWriteSimpleFile(filename, &str[0], str.length());
}

bool
khReadStringFromFile(const std::string &filename, std::string &str,
                     std::uint64_t limit)
{
  // str.resize() can throw an exception. guard against it.
  try {
    std::uint64_t size = 0;
    time_t mtime;
    if (!khGetFileInfo(filename, size, mtime)) {
      // don't warn because file does not exist
      // it's probably not an error.
      return false;
    }
    if ((limit > 0) && (limit < size)) {
      size = limit;
    }

    int oldsize = str.size();
    str.resize(oldsize + size);

    khReadFileCloser file(::open(filename.c_str(), O_RDONLY));
    if (!file.valid()) {
      notify(NFY_WARN, "Unable open %s", filename.c_str());
      return false;
    }

    if (!khReadAll(file.fd(), &str[oldsize], size)) {
      notify(NFY_WARN, "Unable to read from %s", filename.c_str());
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown error");
  }
  return false;
}


// Write the buffer to the specified file - overwrite file if exists
bool
khWriteSimpleFile(const std::string &filename, const void* buf, size_t size)
{
  if (!khEnsureParentDir(filename)) {
    // warning already emitted
    return false;
  }

  khWriteFileCloser file(::open(filename.c_str(),
                                O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC,
                                0644));
  if (!file.valid()) {
    notify(NFY_WARN, "Unable to open %s for writing: %s", filename.c_str(),
           khstrerror(errno).c_str());
    return false;
  }
  if (!khWriteAll(file.fd(), buf, size)) {
    notify(NFY_WARN, "Unable to write %llu bytes to %s: %s",
           static_cast<long long unsigned>(size), filename.c_str(),
           khstrerror(errno).c_str());
    return false;
  }

  return true;
}

void khWriteStringVectorToFile(const std::string &filename,
                               const std::vector<std::string> &strs) {
  if (!khEnsureParentDir(filename)) {
    throw khSimpleException("Can't make parent dir");
  }

  khWriteFileCloser file(::open(filename.c_str(),
                                O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC,
                                0644));
  if (!file.valid()) {
    throw khSimpleErrnoException("Unable to open ")
      << filename << " for writing";
  }
  for (std::vector<std::string>::const_iterator str = strs.begin();
       str != strs.end(); ++str) {
    if (!khWriteAll(file.fd(), &(*str)[0], str->size())) {
      throw khSimpleErrnoException("Unable to write ")
        << str->size() << " bytes to " << filename;
    }
    if (!khWriteAll(file.fd(), "\n", 1)) {
      throw khSimpleErrnoException("Unable to write newline to ")
        << filename;
    }
  }
}


void khWriteStringToFileOrThrow(const std::string &filename,
                                const std::string &str) {
  if (!khEnsureParentDir(filename)) {
    throw khSimpleException("Can't make parent dir for ") << filename;
  }

  khWriteFileCloser file(::open(filename.c_str(),
                                O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC,
                                0644));
  if (!file.valid()) {
    throw khSimpleErrnoException("Unable to open ")
      << filename << " for writing";
  }
  if (!khWriteAll(file.fd(), &(str)[0], str.size())) {
    throw khSimpleErrnoException("Unable to write ")
        << str.size() << " bytes to " << filename;
  }
}


void khReadStringVectorFromFile(const std::string &filename,
                                std::vector<std::string> &strs) {
  std::ifstream in(filename.c_str());
  if (!in) {
    throw khSimpleException("Unable to open '") << filename << "'";
  }
  std::string line;
  while (std::getline(in, line)) {
    strs.push_back(line);
  }
}

int
khOpen(const std::string &fname, int flags, mode_t createMask)
{
  return ::open(fname.c_str(), flags, createMask);
}

int
khFsync(int fd)
{
  int result = ::fsync(fd);
  if ((result == -1) &&
      ((errno == EROFS) || (errno == EINVAL))) {
    // some file can't be sync'd, don't whine in those cases
    errno = 0;
    result = 0;
  }
  return result;
}

int
khClose(int fd)
{
  int result;
  do {
    result = ::close(fd);
  } while ((result == -1) && (errno == EINTR));
  return result;
}

int
khFsyncAndClose(int fd)
{
  int result = khFsync(fd);
  if (result == -1) {
    int saveErrno = errno;
    (void)khClose(fd);
    errno = saveErrno;
  } else {
    result = khClose(fd);
  }
  return result;
}

std::string khGetCwd(void) {
  char *cwd = ::getcwd(0, 0);
  if (cwd) {
    std::string ret = cwd;
    free(cwd);
    return ret;
  } else {
    notify(NFY_WARN, "Unable to getcwd: %s", khstrerror(errno).c_str());
  }
  return std::string();
}


std::string khFindFileInPath(const std::string &file) {
  if (khIsAbspath(file)) {
    return file;
  }

  if (file.find("/") != std::string::npos) {
    std::string path = khComposePath(khGetCwd(), file);
    if (khExists(path)) {
      return path;
    }
  } else {
    const char *path = getenv("PATH");
    if (path) {
      std::vector<std::string> dirs;
      split(path, ":", back_inserter(dirs));

      for (unsigned int i = 0; i < dirs.size(); ++i) {
        std::string path = khComposePath(dirs[i], file);
        if (khExists(path)) {
          return path;
        }
      }
    }
  }

  return std::string();
}

std::string
GetLogFileNameAndCleanupLogs(std::string dirname,
                             std::string filePrefix,
                             int logFilesToKeep,
                             std::string logFilesToKeepEnvVariable)
{
  std::string logfilename = "";

  // Keep track of only the last N=logFilesToKeep logs by default.
  // User can override this with logFilesToKeepEnvVariable.
  // Set to 0 to remove the logging, or any number they want.
  if (getenv(logFilesToKeepEnvVariable.c_str())) {
    logFilesToKeep = atoi(getenv(logFilesToKeepEnvVariable.c_str()));
  }
  if (logFilesToKeep <= 0)
    return logfilename;

  if (dirname.empty()) {
    // Find a safe home for the logfile, if any.
    // We'll choose from "HOME/.fusion" and various temp directories.
    // If none of these exist, leaving the logfilename_ empty will
    // ensure the logfile will not be written.
    // Run through preferences to find a suitable place to put the publisher log
    std::vector<std::string> logfile_dirs;
    if (getenv("HOME")) {
      // User preferences folder.
      logfile_dirs.push_back(std::string(getenv("HOME")) + "/.fusion");
    }

    if (getenv("TMPDIR"))
      logfile_dirs.push_back(getenv("TMPDIR")); // User defined temp folder.

    if (P_tmpdir)
      logfile_dirs.push_back(P_tmpdir); // from stdio.h

    logfile_dirs.push_back("/tmp"); // Standard temp folder.

    // Take the first one that exists and use it.
    for (unsigned int i = 0; i < logfile_dirs.size(); ++i) {
      if ((!logfile_dirs[i].empty()) && khDirExists(logfile_dirs[i])
          && (access(logfile_dirs[i].c_str(), W_OK) == 0)) {
        dirname = logfile_dirs[i];
        break;
      }
    }
  }

  // If no directory is specified at this point, return empty.
  if (dirname.empty())
    return logfilename;

  std::string format = "%F-%T";
  std::string dateTime = GetCurrentTimeStringWithFormat(format);
  std::string logfilePrefix = dirname + "/" + filePrefix + ".";
  std::string logfileSuffix = ".log";
  logfilename = logfilePrefix + dateTime + logfileSuffix;

  // Clean up any older log files than the most recent N (including this
  // new one).
  std::vector<std::string> logfiles;
  khGetFilenamesMatchingPattern(logfilePrefix, logfileSuffix, logfiles);

  // Sort the results in lexicographical order.
  std::sort(logfiles.begin(), logfiles.end(), std::less<std::string>());

  // Special case:
  // Look for fileprefix.log (i.e., missing a timestamp), and swap this to the
  // front of the list (i.e., assume it's the oldest).
  std::string nonTimeStampFileName = dirname + "/" + filePrefix + logfileSuffix;
  for (std::vector<std::string>::iterator logfile_iter = logfiles.begin();
       logfile_iter != logfiles.end(); ++logfile_iter) {
    if (nonTimeStampFileName.compare(*logfile_iter) == 0) {
      // Swap the nonTimeStampFileName to the front of the list.
      // Don't care if it's already on the front, still ok to do.
      logfiles.erase(logfile_iter);
      logfiles.insert(logfiles.begin(), nonTimeStampFileName);
      break;
    }
  }

  // Unlink any files older than the most recent logFilesToKeep.
  logFilesToKeep--; // Subtract off for the one we're adding now.
  for (int i = 0; i < static_cast<int>(logfiles.size()) - logFilesToKeep; ++i) {
    khUnlink(logfiles[i]);
  }

  return logfilename;
}

namespace {
// Given a published DB path, if it fits ([.]*)/([^\*]/key_word/[.]*)
// return the position of "/" separating $1 and $2, else return 0.
// examples of keyword gedb.kda, mapdb.kda, unifiedindex.kda
int GetKeyPosition(const std::string& file_path,
                   const std::string& key_word) {
  // +2 because strlen("/key_word/")
  // 10 because strlen("/gedb.kda/") == 10, strlen("/mapdb.kda/") == 11
  const std::string::size_type key_length =
      key_word.empty() ? 10 : key_word.length() + 2;
  for (std::string buff = file_path; buff.length() > key_length;
       buff = khDirname(buff)) {
    const std::string base = khBasename(buff);
    if (key_word.empty() ? (base == kGedbKey || base == kMapdbKey
                            || base == kUnifiedIndexKey)
                         : (base == key_word)) {
      buff = khDirname(buff);
      if (buff.length() > 1) {
        buff = khDirname(buff);
        if (StartsWith(file_path, buff)) {
          return buff.length();
        } else if (buff == ".") {
          return -1;
        }
      }
      break;
    }
  }
  return 0;
}

}  // end namespace


// Returns true if it looks like an absolute path to a Fusion DB.
// If return value is true, we return db_type through db_type.
bool khIsAbsoluteDbName(const std::string& file_path, std::string* db_type) {
  if (!khIsAbspath(file_path)) {
    return false;
  }
  std::string normalized_path = khNormalizeDir(file_path, false);
  // +2 because strlen("/key_word/")
  // 10 because strlen("/gedb.kda/") == 10, strlen("/mapdb.kda/") == 11
  const std::string::size_type key_length = 10;
  std::string key_word;
  std::string db_name;
  bool found_key = false;
  for (std::string buff = normalized_path; buff.length() > key_length;
       buff = khDirname(buff)) {
    key_word = khBasename(buff);
    if (key_word == kGedbKey || key_word == kMapdbKey) {
      buff = khDirname(buff);  // ends with db_name
      if (buff.length() > 1) {
        db_name = khBasename(buff);
        // Ensure that there is something before db_name
        buff = khDirname(buff);
        if (StartsWith(normalized_path, buff)) {
          found_key = true;
        }
      }
      break;
    }
  }
  if (!found_key) {
    return false;
  }
  db_type->clear();
  if (key_word == kGedbKey) {
    if (EndsWith(db_name, kDatabaseSuffix) &&
        EndsWith(normalized_path, kGedbBase)) {
      *db_type = kDatabaseSubtype;
    }
  } else if (EndsWith(normalized_path, kMapdbBase)) {
    if (EndsWith(db_name, kMapDatabaseSuffix)) {
      *db_type = kMapDatabaseSubtype;
    } else if (EndsWith(db_name, kMercatorMapDatabaseSuffix)) {
      *db_type = kMercatorMapDatabaseSubtype;
    }
  }
  return !db_type->empty();
}

// Given a published DB path, if it fits ([.]*)/([^\*]/key_word/[.]*)
// return $2, else return the string as it is.
// examples of keyword gedb.kda, mapdb.kda, unifiedindex.kda
std::string khGetDbSuffix(const std::string& file_path,
                          const std::string& key_word) {
  if (!khIsAbspath(file_path)) {
    return file_path;
  }
  const int key_pos = GetKeyPosition(file_path, key_word);
  return key_pos <= 0 ? file_path : file_path.substr(key_pos + 1);
}

// Given a published DB path, if it fits ([.]*)/([^\*]/key_word/[.]*)
// return $1/, else return empty string
// examples of keyword gedb.kda, mapdb.kda, unifiedindex.kda
std::string khGetDbPrefix(const std::string& file_path,
                          const std::string& key_word) {
  if (!khIsAbspath(file_path)) {
    return "";
  }
  const int key_pos = GetKeyPosition(file_path, key_word);
  return key_pos <= 0 ? "" : file_path.substr(0, key_pos + 1);
}

bool khIsDbSuffix(const std::string& file_path, const std::string& key_word) {
  if (khIsAbspath(file_path)) {
    return false;
  }
  const int key_pos = GetKeyPosition(file_path, key_word);
  return key_pos == -1;
}

// If original string is /gevol/published_dbs/stream_space/[^/]*/*, then
// return /gevol/published_dbs/search_space.
// Otherwise, return empty string.
std::string khGetSearchPrefix(const std::string& file_path) {
  const std::string& key_word = kStreamSpace;
  // Note: after applying dirname to any absolute path in cycle, we finally
  // get in buff '/'. So, check for buff length > 1.
  for (std::string buff = file_path;
       buff.size() > 1; buff = khDirname(buff)) {
    if (khBasename(buff) == key_word) {
      std::string out = buff.substr(0, buff.size() - key_word.size()) +
          kSearchSpace;
      return out;
    }
  }
  return std::string();
}

// If original string is /gevol/published_dbs/stream_space/[^/]*/*, then
// return /gevol/published_dbs/stream_space.
// Otherwise, return empty string.
std::string khGetStreamPrefix(const std::string& file_path) {
  const std::string& key_word = kStreamSpace;
  // Note: after applying dirname to any absolute path in cycle, we finally
  // get in buff '/'. So, check for buff length > 1.
  for (std::string buff = file_path;
       buff.size() > 1; buff = khDirname(buff)) {
    if (khBasename(buff) == key_word) {
      return buff;
    }
  }
  return std::string();
}


bool khFilesEqual(const std::string &filename_1,
                  const std::string &filename_2) {
  // check file existence and sizes before anything else
  std::uint64_t size_1, size_2;
  time_t mtime_1, mtime_2;
  if (!khGetFileInfo(filename_1, size_1, mtime_1)) {
    notify(NFY_WARN, "Unable to get file info for %s: %s",
           filename_1.c_str(), khstrerror(errno).c_str());
    return false;
  }
  if (!khGetFileInfo(filename_2, size_2, mtime_2)) {
    notify(NFY_WARN, "Unable to get file info for %s: %s",
           filename_2.c_str(), khstrerror(errno).c_str());
    return false;
  }
  if (size_1 != size_2) {
    return false;
  }

  // now open up readers for each file
  khReadFileCloser file_1(::open(filename_1.c_str(), O_LARGEFILE | O_RDONLY));
  if (!file_1.valid()) {
    notify(NFY_WARN, "Unable to open %s for reading: %s",
           filename_1.c_str(), khstrerror(errno).c_str());
    return false;
  }
  khReadFileCloser file_2(::open(filename_2.c_str(), O_LARGEFILE | O_RDONLY));
  if (!file_2.valid()) {
    notify(NFY_WARN, "Unable to open %s for reading: %s",
           filename_2.c_str(), khstrerror(errno).c_str());
    return false;
  }

  // allocate a couple of buffers
  static const size_t buf_size = 1024 * 512;  // 0.5M
  khDeleteGuard<char, ArrayDeleter> buf_1(
      TransferOwnership(new char[buf_size]));
  khDeleteGuard<char, ArrayDeleter> buf_2(
      TransferOwnership(new char[buf_size]));

  // read the files in parallel and compare each chunk
  while (size_1) {
    size_t read_size = std::min(buf_size, size_1);
    if (!khReadAll(file_1.fd(), &*buf_1, read_size)) {
      notify(NFY_WARN, "Unable to read %ld bytes from %s: %s",
             read_size, filename_1.c_str(), khstrerror(errno).c_str());
      return false;
    }
    if (!khReadAll(file_2.fd(), &*buf_2, read_size)) {
      notify(NFY_WARN, "Unable to read %ld bytes from %s: %s",
             read_size, filename_2.c_str(), khstrerror(errno).c_str());
      return false;
    }
    if (memcmp(&*buf_1, &*buf_2, read_size) != 0) {
      return false;
    }

    size_1 -= read_size;
  }

  return true;
}


khTmpFileGuard::khTmpFileGuard(void)
    : filename(khTmpFilename(khComposePath(khTmpDirPath(), "khtmp"), 0600)),
      dounlink(true) {
  if (filename.empty()) {
    throw khSimpleErrnoException("Unable to create tmp file in ")
        << khTmpDirPath();
  }
}

khTmpFileGuard::khTmpFileGuard(const std::string &dir)
    : filename(khTmpFilename(khComposePath(dir, "khtmp"), 0600)),
      dounlink(true) {
  if (filename.empty()) {
    throw khSimpleErrnoException("Unable to create tmp file in ")
        << dir;
  }
}

khTmpFileGuard::~khTmpFileGuard(void) {
  if (dounlink) {
    (void)khUnlink(filename);
  }
}
