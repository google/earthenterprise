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


static char ThisDir[] = ".";

#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gstMisc.h>
#include <gstFileUtils.h>

//
// utility to break apart a filename into it's sub-parts
//
// ie. gstFileInfo fi("/usr/local/lib/libgst.so")
//     fi.dirName() = "/usr/local/lib"
//     fi.fileName() = "libgst.so"
//     fi.baseName() = "libgst"
//     fi.extension() = "so"

gstFileInfo::gstFileInfo()
    : name_(NULL),
      base_name_(NULL),
      file_name_(NULL),
      dir_name_(NULL),
      extension_(NULL) {
  need_stat_ = false;
  status_ = GST_UNKNOWN;
}

//
// input: gstFileInfo fi("/usr/local/lib/libgst.so")
//
gstFileInfo::gstFileInfo(const char* n)
    : base_name_(NULL),
      file_name_(NULL),
      dir_name_(NULL),
      extension_(NULL) {
  name_ = strdupSafe(n);
  if (strlenSafe(n) <= 1) {
    status_ = GST_UNKNOWN;
    need_stat_ = false;
    return;
  }

  char* p = strrchr(name_, '/');

  if (p) {
    dir_name_ = new char[p - name_ + 1];
    strncpy(dir_name_, name_, p - name_);
    dir_name_[p - name_] = '\0';
    file_name_ = strdupSafe(++p);
  } else {
    file_name_ = strdupSafe(name_);
    dir_name_ = ThisDir;
  }

  if (file_name_ && (p = strrchr(file_name_, '.'))) {
    base_name_ = new char[ p - file_name_ + 1 ];
    strncpy(base_name_, file_name_, p - file_name_);
    base_name_[ p - file_name_ ] = '\0';
    extension_ = strDup(++p);
  } else {
    base_name_ = file_name_;
    extension_ = NULL;
  }

  need_stat_ = true;
}

//
// input: gstFileInfo fi("/usr/local/lib", "libgst.so")
//
gstFileInfo::gstFileInfo(const char *p, const char *n)
    : base_name_(NULL),
      file_name_(NULL),
      dir_name_(NULL),
      extension_(NULL) {
  name_ = strdupSafe(p);
  dir_name_ = strdupSafe(p);
  file_name_ = strdupSafe(n);

  char* path;
  if (file_name_ && (path = strchr(file_name_, '.'))) {
    base_name_ = new char[ path - file_name_ + 1 ];
    strncpy(base_name_, file_name_, path - file_name_);
    base_name_[ path - file_name_ ] = '\0';
    extension_ = strDup(++path);
  } else {
    base_name_ = file_name_;
    extension_ = NULL;
  }

  char buf[PATH_MAX];
  snprintf(buf, PATH_MAX - 1, "%s/%s", dir_name_, file_name_);
  SetName(buf);

  need_stat_ = true;
}

//
// input: gstFileInfo fi("/usr/local/lib", "libgst", "so");
//
gstFileInfo::gstFileInfo(const char *p, const char *n, const char *e)
    : base_name_(NULL),
      file_name_(NULL),
      dir_name_(NULL),
      extension_(NULL) {
  name_ = strdupSafe(p);
  dir_name_ = strdupSafe(p);
  base_name_ = strdupSafe(n);
  extension_ = strdupSafe(e);

  char buf[PATH_MAX];
  snprintf(buf, PATH_MAX - 1, "%s.%s", base_name_, extension_);
  file_name_ = strDup(buf);

  snprintf(buf, PATH_MAX - 1, "%s/%s", dir_name_, file_name_);
  SetName(buf);

  need_stat_ = true;
}

gstFileInfo::gstFileInfo(const gstFileInfo& that) {
  name_ = strdupSafe(that.name());
  base_name_ = strDup(that.baseName());
  file_name_ = strDup(that.fileName());
  dir_name_ = strDup(that.dirName());
  extension_ = strDup(that.extension());

  need_stat_ = true;
}

gstFileInfo &gstFileInfo::operator=(const gstFileInfo &that) {
  SetName(that.name());

  if (base_name_ != file_name_)
    delete [] base_name_;

  if (file_name_ != name_)
    delete [] file_name_;

  if (dir_name_ != ThisDir)
    delete [] dir_name_;

  delete [] extension_;

  base_name_ = strDup(that.baseName());
  file_name_ = strDup(that.fileName());
  dir_name_ = strDup(that.dirName());
  extension_ = strDup(that.extension());

  need_stat_ = true;

  return *this;
}

gstFileInfo::~gstFileInfo() {
  if (base_name_ != file_name_)
    delete [] base_name_;

  if (file_name_ != name_)
    delete [] file_name_;

  if (dir_name_ != ThisDir)
    delete [] dir_name_;

  delete [] extension_;
}

//
// swap out the extension, and re-init
//
void gstFileInfo::SetExtension(const char* e) {
  delete [] extension_;

  extension_ = strDup(e);

  char buf[PATH_MAX];
  if (dir_name_ != ThisDir) {
    snprintf(buf, PATH_MAX - 1, "%s/%s.%s", dir_name_, base_name_, extension_);
  } else {
    snprintf(buf, PATH_MAX - 1, "%s.%s", base_name_, extension_);
  }
  SetName(buf);

  need_stat_ = true;
}

void gstFileInfo::SetName(const char* n) {
  if (name_)
    delete [] name_;
  name_ = strdupSafe(n);
}

void gstFileInfo::initstat() {
  if (need_stat_ == false)
    return;

  struct stat64 statinfo;

  if (::stat64(name_, &statinfo) == 0) {
    status_ = GST_OKAY;
    size_ = statinfo.st_size;
    atime_ = statinfo.st_atime;
    mtime_ = statinfo.st_mtime;
    ctime_ = statinfo.st_ctime;
  } else {
    if (errno == EACCES) {
      status_ = GST_PERMISSION_DENIED;
    } else {
      status_ = GST_OPEN_FAIL;
    }
  }

  need_stat_ = false;
}

// -----------------------------------------------------------------------------

gstFileIO::gstFileIO(int fd, ssize_t sz, char* buf)
    : fd_(fd),
      size_(sz) {
  if (buf == NULL) {
    own_buf_ = true;
    buffer_ = new char[sz];
  } else {
    own_buf_ = false;
    buffer_ = buf;
  }
  status_ = GST_OKAY;
}

gstFileIO::~gstFileIO() {
  if (own_buf_)
    delete [] buffer_;
}

gstStatus gstFileIO::write(std::int64_t pos) {
  if (status_ != GST_OKAY)
    return status_;

  if (::lseek64(fd_, pos, SEEK_SET) == -1 ||
      ::write(fd_, buffer_, size_) != size_)
    status_ = GST_WRITE_FAIL;

  return status_;
}

gstStatus gstFileIO::read(std::int64_t pos) {
  if (status_ != GST_OKAY)
    return status_;

  if (::lseek64(fd_, pos, SEEK_SET) == -1 ||
      ::read(fd_, buffer_, size_) != size_)
    status_ = GST_READ_FAIL;

  return status_;
}
