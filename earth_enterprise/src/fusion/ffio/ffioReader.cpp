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


/******************************************************************************
File:        ffioReader.cpp
******************************************************************************/
#include "ffioReader.h"
#include <khFileUtils.h>
#include <notify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>


ffio::Reader::Reader(const std::string &ffdir)
    : presenceMask(ffio::PresenceFilename(ffdir)),
      indexReader_(),
      ffdir_(ffdir)
{
  std::uint64_t runningOffset = 0;

  for (unsigned int i = 0; ; ++i) {
    const size_t fnum_len = 64;
    char fnum[fnum_len];
    snprintf(fnum, fnum_len, "%02d", i);
    std::string fname = ffdir + "/pack." + fnum;

    std::uint64_t size;
    time_t mtime;
    if (!khGetFileInfo(fname, size, mtime))
      break;
    files.push_back(FileInfo(fname, runningOffset,
                             runningOffset+size));
    runningOffset += size;
  }
}


bool
ffio::Reader::FindTile(const khTileAddr &addr,
                       std::uint32_t &fileIndex, std::uint64_t &fileOffset,
                       std::uint32_t &dataLen) const
{
  std::uint64_t beginOffset = 0;
  std::uint32_t len = 0;
  if (presenceMask.GetPresence(addr)) {
    OpenIndex();
    if (indexReader_->FindTile(addr, beginOffset, len)) {
      std::uint64_t endOffset = beginOffset + len; // one beyond
      for (unsigned int i = 0; i < files.size(); ++i) {
        const FileInfo &file(files[i]);
        if ((beginOffset >= file.beginOffset) &&
            (endOffset   <= file.endOffset)) {
          fileIndex  = i;
          fileOffset = beginOffset - file.beginOffset;
          dataLen    = len;
          return true;
        }
      }
      notify(NFY_WARN,
             "Internal Error: Indexed tile location not in raw files: %llu, %u",
             static_cast<long long unsigned>(beginOffset), len);
      notify(NFY_WARN,
             "\t%s: (lrc) %u,%u,%u", ffdir_.c_str(),
             addr.level, addr.row, addr.col);
    }
  }
  return false;
}


std::uint32_t
ffio::Reader::FindReadBlock(const khTileAddr &addr,
                            std::vector<unsigned char> &buf) const
{
  std::string filename;
  std::uint64_t fileOffset = 0;
  std::uint32_t dataLen = 0;
  if (FindTile(addr, filename, fileOffset, dataLen)) {
    buf.resize(dataLen);

    return ReadBlock(filename, fileOffset, dataLen, &buf[0]);
  } else {
    notify(NFY_WARN,
           "Unable to find tile (lrc) %u,%u,%u in %s",
           addr.level, addr.row, addr.col, ffdir_.c_str());
    return 0;
  }
}

std::uint32_t
ffio::Reader::ReadBlock(const std::string &filename,
                        std::uint64_t fileOffset, std::uint32_t dataLen,
                        unsigned char *buf) const
{
  if (!fileHandle.valid() || (openFilename != filename)) {
    fileHandle = ::open(filename.c_str(), O_RDONLY|O_LARGEFILE);
    if (!fileHandle.valid()) {
      openFilename = std::string();
      notify(NFY_WARN, "Unable to open %s: %s",
             filename.c_str(), khstrerror(errno).c_str());
      return 0;
    }
    openFilename = filename;
  }

  if (lseek64(fileHandle.fd(), (off64_t)fileOffset, SEEK_SET) < 0) {
    notify(NFY_WARN, "Unable to seek to %llu in %s: %s",
           static_cast<long long unsigned>(fileOffset), filename.c_str(),
           khstrerror(errno).c_str());
    return 0;
  }
  if (!khReadAll(fileHandle.fd(), &buf[0], dataLen)) {
    notify(NFY_WARN,
           "Unable to read %u bytes from offset %llu in %s: %s",
           dataLen, static_cast<long long unsigned>(fileOffset),
           filename.c_str(), khstrerror(errno).c_str());
    return 0;
  }
  return dataLen;
}


void
ffio::Reader::OpenIndex(void) const
{
  if (!indexReader_) {
    indexReader_ = TransferOwnership(
        new ffio::IndexReader(ffio::IndexFilename(ffdir())));
  }
}


void
ffio::Reader::Close(void) const
{
  // guard will free old one
  indexReader_.clear();

  // guard will free old one
  fileHandle.close();
  openFilename = std::string();
}
