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


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gstKVPTable.h>
#include <gstRecord.h>
#include <khFileUtils.h>
#include <gstFileUtils.h>
#include <gstJobStats.h>

#define KDB_MAGIC 0xef0120ab
#define KDB_VERSION 1

gstKVPTable::gstKVPTable(const char* n)
    : gstTable(n), fd_(-1) {
  memset(static_cast<void*>(&file_header_), 0, sizeof(FileHeader));

  file_header_.magic = KDB_MAGIC;
  file_header_.version = 1;
  file_header_.indexOffset = sizeof(FileHeader);

  index_map_ = NULL;
  modified_ = false;
  have_header_ = false;
}

gstKVPTable::~gstKVPTable() {
  Close();
}

//
// mode can be either:
//    GST_READONLY,
//    GST_WRITEONLY
//
gstStatus gstKVPTable::Open(gstReadMode mode) {
  // cannot open twice
  if (fd_ != -1)
    return GST_UNKNOWN;

  mode_ = mode;

  switch (mode) {
    case GST_READONLY:
      if (!khExists(name()))
        return GST_OPEN_FAIL;

      fd_ = ::open(name(), O_LARGEFILE | O_RDONLY);
      if (fd_ == -1) {
        notify(NFY_WARN, "Unable to open KVP Table (r): %s", name());
        return GST_OPEN_FAIL;
      }
      return ReadHeader();
      break;

    case GST_WRITEONLY:
      if (khExists(name()))
        fd_ = ::open(name(), O_WRONLY | O_TRUNC | O_LARGEFILE, 0666);
      else
        fd_ = ::open(name(), O_WRONLY | O_CREAT | O_LARGEFILE, 0666);
      if (fd_ == -1) {
        notify(NFY_WARN, "Unable to create new file: %s", name());
        return GST_OPEN_FAIL;
      }
      return GST_OKAY;
      break;

    case GST_READWRITE:
      notify(NFY_WARN, "Unsupported read/write mode for kvp table");
      return GST_UNKNOWN;
      break;
  }

  return GST_OPEN_FAIL;
}

gstStatus gstKVPTable::Close() {
  Flush();

  if (index_map_) {
    free(index_map_);
    index_map_ = NULL;
  }

  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }

  return GST_OKAY;
}

gstStatus gstKVPTable::ReadHeader() {
  //
  // load file header
  //
  if (::read(fd_, &file_header_, sizeof(FileHeader)) != sizeof(FileHeader)) {
    notify(NFY_WARN, "Unable to read file header of source: %s", name());
    return GST_OPEN_FAIL;
  }

  //
  // validate header
  //
  if (file_header_.magic != KDB_MAGIC || file_header_.version != KDB_VERSION) {
    notify(NFY_WARN, "Corrupt file (bad header magic): %s", name());
    return GST_OPEN_FAIL;
  }

  //
  // read record definitions
  //
  char buf[file_header_.fieldDefsSize];
  if (::read(fd_, buf, file_header_.fieldDefsSize)
      != file_header_.fieldDefsSize) {
    notify(NFY_WARN, "Unable to read field definitions of source: %s", name());
    return GST_OPEN_FAIL;
  }

  char* tbuf = buf;
  for (int ii = 0; ii < file_header_.numFields; ++ii) {
    std::uint32_t type = *(reinterpret_cast<std::uint32_t*>(tbuf));
    tbuf += sizeof(std::uint32_t);
    char* name = tbuf;
    header_->addSpec(name, static_cast<int>(type));
    tbuf += strlenSafe(name) + 1;
  }

  return GST_OKAY;
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_GETROW, JOBSTATS_READINDX, JOBSTATS_FILEIO, JOBSTATS_FROMRAW};
static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_GETROW,   "Get Row      "},
  {JOBSTATS_READINDX, "+ Read Index "},
  {JOBSTATS_FILEIO,   "+ File I/O   "},
  {JOBSTATS_FROMRAW,  "+ From Raw   "}
};
gstJobStats* table_stats = new gstJobStats("KVP TBL", JobNames, 4);
#endif

bool gstKVPTable::ReadIndex() {
  JOBSTATS_SCOPED(table_stats, JOBSTATS_READINDX);
  index_map_ = static_cast<RecordPos*>(malloc(sizeof(RecordPos) *
                                              file_header_.numRecords));

  gstFileIO mb(fd_, file_header_.numRecords * sizeof(RecordPos),
               reinterpret_cast<char*>(index_map_));

  if (mb.read(file_header_.indexOffset) != GST_OKAY) {
    notify(NFY_WARN, "Unable to read record index from source: %s", name());
    free(index_map_);
    index_map_ = NULL;
    return false;
  }

  return true;
}

