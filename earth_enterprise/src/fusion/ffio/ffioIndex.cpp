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


/******************************************************************************
File:        ffioIndex.cpp
******************************************************************************/
#include "ffioIndex.h"
#include "ff.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <khEndian.h>
#include <khException.h>
#include <geFileUtils.h>
#include <khFileUtils.h>
#include <autoingest/MiscConfig.h>

namespace ffio {
namespace IndexStorage {
// Low level classes written to disk as the flat file index

// The header says how many LevelRecords exist
// Each LevelRecord defines the extents and offset of its TileRecords
// The header also says how big the entire index is (sanity check?).
// The TileRecords point to the actual data (not the FFRecHeader)
/*
  -------------------------------------------------------------
  |  Header                                  =================|=======+
  +-----------------------------------------------------------+       :
  |  LevelRecord  ==+                                         |       :
  +-----------------:-----------------------------------------+       :
  |  LevelRecord  ==:======+                                  |       :
  +-----------------:------:----------------------------------+       :
  |  LevelRecord  ==:======:====+                             |       :
  +-----------------:------:----:-----------------------------+       :
  |     .           :      :    :                             |       :
  |     .           :      :    :                             |       :
  |     .           :      :    :                             |       :
  +-----------------v------:----:-----------------------------+       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR : TR : TR | TR | TR | TR | TR | TR |       :
  +------------------------v----:-----------------------------+       :
  | TR | TR | TR | TR | TR | TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR | TR : TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR | TR : TR | TR | TR | TR | TR | TR |       :
  +-----------------------------v-----------------------------+       :
  | TR | TR | TR | TR | TR | TR | TR | TR | TR | TR | TR | TR |       :
  | TR | TR | TR | TR | TR | TR | TR | TR | TR | TR | TR | TR |       :
  +-----------------------------------------------------------+       :
  |     .                                                     |       :
  |     .                                                     |       :
  |     .                                                     |       :
  +-----------------------------------------------------------+   <---+
*/

// At the time this code was written, 100% of our processing machines
// were LittleEndian and were expected to remain that way for the
// foreseable future. Accordingly, this format is LittleEndian. Note
// that this differs from the flatfiles that this indexes. They are
// BigEndian. If you're using the public APIs, the endianess of this
// file should be completely hidden.


// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 64 bytes long.
class Header {
 public:
  static const char themagic[];

  char   magic[23];
  uint8  indexFormatVersion;
  uint64 totalFFSize;
  uint32 totalIndexSize;
  uint32 totalStoredTiles;
  uint8  type;                // ffio::Type
  uint8  numLevels;
  uint8  unused1;             // == 0
  uint8  unused2;             // == 0
  uint32 unused3;             // == 0
  char   typeData[16];    // used by various type specializations

  Header(uint32 totalIndexSize_,
         ffio::Type type_,
         uint   numLevels_,
         void* littleEndianTypeData,
         uint64 totalFFSize_,
         uint32 totalStoredtiles_);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }

  // should only be used by index reader
  Header(void) { }
};
COMPILE_TIME_CHECK(sizeof(Header) == 64, BadFFIOIndexHeaderSize);

// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 32 bytes long.
class LevelRecord {
 public:
  uint32 tileRecordsOffset;
  uint32 totalStoredTiles;
  uint32 startRow;
  uint32 startCol;
  uint32 numRows;
  uint32 numCols;
  uint8  levelNum;
  uint8  unused1;             // == 0
  uint16 unused2;             // == 0
  uint32 unused3;             // == 0

  LevelRecord(uint32 tileRecordsOffset_,
              const khExtents<uint32> &tileExtents,
              uint   levelNum_,
              uint32 totalStoredTiles_);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }
};
COMPILE_TIME_CHECK(sizeof(LevelRecord) == 32, BadFFIOIndexLevelRecordSize);

// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 16 bytes long.
class TileRecord {
 public:
  uint64 dataOffset;
  uint32 dataLen;
  uint32 unused1;             // == 0

  TileRecord(uint64 dataOffset_, uint32 dataLen_);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }
};
COMPILE_TIME_CHECK(sizeof(TileRecord) == 16, BadFFIOIndexTileRecordSize);
} // namespace IndexStorage
} // namespace ffio


