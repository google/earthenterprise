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


/******************************************************************************
File:        ffioReader.h
******************************************************************************/
#ifndef __ffioReader_h
#define __ffioReader_h

#include <string>
#include "ffioIndex.h"
#include "ffioPresenceMask.h"

namespace ffio {
class Reader {
 private:

  ffio::PresenceMask presenceMask;
  mutable khDeleteGuard<IndexReader> indexReader_;
  mutable std::string  openFilename;
  mutable khReadFileCloser fileHandle;
  std::string ffdir_;
 public:
  class FileInfo {
   public:
    std::string filename;
    std::uint64_t beginOffset;
    std::uint64_t endOffset;   // one beyond

    FileInfo(const std::string fname,
             std::uint64_t boff, std::uint64_t eoff)
        : filename(fname), beginOffset(boff), endOffset(eoff) { }
  };
  std::vector<FileInfo> files;

  Reader(const std::string &ffdir);
  ~Reader(void) { Close(); }

  inline const ffio::PresenceMask &presence(void) const {
    return presenceMask;
  }
  inline const std::string &ffdir(void) const { return ffdir_; }
  inline ffio::Type type(void) const {
    OpenIndex();
    return indexReader_->type;
  }
  inline const void* typeData(void) const {
    OpenIndex();
    return indexReader_->typeData;
  }
  const IndexReader* indexReader(void) const {
    OpenIndex();
    return indexReader_;
  }

  void OpenIndex(void) const;
  void Close(void) const;

  bool FindTile(const khTileAddr &addr,
                std::uint32_t &fileIndex, std::uint64_t &fileOffset,
                std::uint32_t &dataLen) const;

  inline bool FindTile(const khTileAddr& addr,
                       std::string &filename, std::uint64_t &fileOffset,
                       std::uint32_t &dataLen) const {
    std::uint32_t fileIndex = 0;
    if (FindTile(addr, fileIndex, fileOffset, dataLen)) {
      filename = files[fileIndex].filename;
      return true;
    } else {
      return false;
    }
  }

  // low level read - used if you already have the result from FindTile
  std::uint32_t ReadBlock(const std::string &filename,
                   std::uint64_t fileOffset, std::uint32_t dataLen,
                   unsigned char *buf) const;

  // Find & read tile into buf. buf will grow if necessary to hold the
  // results. Will return 0 if unable to load the tile.
  std::uint32_t FindReadBlock(const khTileAddr &addr,
                       std::vector<unsigned char> &buf) const;

  inline void PopulateCoverage(khInsetCoverage &cov) const {
    presenceMask.PopulateCoverage(cov);
  }
};
} // namespace ffio

#endif /* __ffioReader_h */
