/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHFILEUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHFILEUTILS_H_

#include <sys/types.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include "common/khGuard.h"
//#include "common/khTypes.h"
#include <cstdint>
#include <cstdint>

std::string khBasename(const std::string &filename);
std::string khDirname(const std::string &filename);
std::string khExtension(const std::string &filename,
                        bool includeDot = true);
std::string khDropExtension(const std::string &filename);
std::string khReplaceExtension(const std::string &filename,
                               const std::string &newExt);
bool khHasExtension(const std::string &filename,
                    const std::string &ext);
std::string khEnsureExtension(const std::string &filename,
                              const std::string &ext);
std::string khComposePath(const std::string &dir, const std::string &file);
std::string khComposePath(const std::string &parentdir,
                          const std::string &childdir,
                          const std::string &file);
std::string khComposeTimePath(const struct tm& time);

bool khIsURI(const std::string &filename);
bool khIsAbspath(const std::string &filename);
std::string khAbspath(const std::string &filename);
std::string khNormalizeDir(const std::string &dirname,
                           bool add_trailing_slash = true);
std::string khRelativePath(const std::string &absfrom,
                           const std::string &absto);

bool khExists(const std::string &filename);  // File exists may not be readable.
bool khIsExecutable(const std::string &filename);  // File is executable.
bool khIsFileReadable(const std::string &filename);  // File is readable.

bool khDirExists(const std::string &dirname);
bool khSymlinkExists(const std::string &filename);
// find all files in a directry that have a given extention.
// return the full path of the files
void khFindFilesInDir(const std::string &dirname,
                      std::vector<std::string> &files_return,  // full path
                      const std::string &ext = std::string());
// find all files in a directry that have a given extention.
// return the basenames of the file (dir is trimmed)
void khFindBasenamesInDir(const std::string &dirname,
                          const std::string &ext,
                          std::vector<std::string> *files_return);  // basename
 std::uint32_t khCountFilesInDir(const std::string &dirname);

// will return empty string if not found
std::string khFindFileInPath(const std::string &file);

bool khReadSymlink(const std::string &filename, std::string &target);
bool khPruneDir(const std::string &dirname);
bool khPruneFileOrDir(const std::string &path);
bool khMakeDir(const std::string &dirname, mode_t mode = 0777);
void khMakeDirOrThrow(const std::string &dirname, mode_t mode = 0777);
// clean old dir and make new empty one
bool khMakeCleanDir(const std::string &dirname, mode_t mode = 0777);
bool khChmod(const std::string &filename, mode_t mode);
std::string khGetCwd(void);
bool khChangeDir(const std::string &filename);
bool khMove(const std::string &oldname, const std::string &newname);
bool khRename(const std::string &oldname, const std::string &newname);
bool khSymlink(const std::string &oldpath, const std::string &newpath);
bool khLink(const std::string &oldpath,
            const std::string &newpath,
            bool silent = false);
bool khUnlink(const std::string &path);
bool khEnsureParentDir(const std::string &filename, mode_t mode = 0777);

// Returns path to local tmp directory for fusion files built on local tmp dir
std::string khTmpFusionDirPath();

// Returns path for best local tmp directory (no trailing /)
std::string khTmpDirPath();

// kind-of-like tempnam, but safer because it also makes the file so another
// thread/app can't make one with the same name
std::string khTmpFilename(const std::string &basename, mode_t mode = 0666);

// Creates a temporary directory under /tmp (or as defined in the env).
// The directory is created with permissions 0700.
std::string khCreateTmpDir(const std::string &prefix);

void khGetNumericFilenames(const std::string &pattern,
                           std::vector<std::string>& out);
std::string khMakeNumericFilename(const std::string &base,
                                  std::uint64_t num,
                                  unsigned int numPlaces = 3);

// Return a list of files (in no particular order) that
// start with specified prefix and end with the specified suffix.
// directory must be included in prefix
// returned list is full path
void
khGetFilenamesMatchingPattern(const std::string& prefix,
                              const std::string& suffix,
                              std::vector<std::string>& out);

// Return a list of files (in no particular order) that
// start with specified prefix and end with the specified suffix.
// prefix is compared only against basename
// returned list is basename
void khGetBasenamesMatchingPattern(const std::string &dirname,
                                   const std::string& prefix,
                                   const std::string& suffix,
                                   std::vector<std::string>* out);

// GetLogFileNameAndCleanupLogs will create a filename of the form:
// dirname/filePrefix.YYYY-MM-DD-HH:MM:SS.log
// It will also clean up older logfiles than the N most recent (including this
// new one) where N is specified by logFilesToKeep which can be overriden by the
// logFilesToKeepEnvVariable.
// If dirname is empty it will attempt to put it in the following directories:
// ~/.fusion, TMPDIR, /tmp
// If no directory is specified or a valid directory found,
// or if logFilesToKeep <= 0, then an empty string is returned.
std::string GetLogFileNameAndCleanupLogs(std::string dirnameInput,
                             std::string filePrefix,
                             int logFilesToKeep,
                             std::string logFilesToKeepEnvVariable);