const char
ffio::IndexStorage::Header::themagic[] = "Keyhole Flatfile Index";
COMPILE_TIME_CHECK(sizeof(ffio::IndexStorage::Header::themagic)-1 ==
                   sizeof(ffio::IndexStorage::Header().magic),
                   InvalidMagicSize)

    ffio::IndexStorage::Header::Header(uint32 totalIndexSize_,
                                       ffio::Type type_,
                                       uint   numLevels_,
                                       void* littleEndianTypeData,
                                       uint64 totalFFSize_,
                                       uint32 totalStoredTiles_) :
        indexFormatVersion(1),
        totalFFSize(totalFFSize_),
        totalIndexSize(totalIndexSize_),
        totalStoredTiles(totalStoredTiles_),
        type(type_),
        numLevels(numLevels_),
        unused1(0),
        unused2(0),
        unused3(0)
{
  memcpy(magic, themagic, sizeof(magic));
  memcpy(typeData, littleEndianTypeData, sizeof(typeData));
}

void
ffio::IndexStorage::Header::LittleEndianToHost(void) {
  // magic -- no need to swap
  indexFormatVersion = ::LittleEndianToHost(indexFormatVersion);
  totalFFSize        = ::LittleEndianToHost(totalFFSize);
  totalIndexSize     = ::LittleEndianToHost(totalIndexSize);
  totalStoredTiles   = ::LittleEndianToHost(totalStoredTiles);
  type               = ::LittleEndianToHost(type);
  numLevels          = ::LittleEndianToHost(numLevels);

  // Go ahead and swap the unused ones as well, the cost is minimal and it
  // protects against future mistakes when these do get used.
  unused1            = ::LittleEndianToHost(unused1);
  unused2            = ::LittleEndianToHost(unused2);
  unused3            = ::LittleEndianToHost(unused3);

  // Don't swap typeData, the subtype already made it little endian before
  // giving it to me
}


ffio::IndexStorage::LevelRecord::LevelRecord
(uint32 tileRecordsOffset_,
 const khExtents<uint32> &tileExtents,
 uint   levelNum_,
 uint32 totalStoredTiles_) :
    tileRecordsOffset(tileRecordsOffset_),
    totalStoredTiles(totalStoredTiles_),
    startRow(tileExtents.beginRow()),
    startCol(tileExtents.beginCol()),
    numRows(tileExtents.height()),
    numCols(tileExtents.width()),
    levelNum(levelNum_),
    unused1(0),
    unused2(0),
    unused3(0)
{
}

void
ffio::IndexStorage::LevelRecord::LittleEndianToHost(void) {
  tileRecordsOffset = ::LittleEndianToHost(tileRecordsOffset);
  totalStoredTiles  = ::LittleEndianToHost(totalStoredTiles);
  numRows       = ::LittleEndianToHost(numRows);
  numCols       = ::LittleEndianToHost(numCols);
  startRow      = ::LittleEndianToHost(startRow);
  startCol      = ::LittleEndianToHost(startCol);
  levelNum      = ::LittleEndianToHost(levelNum);

  // Go ahead and swap the unused ones as well, the cost is minimal and it
  // protects against future mistakes when these do get used.
  unused1            = ::LittleEndianToHost(unused1);
  unused2            = ::LittleEndianToHost(unused2);
  unused3            = ::LittleEndianToHost(unused3);
}


ffio::IndexStorage::TileRecord::TileRecord(uint64 dataOffset_,
                                           uint32 dataLen_) :
    dataOffset(dataOffset_),
    dataLen(dataLen_),
    unused1(0)
{
}

void
ffio::IndexStorage::TileRecord::LittleEndianToHost(void) {
  dataOffset  = ::LittleEndianToHost(dataOffset);
  dataLen     = ::LittleEndianToHost(dataLen);

  // Go ahead and swap the unused ones as well, the cost is minimal and it
  // protects against future mistakes when these do get used.
  unused1            = ::LittleEndianToHost(unused1);
}




// ****************************************************************************
// ***  IndexWriter::LevelInfo
// ****************************************************************************
// Fills in TileRecord, updates count
// Will throw if invalid addr
void
ffio::IndexWriter::LevelInfo::AddTile(uint32 row, uint32 col,
                                      uint64 dataOffset,
                                      uint32 dataLen)
{
  if (!extents.ContainsRowCol(row, col)) {
    throw khException(kh::tr("Internal Error: Invalid row/col"));
  }
  if (dataLen == 0) {
    throw khException(kh::tr("Internal Error: Invalid data length"));
  }

  IndexStorage::TileRecord *tile = TileRec(row, col);
  (void) new(tile) ffio::IndexStorage::TileRecord(dataOffset, dataLen);
  tile->HostToLittleEndian();
  ++totalStoredTiles;
}

