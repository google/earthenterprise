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


#include <alloca.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include <gstTXTTable.h>
#include <notify.h>
#include <gstRecord.h>
#include <khGuard.h>
#include <khFileUtils.h>

static const char* kIndexExtension = ".ksx";

gstTXTTable::gstTXTTable(const char* n)
    : gstTable(n),
      file_descriptor_(-1) {
  SetFileType(Undefined);
  status_ = GST_UNKNOWN;
  skip_rows_ = 0;
}

gstTXTTable::~gstTXTTable() {
  Close();
}

bool gstTXTTable::SetFileType(const char* ftype) {
  if (!strcasecmp(ftype, "delimited")) {
    SetFileType(Delimited);
  } else if (!strcasecmp(ftype, "fixedwidth")) {
    SetFileType(FixedWidth);
  } else {
    return false;
  }

  return true;
}

gstStatus gstTXTTable::Open(gstReadMode read_mode) {
  read_mode_ = read_mode;

  //
  // get the stat structure of this file to make a few guesses
  //
  if (!khExists(name())) {
    notify(NFY_WARN, "Text file \"%s\" doesn't appear to exist", name());
    return GST_OPEN_FAIL;
  }

  if ((file_descriptor_ = ::open64(name(), O_RDONLY | O_NONBLOCK)) == -1) {
    notify(NFY_WARN, "Unable to open txt file %s", name());
    return GST_OPEN_FAIL;
  }

  //
  // try to use existing index file, otherwise build a new one
  //
  if (LoadIndex() != GST_OKAY && BuildIndex() != GST_OKAY) {
    notify(NFY_WARN, "Unable to build index of txt file %s", name());
    ::close(file_descriptor_);
    file_descriptor_ = -1;
    return status_;
  }

  return (status_ = GST_OKAY);
}


gstStatus gstTXTTable::Close() {
  if (file_descriptor_ != -1) {
    if (::close(file_descriptor_) != 0) {
      notify(NFY_WARN, "Problems closing txt file");
      return (status_ = GST_UNKNOWN);
    }
    file_descriptor_ = -1;
  }

  return (status_ = GST_OKAY);
}


class CloseFileCatcher {
 public:
  CloseFileCatcher(int fd) : file_desc_(fd) { }
  ~CloseFileCatcher() { ::close(file_desc_); }
  int file_desc_;
};

