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
    uint32 totalStoredTiles;

    LevelInfo(const khLevelCoverage &def,
              IndexStorage::TileRecord *tiles_) :
        khLevelCoverage(def),
        tiles(tiles_),
        totalStoredTiles(0)
    { }

    IndexStorage::TileRecord *TileRec(uint32 row, uint32 col) const;
    // Will throw if invalid addr
    void AddTile(uint32 row, uint32 col,
                 uint64 dataOffset,
                 uint32 dataLen);
  };


  Type   type;
  char   typeData[16];
  // Proxy file which is copied to the requested file on destruction.
  geLocalBufferWriteFile proxyFile;
  void* filebuf;
  khMunmapGuard munmapGuard; // must be after proxyFile for destruction
  uint64 totalFFSize;
  uint32 totalIndexSize;
  uint32 totalStoredTiles;

  std::vector<LevelInfo> levelvec;
  LevelInfo* levels[NumFusionLevels];

 public:
  inline uint32 TotalTilesWritten(void) const {
    return totalStoredTiles;
  }

  void SetLittleEndianTypeData(void* typeData_);
  IndexWriter(Type type,
              const std::string &filename,
              const khInsetCoverage &coverage,
              void* littleEndianTypeData = 0);
  ~IndexWriter(void);
  // Will throw if invalid addr
  void AddTile(const khTileAddr &addr, uint32 dataLen, uint32 recordLen);
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


    const IndexStorage::TileRecord *TileRec(uint32 row, uint32 col) const;
    bool HasRowCol(uint32 row, uint32 col) const;
    bool FindTile(uint32 row, uint32 col,
                  uint64 &fileOffset, uint32 &dataLen) const;
  };
  std::vector<LevelInfo> levelvec;
  LevelInfo* levels[NumFusionLevels];

  Type   type;
  char   typeData[16];
  khReadFileCloser fileHandle;
  void* filebuf;
  khMunmapGuard munmapGuard; // must be after fileHandle for destruction
  uint64 totalFFSize;
  uint32 totalIndexSize;
  uint32 totalStoredTiles;

  IndexReader(const std::string &filename);
  ~IndexReader(void);
  bool FindTile(const khTileAddr &addr,
                uint64 &fileOffset, uint32 &dataLen);

  void DumpRecords(const LevelInfo *level) const;

  void PopulateCoverage(khInsetCoverage &cov) const;
};


}

#endif /* __ffioIndex_h */