ffio::IndexStorage::TileRecord *
ffio::IndexWriter::LevelInfo::TileRec(uint32 row, uint32 col) const
{
  return tiles +
    ((row - extents.beginRow()) * extents.width()) +
    (col - extents.beginCol());
}


// ****************************************************************************
// ***  IndexWriter
// ****************************************************************************
// creates file, mmaps it,
// zeros entire file (Header, LevelRecords & TileRecords)
ffio::IndexWriter::IndexWriter(Type type_,
                               const std::string &filename,
                               const khInsetCoverage &coverage,
                               void* littleEndianTypeData) :
    type(type_),
    filebuf(0),
    totalFFSize(0),
    totalIndexSize(0),
    totalStoredTiles(0)
{
  // calculate totalIndexSize
  totalIndexSize = sizeof(ffio::IndexStorage::Header) +
                   coverage.numLevels() * sizeof(ffio::IndexStorage::LevelRecord);

  for (uint lev = coverage.beginLevel(); lev < coverage.endLevel(); ++lev) {
    const khExtents<uint32> &levExtents(coverage.levelExtents(lev));
    assert(levExtents.width());
    assert(levExtents.height());
#ifndef NDEBUG
    uint32 endpos = (0x1U << lev);
#endif
    assert(levExtents.beginRow() < endpos);
    assert(levExtents.beginCol() < endpos);
    assert(levExtents.endRow() <= endpos);
    assert(levExtents.endCol() <= endpos);
    totalIndexSize += levExtents.width() *
                      levExtents.height() *
                      sizeof(ffio::IndexStorage::TileRecord);
  }

  // Open file, mmap it, and zero it out
  // Use local tmp file if indicated (i.e. for NFS async mode)
  (void) ::umask(000);
  proxyFile.Open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
  int fd = proxyFile.FileId();

  // presize & fill w/ 0's
  khFillFile(fd, 0, totalIndexSize);

  filebuf = mmap(0, totalIndexSize, PROT_READ|PROT_WRITE,
                 MAP_SHARED, fd, 0);
  if (filebuf == MAP_FAILED) {
    throw khErrnoException(kh::tr("Unable to mmap %1").arg(filename));
  }
  munmapGuard.DelayedSet(filebuf, totalIndexSize);


  // populate levels
  uint32 tmpSize = sizeof(ffio::IndexStorage::Header) +
                   coverage.numLevels() * sizeof(ffio::IndexStorage::LevelRecord);
  memset(levels, 0, sizeof(levels));
  levelvec.reserve(coverage.numLevels());
  for (uint lev = coverage.beginLevel(); lev < coverage.endLevel(); ++lev) {
    khLevelCoverage levCov(coverage.levelCoverage(lev));
    assert(lev <= MaxFusionLevel);
    levelvec.push_back
      (LevelInfo
       (levCov,
        (ffio::IndexStorage::TileRecord*)(((uchar*)filebuf)+tmpSize)));
    // safe to store the address to an element in the vector since
    // we pre-sized the vector
    levels[lev] = &levelvec[levelvec.size()-1];
    tmpSize += levCov.extents.width() *
               levCov.extents.height() *
               sizeof(ffio::IndexStorage::TileRecord);
  }

  if (littleEndianTypeData) {
    memcpy(typeData, littleEndianTypeData, sizeof(typeData));
  } else {
    memset(typeData, 0, sizeof(typeData));
  }
}

void
ffio::IndexWriter::SetLittleEndianTypeData(void* typeData_) {
  memcpy(typeData, typeData_, sizeof(typeData));
}


// writes Header & LevelRecords, munmaps file
ffio::IndexWriter::~IndexWriter(void)
{
  // write the header
  ffio::IndexStorage::Header *header =
    new(filebuf) ffio::IndexStorage::Header(totalIndexSize, type,
                                            levelvec.size(),
                                            typeData,
                                            totalFFSize,
                                            totalStoredTiles);
#if 0
  printf("indexFormatVersion = %d\n", header->indexFormatVersion);
  printf("totalFFSize = %llu\n", header->totalFFSize);
  printf("totalIndexSize = %u\n", header->totalIndexSize);
  printf("totalStoredTiles = %u\n", header->totalStoredTiles);
  printf("type = %u\n", header->type);
  printf("numLevels = %u\n", header->numLevels);
#endif
  header->HostToLittleEndian();

  // write the LevelRecords
  ffio::IndexStorage::LevelRecord *lrec =
    (ffio::IndexStorage::LevelRecord*)(header+1);
  for (std::vector<LevelInfo>::const_iterator li = levelvec.begin();
       li != levelvec.end(); ++li) {
    (void) new(lrec)
      ffio::IndexStorage::LevelRecord((char*)li->tiles - (char*)filebuf,
                                      li->extents,
                                      li->level,
                                      li->totalStoredTiles);
#if 0
    printf("    ----- level %d -----\n", lrec->levelNum);
    printf("    tileRecordsOffset = %u\n", lrec->tileRecordsOffset);
    printf("    totalStoredTiles = %u\n", lrec->totalStoredTiles);
    printf("    startRow = %u\n", lrec->startRow);
    printf("    startCol = %u\n", lrec->startCol);
    printf("    numRows = %u\n", lrec->numRows);
    printf("    numCols = %u\n", lrec->numCols);
#endif
    lrec->HostToLittleEndian();
    ++lrec;
  }

  // munmapGuard destructor will call msync and munmap
}