gstStatus gstTXTTable::LoadIndex() {
  std::string index_file = khReplaceExtension(name(), kIndexExtension);
  if (!khExists(index_file))
    return GST_OPEN_FAIL;
  std::uint64_t size;
  time_t mtime;
  if (!khGetFileInfo(name(), size, mtime)) {
    return GST_OPEN_FAIL;
  }

  int indexfd;
  if ((indexfd = ::open(index_file.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
    notify(NFY_INFO, "Unable to open txt index file %s", index_file.c_str());
    return GST_OPEN_FAIL;
  }

  CloseFileCatcher fdcatch(indexfd);

  //
  // read index header
  //
  IndexHeader hdr;
  if (::read(indexfd, static_cast<void*>(&hdr), sizeof(IndexHeader)) !=
      sizeof(IndexHeader)) {
    notify(NFY_WARN, "Unable to read header of txt index file %s", index_file.c_str());
    return GST_READ_FAIL;
  }

  //
  // verify magic
  //
  if (memcmp(hdr.magic, TXT_INDEX_MAGIC, 8) != 0) {
    notify(NFY_WARN, "Corrupted txt index file %s", index_file.c_str());
    return GST_READ_FAIL;
  }

  //
  // verify modification time
  //
  if (hdr.mtime != mtime) {
    notify(NFY_WARN,
           "Source txt table has been modified, invalidating index %s",
           index_file.c_str());
    if (!khUnlink(index_file.c_str()))
      notify(NFY_WARN, "** Unable to remove!");
    return GST_UNKNOWN;
  }

  record_index_.resize(hdr.numrows * sizeof(off64_t));

  //
  // read entire index into memory
  //
  if (::lseek64(indexfd, hdr.offset, SEEK_SET) == -1) {
    notify(NFY_WARN, "Unable to seek txt index file %s", index_file.c_str());
    return GST_READ_FAIL;
  }

  if (::read(indexfd, &record_index_[0], hdr.numrows * sizeof(off64_t)) !=
      ssize_t(hdr.numrows * sizeof(off64_t))) {
    notify(NFY_WARN, "Unable to read txt index file %s", index_file.c_str());
    return GST_READ_FAIL;
  }

  num_rows_ = record_index_.size();

  return GST_OKAY;
}


char* FindNextChar(char c, char* start_pos, char* end_pos) {
  for (char* pos = start_pos; pos < end_pos; ++pos) {
    if (*pos == c)
      return pos;
  }
  return NULL;
}

gstStatus gstTXTTable::BuildIndex() {
  const size_t buffsize = 8192;

  // scan entire file to count new lines
  ssize_t bytes_read = 0;
  char buf[buffsize];

  if (::lseek64(file_descriptor_, 0, SEEK_SET) == -1)
    return (status_ = GST_READ_FAIL);

  off64_t block_start = 0;
  off64_t line_start = 0;

  while ((bytes_read = ::read(file_descriptor_, static_cast<void*>(buf),
                              buffsize)) > 0) {
    char* pos = buf;
    while ((pos = FindNextChar('\n', pos, buf + bytes_read)) != NULL) {
      if (skip_rows_ == 0) {
        // skip any totally blank line
        unsigned int size = record_index_.size();
        if (size && record_index_[size - 1] == line_start - 1) {
          record_index_[size - 1] = line_start;
        } else {
          record_index_.push_back(line_start);
        }
      } else {
        --skip_rows_;
      }
      ++pos;
      line_start = block_start + (pos - buf);
    }
    block_start += bytes_read;
  }
  num_rows_ = record_index_.size();

  // sanity check...
  if (record_index_.size() == 0) {
    num_columns_ = 0;
    return (status_ = GST_READ_FAIL);
  }

  notify(NFY_DEBUG, "Indexed %llu rows of file %s",
         static_cast<long long unsigned>(record_index_.size()), name());

  if (read_mode_ == GST_READONLY)
    return (status_ = GST_OKAY);

  //
  // save out index file, if possible
  //
  std::string index_file = khReplaceExtension(name(), kIndexExtension);
  int indexfd;
  if ((indexfd = ::open(index_file.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
    notify(NFY_WARN, "Unable to write index file %s", index_file.c_str());
    // this is not a fatal error.  we still have the index in memory
    return (status_ = GST_OKAY);
  }

  CloseFileCatcher fdcatch(indexfd);

  std::uint64_t size;
  time_t mtime;
  if (!khGetFileInfo(name(), size, mtime)) {
    return (status_ = GST_WRITE_FAIL);
  }

  IndexHeader hdr;
  memcpy(hdr.magic, TXT_INDEX_MAGIC, 8);
  hdr.numrows = record_index_.size();
  hdr.numcols = 0;    // currently unused
  hdr.mtime = mtime;
  hdr.offset = sizeof(hdr);

  if (::write(indexfd, &hdr, sizeof(IndexHeader)) != sizeof(IndexHeader)) {
    notify(NFY_WARN, "Unable to write header to txt index file %s",
           index_file.c_str());
    return (status_ = GST_WRITE_FAIL);
  }

  if (::write(indexfd, &record_index_[0], record_index_.size() * sizeof(off64_t)) !=
      ssize_t(record_index_.size() * sizeof(off64_t))) {
    notify(NFY_WARN, "Unable to write txt index file %s", index_file.c_str());
    return (status_ = GST_WRITE_FAIL);
  }

  return (status_ = GST_OKAY);
}


bool gstTXTTable::Readline(std::uint32_t row) {
  if (status_ != GST_OKAY)
    return -1;

  if (row < 0 || row >= record_index_.size()) {
    notify(NFY_WARN, "Row %u is outside the valid range of 0-%lu for file %s",
           row, static_cast<long unsigned>(record_index_.size() - 1), name());
    status_ = GST_READ_FAIL;
    return -1;
  }

  // determine the size of this row, where the last row is a special case
  size_t count = 0;
  if (row == record_index_.size() - 1) {
    off64_t end = ::lseek64(file_descriptor_, 0, SEEK_END);
    count = end - record_index_[row];
  } else {
    count = record_index_[row + 1] - record_index_[row];
  }

  if (::lseek64(file_descriptor_, record_index_[row], SEEK_SET) == -1) {
    notify(NFY_WARN, "Failed to seek to file position: %llu",
           static_cast<long long unsigned>(record_index_[row]));
    status_ = GST_READ_FAIL;
    return -1;
  }

  row_buffer_.resize(count);

  ssize_t bytes_read = ::read(file_descriptor_, &row_buffer_[0], count);
  if (bytes_read == 0) {
    notify(NFY_WARN, "Read 0 bytes for row: %u (offset:%lld)", row,
           static_cast<long long unsigned>(record_index_[row]));
    status_ = GST_READ_FAIL;
    return -1;
  }

  if (bytes_read != (ssize_t)count) {
    notify(NFY_WARN, "Unable to read all bytes for row %d", row);
    return false;
  }

  return true;
}

//
// read a single record(row) from the file and format it into
// the provided gstRecord(rec)
//
gstRecordHandle gstTXTTable::Row(std::uint32_t row) {
  if (!Readline(row)) {
    status_ = GST_READ_FAIL;
    return gstRecordHandle();
  }

  char* buff = &row_buffer_[0];
  char* end = &row_buffer_[row_buffer_.size() - 1];

  if (end == NULL) {
    notify(NFY_WARN, "No trailing new line for row: %d", row);
    status_ = GST_READ_FAIL;
    return gstRecordHandle();
  }

  unsigned int field = 0;
  if (file_type_ == Delimited) {
    gstRecordHandle new_rec = NewRecord();
    while (end > buff && isspace(*(end - 1)))
      end--;
    *end = '\0';

    char *pos = &buff[0];

    while (pos < end) {
      if (field == new_rec->NumFields()) {
        notify(NFY_WARN, "Row %d has more fields than specified.", row);
        break;
      }

      // char *stop = end;

      // special case if field begins with double-quotes
      if (*pos == '\"') {
        // crawl along field looking for end double-quotes
        // taking special care to look for a pair of double-quotes,
        // which will be interpreted to mean the double-quote is part of
        // the field
        char tmpbuf[end - pos + 1];
        char* out = &tmpbuf[0];
        ++pos;
        while (pos < end - 1) {
          // normal characters pass through
          if (*pos != '"') {
            *out = *pos;
            out++;
            pos++;
            // a single double quote closes the string,
            // and a pair means take just a single double-quote
          } else {
            if (*(pos + 1) == '"') {
              *out = '"';
              out++;
              pos += 2;
            } else {
              break;
            }
          }
        }

        *out = '\0';
        new_rec->Field(field)->set(reinterpret_cast<char*>(tmpbuf));

        // get past the final closing double-quotes, and delimiter
        pos += 2;
      } else {
        char* stop = static_cast<char*>(memchr(static_cast<void*>(pos),
                                               delimiter_, end - pos));
        if (stop == NULL)
          stop = end;
        new_rec->Field(field)->set(static_cast<char*>(pos),
                                   static_cast<int>(stop - pos));
        pos = stop + 1;
      }

      ++field;
    }
    return new_rec;
  } else if (file_type_ == FixedWidth) {
    gstRecordHandle new_rec = NewRecord();
    gstHeaderHandle hdr = GetHeader();
    char* pos = &buff[0];
    for (unsigned int ii = 0; ii < hdr->numColumns(); ++ii) {
      new_rec->Field(ii)->set(reinterpret_cast<char*>(pos), hdr->length(ii));
      if (hdr->mult(ii) != 0)
        *(new_rec->Field(ii)) *= gstValue(hdr->mult(ii));
      pos += hdr->length(ii);
    }
    return new_rec;
  } else {
    notify(NFY_WARN, "Unknown FileType\n");
    status_ = GST_READ_FAIL;
    return gstRecordHandle();
  }
}