gstRecordHandle gstKVPTable::Row(std::uint32_t row) {
  JOBSTATS_SCOPED(table_stats, JOBSTATS_GETROW);

  gstRecordHandle new_record;

  if (row > file_header_.numRecords) {
    // return NULL;
  } else if (index_map_ == NULL && !ReadIndex()) {
    // return false;
  } else {
    JOBSTATS_BEGIN(table_stats, JOBSTATS_FILEIO);
    gstFileIO mb(fd_, index_map_[row].size);
    gstStatus read_stat = mb.read(index_map_[row].offset);
    JOBSTATS_END(table_stats, JOBSTATS_FILEIO);
    if (read_stat != GST_OKAY) {
      notify(NFY_WARN, "Unable to read row %u from source: %s", row, name());
      // return NULL;
    } else {
      JOBSTATS_BEGIN(table_stats, JOBSTATS_FROMRAW);
      new_record = header_->FromRaw(mb.buffer(), index_map_[row].size);
      JOBSTATS_END(table_stats, JOBSTATS_FROMRAW);
    }
  }

  return new_record;
}

gstStatus gstKVPTable::Flush() {
  if (mode_ != GST_WRITEONLY)
    return GST_UNSUPPORTED;

  if (fd_ == -1)
    return GST_WRITE_FAIL;

  if (modified_) {
    if (::lseek64(fd_, 0, SEEK_SET) == -1 ||
        write(fd_, reinterpret_cast<char*>(&file_header_),
              sizeof(FileHeader)) != sizeof(FileHeader)) {
      notify(NFY_WARN, "Unable to write file header");
      return GST_WRITE_FAIL;
    }

    if (::lseek64(fd_, file_header_.indexOffset, SEEK_SET) == -1
        || write(fd_, reinterpret_cast<char*>(index_array_.array()),
                 file_header_.numRecords * sizeof(RecordPos)) == -1) {
      notify(NFY_WARN, "Unable to write record index");
      return GST_WRITE_FAIL;
    }

    modified_ = false;
  }

  return GST_OKAY;
}

void gstKVPTable::SetHeader(const gstHeaderHandle& hdr) {
  if (have_header_) {
    notify(NFY_WARN, "Unable to add header!");
    return;
  }

  have_header_ = true;

  //
  // compute size of header def block
  //
  int sz = 0;
  for (unsigned int ii = 0; ii < hdr->numColumns(); ++ii) {
    sz += sizeof(std::uint32_t);
    sz += strlenSafe(hdr->Name(ii).utf8()) + 1;
  }

  //
  // fill in raw header defintion
  //
  char buf[sz];
  char* tbuf = buf;
  for (unsigned int ii = 0; ii < hdr->numColumns(); ++ii) {
    *(reinterpret_cast<std::uint32_t*>(tbuf)) = hdr->ftype(ii);
    tbuf += sizeof(std::uint32_t);
    strcpySafe(tbuf, hdr->Name(ii).utf8());
    tbuf += strlenSafe(hdr->Name(ii).utf8()) + 1;
  }

  if (::lseek64(fd_, file_header_.indexOffset, SEEK_SET) == -1 ||
      write(fd_, buf, sz) != sz) {
    notify(NFY_WARN, "Unable to write record header definitions");
    return;
  }

  file_header_.numFields = hdr->numColumns();
  file_header_.fieldDefsSize = sz;
  file_header_.indexOffset += sz;
  Flush();

  gstTable::SetHeader(hdr);
}

gstStatus gstKVPTable::AddRecord(const gstRecordHandle rec) {
  // we must be in write mode to accept new geodes
  if (mode_ != GST_WRITEONLY) {
    notify(NFY_WARN, "Unsupported writing to a READONLY file. ");
    return GST_UNSUPPORTED;
  }

  RecordPos newpos;
  newpos.offset = file_header_.indexOffset;
  newpos.size = header_->RawSize(rec);
  newpos.filler = 0;

  char buf[newpos.size];
  header_->ToRaw(rec, buf);

  //
  // write new record at the index position
  // and slide index to end of this new record
  //

  gstFileIO mb(fd_, newpos.size, buf);
  if (mb.write(newpos.offset) != GST_OKAY) {
    notify(NFY_WARN, "Cannot write new record to file %s", name());
    return GST_WRITE_FAIL;
  }

  file_header_.indexOffset += newpos.size;
  index_array_.append(newpos);
  file_header_.numRecords = index_array_.length();

  modified_ = true;

  return GST_OKAY;
}