// fills in TileRecord, updates counts & sizes
// Will throw if invalid addr
void
ffio::IndexWriter::AddTile(const khTileAddr &addr,
                           uint32 dataLen, uint32 recordLen)
{
  assert(addr.level <= MaxFusionLevel);
  assert(levels[addr.level]);
  levels[addr.level]->AddTile(addr.row, addr.col,
                              totalFFSize+sizeof(FFRecHeader), dataLen);
  ++totalStoredTiles;
  totalFFSize += recordLen;
}


// ****************************************************************************
// ***  ffio::IndexReader
// ****************************************************************************
ffio::IndexReader::IndexReader(const std::string &filename)
{
  // since these files can be very small, sometimes kernels and
  // nfs servers in between the writer and the reader can delay the writing.
  // Let's wait until the file is at least a little bit cool. :-)
  WaitIfFileIsTooNew(filename, MiscConfig::Instance().NFSVisibilityDelay);

  // open the file, read & verify the header
  fileHandle = ::open(filename.c_str(), O_RDONLY);
  if (fileHandle.fd() < 0) {
    throw khErrnoException(kh::tr("Unable to open index %1").arg(filename));
  }
  IndexStorage::Header header;
  if (!khReadAll(fileHandle.fd(), &header, sizeof(header))) {
    throw khErrnoException(kh::tr("Unable to read index header %1").
                           arg(filename));
  }
  header.LittleEndianToHost();

  if (memcmp(header.magic, IndexStorage::Header::themagic,
             sizeof(header.magic)) != 0) {
    throw khException(kh::tr("Corrupted index header %1").
                      arg(filename));
  }

  if (header.indexFormatVersion == 1) {
    type             = (ffio::Type)header.type;
    totalFFSize      = header.totalFFSize;
    totalIndexSize   = header.totalIndexSize;
    totalStoredTiles = header.totalStoredTiles;
    COMPILE_TIME_ASSERT(sizeof(typeData) == sizeof(header.typeData),
                        InvalidTypeDataSize);
    memcpy(typeData, header.typeData, sizeof(typeData));
  } else {
    throw khException(kh::tr
                      ("Unsupported index format version (%1) for %2").
                      arg(header.indexFormatVersion).arg(filename));
  }
#if 0
  printf("indexFormatVersion = %d\n", header.indexFormatVersion);
  printf("totalFFSize = %llu\n", header.totalFFSize);
  printf("totalIndexSize = %u\n", header.totalIndexSize);
  printf("totalStoredTiles = %u\n", header.totalStoredTiles);
  printf("type = %u\n", header.type);
  printf("numLevels = %u\n", header.numLevels);
#endif

  // mmap the entire index
  filebuf = mmap(0, totalIndexSize, PROT_READ,
                 MAP_PRIVATE, fileHandle.fd(), 0);
  if (filebuf == MAP_FAILED) {
    throw khErrnoException(kh::tr("Unable to mmap %1").arg(filename));
  }
  munmapGuard.DelayedSet(filebuf, totalIndexSize);

  memset(levels, 0, sizeof(levels));
  levelvec.reserve(header.numLevels);
  IndexStorage::LevelRecord *recs =
    (IndexStorage::LevelRecord*)((char*)filebuf +
                                 sizeof(IndexStorage::Header));

  for (uint i = 0; i < header.numLevels; ++i) {
    IndexStorage::LevelRecord rec = recs[i];
    rec.LittleEndianToHost();
#if 0
    printf("    ----- level %d -----\n", rec.levelNum);
    printf("    tileRecordsOffset = %u\n", rec.tileRecordsOffset);
    printf("    totalStoredTiles = %u\n", rec.totalStoredTiles);
    printf("    startRow = %u\n", rec.startRow);
    printf("    startCol = %u\n", rec.startCol);
    printf("    numRows = %u\n", rec.numRows);
    printf("    numCols = %u\n", rec.numCols);
#endif

    // sanity check the index (in case it got corrupted)
    if (rec.levelNum > MaxFusionLevel) {
      throw khException(kh::tr
                        ("Corrupted index header (levelNum) %1: level %2").
                        arg(filename).arg(rec.levelNum));
    }
    if (rec.tileRecordsOffset +
        rec.totalStoredTiles * sizeof(IndexStorage::TileRecord) >
        totalIndexSize) {
      throw khException(kh::tr
                        ("Corrupted index header (totalIndexSize) %1: level %2").
                        arg(filename).arg(rec.levelNum));
    }
    // interpret the LevelRecord to build my LevelInfo object
    levelvec.push_back
      (LevelInfo(khLevelCoverage
                 (rec.levelNum,
                  khExtents<uint32>(RowColOrder,
                                    rec.startRow,
                                    rec.startRow + rec.numRows,
                                    rec.startCol,
                                    rec.startCol + rec.numCols)),
                 (IndexStorage::TileRecord*)((char*)filebuf +
                                             rec.tileRecordsOffset)));
    // safe to store the address to an element in the vector since
    // we pre-sized the vector
    levels[rec.levelNum] = &levelvec[i];
  }
}

