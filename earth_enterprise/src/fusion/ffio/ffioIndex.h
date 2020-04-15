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
File:        ffioIndex.h
******************************************************************************/
#ifndef __ffioIndex_h
#define __ffioIndex_h

#include "ffio.h"
#include <string>
#include <khGuard.h>
#include <geFileUtils.h>
#include <khTileAddr.h>
#include <khInsetCoverage.h>

namespace ffio {
namespace IndexStorage {
class Header;
class LevelRecord;
class TileRecord;
}

inline std::string IndexFilename(const std::string &ffdir) {
  return ffdir + "/pack.idx";
}

class IndexWriter {
 public:
  class LevelInfo : public khLevelCoverage {
   public:
    IndexStorage::TileRecord *tiles;
    std::uint32_t totalStoredTiles;

    LevelInfo(const khLevelCoverage &def,
              IndexStorage::TileRecord *tiles_) :
        khLevelCoverage(def),
        tiles(tiles_),
        totalStoredTiles(0)
    { }

    IndexStorage::TileRecord *TileRec(std::uint32_t row, std::uint32_t col) const;
    // Will throw if invalid addr
    void AddTile(std::uint32_t row, std::uint32_t col,
                 std::uint64_t dataOffset,
                 std::uint32_t dataLen);
  };


  Type   type;
  char   typeData[16];
  // Proxy file which is copied to the requested file on destruction.
  geLocalBufferWriteFile proxyFile;
  void* filebuf;
  khMunmapGuard munmapGuard; // must be after proxyFile for destruction
  std::uint64_t totalFFSize;
  std::uint32_t totalIndexSize;
  std::uint32_t totalStoredTiles;

  std::vector<LevelInfo> levelvec;
  LevelInfo* levels[NumFusionLevels];

 public:
  inline std::uint32_t TotalTilesWritten(void) const {
    return totalStoredTiles;
  }

  void SetLittleEndianTypeData(void* typeData_);
  IndexWriter(Type type,
              const std::string &filename,
              const khInsetCoverage &coverage,
              void* littleEndianTypeData = 0);
  ~IndexWriter(void);
  // Will throw if invalid addr
  void AddTile(const khTileAddr &addr, std::uint32_t dataLen, std::uint32_t recordLen);
};

class IndexReader {
 public:
  class LevelInfo : public khLevelCoverage {
   public:
    const IndexStorage::TileRecord *tilerecs;

    LevelInfo(const khLevelCoverage &def,
              const IndexStorage::TileRecord *tiles_) :
        khLevelCoverage(def),
        tilerecs(tiles_)
    { }


    const IndexStorage::TileRecord *TileRec(std::uint32_t row, std::uint32_t col) const;
    bool HasRowCol(std::uint32_t row, std::uint32_t col) const;
    bool FindTile(std::uint32_t row, std::uint32_t col,
                  std::uint64_t &fileOffset, std::uint32_t &dataLen) const;
  };
  std::vector<LevelInfo> levelvec;
  LevelInfo* levels[NumFusionLevels];

  Type   type;
  char   typeData[16];
  khReadFileCloser fileHandle;
  void* filebuf;
  khMunmapGuard munmapGuard; // must be after fileHandle for destruction
  std::uint64_t totalFFSize;
  std::uint32_t totalIndexSize;
  std::uint32_t totalStoredTiles;

  IndexReader(const std::string &filename);
  ~IndexReader(void);
  bool FindTile(const khTileAddr &addr,
                std::uint64_t &fileOffset, std::uint32_t &dataLen);

  void DumpRecords(const LevelInfo *level) const;

  void PopulateCoverage(khInsetCoverage &cov) const;
};


}

#endif /* __ffioIndex_h */