// will throw if file doesn't exist
 std::uint64_t khGetFileSizeOrThrow(const std::string &fname);

// Will get the disk usage for a file/directory.
// returns 0 if the file does not exist.
 std::uint64_t khDiskUsage(const std::string& path);

// Will get the disk usage for a file/directory.
// returns 0 if the file does not exist.
// Convenience utility. Same as khDiskUsage but returns only 32bit size.
 std::uint32_t khDiskUsage32(const std::string& path);

bool khGetFileInfo(const std::string &fname, std::uint64_t &size, time_t &mtime);
bool khGetFilesystemFreeSpace(const std::string &path, std::uint64_t &free);

// Print to stdout the title and a line with the file/directory size for
// each given path.  Empty paths are skipped.
void khPrintFileSizes(const std::string& title, std::vector<std::string> paths);

// move a new file into place, saving a copy of the old one
bool khReplace(const std::string &targetfile, const std::string &newsuffix,
               const std::string &backupsuffix);

void khGetOverflowFilenames(const std::string &filename,
                            std::vector<std::string>& overflowFiles);

// will default to low number if cannot query system
 std::uint32_t khMaxOpenFiles(void);

// Try really hard to read count bytes from fd into buf
// will retry if partial read or if errno == EINTR
// On failure errno holds the low level failure
bool khReadAll(int fd, void *buf, size_t count);
bool khPreadAll(int fd, void *buf, size_t count, off64_t offset);

// Try really hard to write count bytes from buf into fd
// will retry if partial write or if errno == EINTR
// On failure errno holds the low level failure
bool khWriteAll(int fd, const void *buf, size_t count);
bool khPwriteAll(int fd, const void *buf, size_t count, off64_t offset);

// if returns true, offsetReturn contains offset into dest where src was placed
// if returns false, warning has already been emitted
bool khAppendFile(const std::string &srcFilename,
                  const std::string &destFilename,
                  std::uint64_t &offsetReturn);

// if returns false, warning has already been emitted
bool khMakeEmptyFile(const std::string &destFilename);

// if returns false, warning has already been emitted
bool khFillFile(int fd, char fill, std::uint64_t size);

// Copy from source to dest file. Files are closed upon completion.
bool khCopyOpenFile(const int srcFd, const char* const srcFname,
                    const int dstFd, const char* const dstFname);

bool khCopyFile(const std::string &srcFilename,
                const std::string &destFilename);

bool khLinkOrCopyFile(const std::string &srcFilename,
                      const std::string &destFilename);

// will throw exception
void khLinkOrCopyDirContents(const std::string &srcDir,
                             const std::string &destDir);

// Writes str to filename. file is opened in O_WRONLY | O_CREAT | O_TRUNC mode
bool khWriteStringToFile(const std::string &filename, const std::string &str);

// Writes str to filename. file is opened in O_WRONLY | O_CREAT | O_TRUNC mode
// will throw exception on error
void khWriteStringToFileOrThrow(const std::string &filename,
                                const std::string &str);

// file is opened in O_WRONLY | O_CREAT | O_TRUNC mode
// will thow exception on error
void khWriteStringVectorToFile(const std::string &filename,
                               const std::vector<std::string> &strs);

// Read file into a vector of strings - appends to vector
// will throw on error
void khReadStringVectorFromFile(const std::string &filename,
                                std::vector<std::string> &strs);

// Read file into a string - appends to str.
// if limit is greater than zero, read only that man bytes
bool khReadStringFromFile(const std::string &filename, std::string &str,
                          std::uint64_t limit = 0);

// Write the buffer to the specified file - overwrite file if exists
bool khWriteSimpleFile(const std::string &filename, const void* buf,
                       size_t size);

// Read simple file supplied for symmetry (same as Read String From File)
inline bool khReadSimpleFile(const std::string &filename, std::string &str,
                             std::uint64_t limit = 0) {
  return khReadStringFromFile(filename, str, limit);
}

void WaitIfFileIsTooNew(const std::string &filename, unsigned int delaySec);

// very thin wrappers around ::open
// no warnings emitted, no parent dirs created
// no exceptions, return and errno preserved from ::open
int khOpen(const std::string &fname, int flags, mode_t createMask = 0666);
inline int khOpenForWrite(const std::string &fname, mode_t createMask = 0666) {
  return khOpen(fname, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC, createMask);
}
inline int khOpenForAppend(const std::string &fname, mode_t createMask = 0666) {
  return khOpen(fname, O_WRONLY | O_CREAT | O_LARGEFILE | O_APPEND, createMask);
}
inline int khOpenForRead(const std::string &fname) {
  return khOpen(fname, O_RDONLY | O_LARGEFILE);
}

// very thin wrapper around ::close
// loops on ::close while errno==EINTR
// no warnings emitted
// no exceptions, return and errno preserved from ::close
int khClose(int fd);