ffio::IndexReader::~IndexReader(void)
{
  // munmapGuard destructor will call msync and munmap

  // fileHandle's destructor will take care of closing the file
}


bool
ffio::IndexReader::FindTile(const khTileAddr &addr,
                            uint64 &fileOffset, uint32 &dataLen)
{
  assert(addr.level <= MaxFusionLevel);
  return (levels[addr.level] &&
          levels[addr.level]->FindTile(addr.row, addr.col,
                                       fileOffset, dataLen));
}

void
ffio::IndexReader::PopulateCoverage(khInsetCoverage &cov) const
{
  if (levelvec.empty())
    return;

  uint beginLevel = levelvec.front().level;
  uint endLevel   = levelvec.back().level + 1;

  if (levelvec.size() != endLevel - beginLevel) {
    throw khException
      (kh::tr("Internal Error: misordered levels in ffio::IndexReader"));
  }

  std::vector<khExtents<uint32> > extentsList;
  extentsList.reserve(levelvec.size());
  for (uint i = 0; i < levelvec.size(); ++i) {
    extentsList.push_back(levelvec[i].extents);
  }

  cov = khInsetCoverage(beginLevel, endLevel, extentsList);
}

void
ffio::IndexReader::DumpRecords(const LevelInfo *level) const
{
  for (uint32 row = level->extents.beginRow();
       row < level->extents.endRow(); ++row) {
    for (uint32 col = level->extents.beginCol();
         col < level->extents.endCol(); ++col) {
      const IndexStorage::TileRecord *tile =
        level->TileRec(row, col);
      if (tile->dataLen) {
        printf("    %d,%d:  offset = %llu, len = %u\n",
               row, col,
               (long long unsigned int)::LittleEndianToHost(tile->dataOffset),
               ::LittleEndianToHost(tile->dataLen));
      } else {
        printf("    %d,%d:  Empty\n", row, col);
      }
    }
  }
}


// ****************************************************************************
// ***  ffio::IndexReader::LevelInfo
// ****************************************************************************
const ffio::IndexStorage::TileRecord *
ffio::IndexReader::LevelInfo::TileRec(uint32 row, uint32 col) const
{
  return tilerecs +
    ((row - extents.beginRow()) * extents.width()) +
    (col - extents.beginCol());
}

bool
ffio::IndexReader::LevelInfo::FindTile(uint32 row, uint32 col,
                                       uint64 &fileOffset, uint32 &dataLen) const
{
  if (extents.ContainsRowCol(row, col)) {
    const IndexStorage::TileRecord *tile = TileRec(row, col);
    // we can check for 0 w/o doing endian conversion, 0 is always 0
    if (tile->dataLen == 0) {
      return false;
    } else {
      // file is store as LittleEndian, just extract the two pieces I
      // need, converting endian on the fly
      fileOffset = ::LittleEndianToHost(tile->dataOffset);
      dataLen    = ::LittleEndianToHost(tile->dataLen);
      return true;
    }
  } else {
    return false;
  }
}


bool
ffio::IndexReader::LevelInfo::HasRowCol(uint32 row, uint32 col) const
{
  if (extents.ContainsRowCol(row, col)) {
    const IndexStorage::TileRecord *tile = TileRec(row, col);
    return (tile->dataLen != 0);
  }
  return false;
}
