/*
 * Copyright 2017 Google Inc.
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

#ifndef KHSRC_FUSION_GST_GSTFILEUTILS_H__
#define KHSRC_FUSION_GST_GSTFILEUTILS_H__

#include <gstTypes.h>
#include <notify.h>
#include <time.h>

class gstFileInfo {
 public:
  gstFileInfo();
  gstFileInfo(const char* n);
  gstFileInfo(const char* p, const char* n);
  gstFileInfo(const char* p, const char* n, const char* e);
  gstFileInfo(const gstFileInfo &that);
  ~gstFileInfo();

  gstFileInfo &operator=(const gstFileInfo &that);

  const char* name() const { return name_; }
  const char* baseName() const { return base_name_; }
  const char* fileName() const { return file_name_; }
  const char* dirName() const { return dir_name_; }
  const char* extension() const { return extension_; }

  void SetExtension(const char*);

  gstStatus status() { initstat(); return status_; }
  std::int64_t size() { initstat(); return size_; }
  time_t atime() { initstat(); return atime_; }
  time_t mtime() { initstat(); return mtime_; }
  time_t ctime() { initstat(); return ctime_; }

 private:
  void initstat();
  void SetName(const char* n);

  char* name_;
  char* base_name_;
  char* file_name_;
  char* dir_name_;
  char* extension_;

  gstStatus status_;

  // taken straight from the stat structure
  std::int64_t size_;     // total size, in bytes
  time_t atime_;   // time of last access
  time_t mtime_;   // time of last modification
  time_t ctime_;   // time of last change

  bool need_stat_;
};


class gstFileIO {
 public:
  gstFileIO(int, ssize_t sz, char* buf = nullptr);
  ~gstFileIO();

  gstFileIO(const gstFileIO&) = delete;
  gstFileIO(gstFileIO&&) = delete;
  gstFileIO& operator=(const gstFileIO&) = delete;
  gstFileIO& operator=(gstFileIO&&) = delete;

  gstStatus write(std::int64_t pos);
  gstStatus read(std::int64_t pos);

  gstStatus status() const { return status_; }

  char* buffer() const { return buffer_; }

 private:
  gstStatus status_;
  int fd_;
  ssize_t size_;
  char* buffer_;
  bool own_buf_;
};

#endif  // !KHSRC_FUSION_GST_GSTFILEUTILS_H__