// very thin wrapper around ::fsync
// ignores errors on files that don't support fsync
// no warnings emitted
// no exceptions, return and errno preserved from ::fsync
int khFsync(int fd);

// combination of fsync & close
// if fsync reports an error, the file will still be closed, but the error
// from fsync will be used regardless of the resturn code from ::close
// if fsync succeeds, the return code and errno will come from ::close
int khFsyncAndClose(int fd);

// ****************************************************************************
// ***  khFileList
// ***
// ***  Class for handling lists of files,
// ***  Does existence checks by default. Optionally converts to abspath
// ***
// ***  NOTE: This throws exceptions!
// ****************************************************************************
class khFileList : public std::vector<std::string> {
 public:
  khFileList(bool check_existence = true,
             bool want_abspath = true,
             bool allow_uri = true)
      : check_existence_(check_existence),
        want_abspath_(want_abspath),
        allow_uri_(allow_uri) {
  }

  void AddFile(const std::string &fname);

  void AddFromFilelist(const std::string &filelistname);

  template <class Iter>
  void AddFiles(Iter begin, Iter end) {
    for (Iter i = begin; i != end; ++i) {
      AddFile(*i);
    }
  }

 private:
  const bool check_existence_;
  const bool want_abspath_;
  const bool allow_uri_;
};



// ****************************************************************************
// ***  khFilesTransaction
// ***
// ***  Class for changing multiple files in an atomic fashion. Either all
// ***  the changes will be made or none of them will. It accomplishes this
// ***  by using backup files so you don't want to use this class for
// ***  gargantuan files.
// ***
// ***  Usage:
// ***  1) Create a instance you tell it what file extensions to use
// ***  for the new and backup files.
// ***  2) For every file you want to change, write your changes to the
// ***  filesystem with the new file extension and add them new filename to
// ***  the transaction object
// ***  3) Call Commit on the transaction object to have all the new files
// ***  moved into place.
// ***
// ***  Exceptions thrown anywhere along the way (even during the Commit)
// ***  will cause all partially completed work to be rolled back.
// ***
// ***  If you don't commit before the transaction object goes out of scope
// ***  all the file changes will be rolled back.
// ***
// ***  Files added with AddFile() MUST have the new extension already added,
// ***  but that shouldn't be a problem since you are required to write the
// ***  file under that name anyway.
// ****************************************************************************
class khFilesTransaction {
  std::vector<std::string> newlist;
  std::vector<std::string> deletelist;
  std::string newext;
  std::string backupext;
 public:
  khFilesTransaction(const std::string &newext_ = ".new",
                     const std::string &backupext_ = ".bak") :
      newext(newext_), backupext(backupext_) { }
  ~khFilesTransaction(void) throw();

  void AddNewPath(const std::string &path);
  size_t NumNew() const { return newlist.size(); }
  size_t NumDeleted() const { return deletelist.size(); }

  // Can throw!!!
  void DeletePath(const std::string &path);
  bool Commit(void);
};


// ****************************************************************************
// ***  khTmpFileGuard
// ***
// ***  Makes a temporary file. The file is unlinked when the class goes out
// ***  of scope. Uses khTmpFilename (above) to generate the filename.
// ****************************************************************************
class khTmpFileGuard {
 private:
  std::string filename;
  bool dounlink;

 public:
  inline const std::string& name(void) const { return filename; }
  inline void DontUnlink(void) { dounlink = false; }

  // create it in default tmpdir
  khTmpFileGuard(void);

  // create it in specified dir
  explicit khTmpFileGuard(const std::string &dir);

  ~khTmpFileGuard(void);
};

// Given a published DB path, if it fits ([.]*)/([^\*]/key_word/[.]*)
// return $2, else return the string as it is.
// examples of keyword gedb.kda, mapdb.kda
std::string khGetDbSuffix(const std::string& file_path,
                          const std::string& key_word);

// Given a published DB path, if it fits ([.]*)/([^\*]/key_word/[.]*)
// return $1/, else return empty string
// examples of keyword gedb.kda, mapdb.kda
std::string khGetDbPrefix(const std::string& file_path,
                          const std::string& key_word);

// Returns true if it looks like an absolute path to a Fusion DB.
// If return value is true, we return db_type through db_type.
bool khIsAbsoluteDbName(const std::string& file_path,
                        std::string* db_type);

bool khIsDbSuffix(const std::string& file_path, const std::string& key_word);

// If original string is (/gevol/published_dbs/stream_space/[^/]*)/*, then
// return $1 substituting stream_space by search_space;
// Else return empty string.
std::string khGetSearchPrefix(const std::string& file_path);

// If original string is (/gevol/published_dbs/stream_space/[^/]*)/*, then
// return $1;
// Else return empty string.
std::string khGetStreamPrefix(const std::string& file_path);


// Determines if files are equal
// if either file doesn't exist or cannot be read, a dianostic message will
// we written and function will return false.
bool khFilesEqual(const std::string &filename_1,
                  const std::string &filename_2);



#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHFILEUTILS_H_
