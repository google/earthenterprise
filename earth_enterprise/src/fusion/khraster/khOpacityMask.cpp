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

#include "fusion/khraster/khOpacityMask.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/khAssert.h"
#include "common/khEndian.h"
#include "common/geFileUtils.h"
#include "common/khFileUtils.h"
#include "common/khException.h"
#include "fusion/autoingest/MiscConfig.h"


// ****************************************************************************
// ***  khOpacityMask::Level
// ****************************************************************************
khOpacityMask::Level::Level(uint lev, const khExtents<uint32> &extents)
    : khLevelCoverage(lev, extents),
      buf(TransferOwnership(new uchar[BufferSize()])) {
  memset(&buf[0], 0, BufferSize());
}

khOpacityMask::Level::Level(uint lev, const khExtents<uint32> &extents,
                            uchar *src)
    : khLevelCoverage(lev, extents),
      buf(TransferOwnership(new uchar[BufferSize()])) {
  memcpy(&buf[0], src, BufferSize());
}

khOpacityMask::OpacityType khOpacityMask::Level::GetOpacity(
    uint32 row, uint32 col) const {
  if (extents.ContainsRowCol(row, col)) {
    uint32 r = row - extents.beginRow();
    uint32 c = col - extents.beginCol();
    uint32 tilePos = r * extents.numCols() + c;
    uint32 byteNum = tilePos / 4;
    uint32 byteOffset = (tilePos % 4) * 2;

    assert(byteNum < BufferSize());
    return (OpacityType)((buf[byteNum] >> byteOffset) & 0x3);
  } else {
    return Unknown;
  }
}

void khOpacityMask::Level::SetOpacity(
    uint32 row, uint32 col, OpacityType opacity) {
  uint32 r = row - extents.beginRow();
  uint32 c = col - extents.beginCol();
  uint32 tilePos = r * extents.numCols() + c;
  uint32 byteNum = tilePos / 4;
  uint32 byteOffset = (tilePos % 4) * 2;

  assert(byteNum < BufferSize());
  buf[byteNum] =
    (buf[byteNum] & ~(0x3 << byteOffset)) |  // strip out old value
    (((uchar)opacity & 0x3) << byteOffset);  // 'or' in new value
}


// ****************************************************************************
// ***  Opacity File format objects
// ****************************************************************************
// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 32 bytes long.
class OpacityHeader {
 public:
  static const char themagic[];

  char   magic[21];
  uint8  opacityFormatVersion;
  uint8  numLevels;
  uint8  unused1;
  uint32 totalFileSize;
  uint32 unused2;

  OpacityHeader(uint8 numLevels, uint32 totalSize);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }

  // sould only be used when reading from file
  OpacityHeader() { }
};
COMPILE_TIME_CHECK(sizeof(OpacityHeader) == 32, BadOpacityHeaderSize);

const char
OpacityHeader::themagic[] = "Keyhole Opacity Mask";
COMPILE_TIME_CHECK(sizeof(OpacityHeader::themagic)-1 ==
                   sizeof(OpacityHeader().magic),
                   InvalidMagicSize);


OpacityHeader::OpacityHeader(uint8 numLevels_, uint32 totalSize)
    : opacityFormatVersion(1),
      numLevels(numLevels_),
      unused1(0),
      totalFileSize(totalSize),
      unused2(0) {
  memcpy(magic, themagic, sizeof(magic));
}

void
OpacityHeader::LittleEndianToHost(void) {
  // magic -- no need to swap
  opacityFormatVersion = ::LittleEndianToHost(opacityFormatVersion);
  numLevels            = ::LittleEndianToHost(numLevels);
  unused1              = ::LittleEndianToHost(unused1);
  totalFileSize        = ::LittleEndianToHost(totalFileSize);
  unused2              = ::LittleEndianToHost(unused2);
}


// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 32 bytes long.
class OpacityLevelRecord {
 public:
  uint32 opacityBufferOffset;
  uint32 opacityBufferSize;
  uint32 startRow;
  uint32 startCol;
  uint32 numRows;
  uint32 numCols;
  uint8  levelNum;
  uint8  unused1;             // == 0
  uint16 unused2;             // == 0
  uint32 unused3;             // == 0

  OpacityLevelRecord(uint32 opacityBufferOffset_,
                     const khOpacityMask::Level &level);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }
};
COMPILE_TIME_CHECK(sizeof(OpacityLevelRecord) == 32,
                   BadOpacityLevelRecordSize);

OpacityLevelRecord::OpacityLevelRecord(uint32 opacityBufferOffset_,
                                       const khOpacityMask::Level &level)
    : opacityBufferOffset(opacityBufferOffset_),
      opacityBufferSize(level.BufferSize()),
      startRow(level.extents.beginRow()),
      startCol(level.extents.beginCol()),
      numRows(level.extents.numRows()),
      numCols(level.extents.numCols()),
      levelNum(level.level),
      unused1(0),
      unused2(0),
      unused3(0) {
}

void OpacityLevelRecord::LittleEndianToHost(void) {
  opacityBufferOffset = ::LittleEndianToHost(opacityBufferOffset);
  opacityBufferSize   = ::LittleEndianToHost(opacityBufferSize);
  startRow            = ::LittleEndianToHost(startRow);
  startCol            = ::LittleEndianToHost(startCol);
  numRows             = ::LittleEndianToHost(numRows);
  numCols             = ::LittleEndianToHost(numCols);
  levelNum            = ::LittleEndianToHost(levelNum);

  // Go ahead and swap the unused ones as well, the cost is minimal and it
  // protects against future mistakes when these do get used.
  unused1             = ::LittleEndianToHost(unused1);
  unused2             = ::LittleEndianToHost(unused2);
  unused3             = ::LittleEndianToHost(unused3);
}



// ****************************************************************************
// ***  khOpacityMask
// ****************************************************************************
khOpacityMask::khOpacityMask(const khInsetCoverage &coverage)
    : numLevels(coverage.numLevels()),
      beginLevel(coverage.beginLevel()),
      endLevel(coverage.endLevel()) {
  for (uint i = beginLevel; i < endLevel; ++i) {
    levels[i] = TransferOwnership(new Level(i, coverage.levelExtents(i)));
  }
}

khOpacityMask::khOpacityMask(const std::string &filename)
    : numLevels(0),
      beginLevel(MaxFusionLevel),  // opposite ends so we can
      endLevel(0) {                // load the real values.
  // Since these files can be very small, sometimes kernels and
  // nfs servers in between the writer and the reader can delay the writing.
  // Let's wait until the file is at least a little bit cool. :-)
  WaitIfFileIsTooNew(filename, MiscConfig::Instance().NFSVisibilityDelay);

  uint64 fileSize;
  time_t mtime;
  if (!khGetFileInfo(filename, fileSize, mtime)) {
    throw khErrnoException(
        kh::tr("Unable to get file size for %1").arg(filename));
  }

  // open the file, read & verify the header
  khReadFileCloser fileHandle(::open(filename.c_str(), O_RDONLY));
  if (fileHandle.fd() < 0) {
    throw khErrnoException(kh::tr("Unable to open %1").arg(filename));
  }
  OpacityHeader header;
  if (!khReadAll(fileHandle.fd(), &header, sizeof(header))) {
    throw khErrnoException(kh::tr("Unable to read header %1").
                           arg(filename));
  }
  header.LittleEndianToHost();

  if (memcmp(header.magic, OpacityHeader::themagic,
             sizeof(header.magic)) != 0) {
    throw khErrnoException(kh::tr("Corrupted header (magic) %1").
                           arg(filename));
  }

  if (header.opacityFormatVersion == 1) {
    // nothing to do here
  } else {
    throw khException(kh::tr
                      ("Unsupported format version (%1) for %2").
                      arg(header.opacityFormatVersion).arg(filename));
  }

#if 0
  printf("opacityFormatVersion = %d\n", header.opacityFormatVersion);
  printf("totalFileSize = %u\n", header.totalFileSize);
  printf("numLevels = %u\n", header.numLevels);
#endif

  // some sanity checks
  if (header.totalFileSize != fileSize) {
    throw khErrnoException(kh::tr("Corrupted header (file size) %1").
                           arg(filename));
  }
  if (header.numLevels > NumFusionLevels) {
    throw khErrnoException(kh::tr("Corrupted header (num levels) %1").
                           arg(filename));
  }

  // set my member now that I know what it shold be
  numLevels = header.numLevels;

  // mmap the entire index
  uchar *filebuf =
      reinterpret_cast<uchar*>(mmap(0, header.totalFileSize, PROT_READ,
                                    MAP_PRIVATE, fileHandle.fd(), 0));
  if (filebuf == MAP_FAILED) {
    throw khErrnoException(kh::tr("Unable to mmap %1").arg(filename));
  }
  khMunmapGuard munmapGuard(filebuf, header.totalFileSize);


  OpacityLevelRecord *recs =
      reinterpret_cast<OpacityLevelRecord*>(filebuf + sizeof(OpacityHeader));
  for (uint i = 0; i < header.numLevels; ++i) {
    OpacityLevelRecord rec = recs[i];
    rec.LittleEndianToHost();

#if 0
    printf("    ----- level %d -----\n", rec.levelNum);
    printf("    opacityBufferOffset = %u\n", rec.opacityBufferOffset);
    printf("    opacityBufferSize   = %u\n", rec.opacityBufferSize);
    printf("    startRow = %u\n", rec.startRow);
    printf("    startCol = %u\n", rec.startCol);
    printf("    numRows = %u\n", rec.numRows);
    printf("    numCols = %u\n", rec.numCols);
#endif

    // sanity check the index (in case it got corrupted)
    if (rec.levelNum >= NumFusionLevels) {
      throw khException
        (kh::tr("Corrupted level header (levelNum) %1: level %2").
         arg(filename).arg(rec.levelNum));
    }
    if (rec.opacityBufferSize !=
        khOpacityMask::Level::BufferSize(rec.numRows, rec.numCols)) {
      throw khException
        (kh::tr("Corrupted level header (size) %1: level %2").
         arg(filename).arg(rec.levelNum));
    }
    if (rec.opacityBufferOffset + rec.opacityBufferSize >
        header.totalFileSize) {
      throw khException
        (kh::tr("Corrupted level header (offset) %1: level %2").
         arg(filename).arg(rec.levelNum));
    }


    levels[rec.levelNum] = TransferOwnership(
        new Level(rec.levelNum,
                  khExtents<uint32>(RowColOrder,
                                    rec.startRow,
                                    rec.startRow + rec.numRows,
                                    rec.startCol,
                                    rec.startCol + rec.numCols),
                  filebuf + rec.opacityBufferOffset));

    if (rec.levelNum >= endLevel)
      endLevel = rec.levelNum+1;
    if (rec.levelNum < beginLevel)
      beginLevel = rec.levelNum;
  }

  if (numLevels != endLevel - beginLevel) {
    throw khException
      (kh::tr("Corrupted header (min/max/num) %1").
       arg(filename));
  }

  // munmapGuard's destructor will call munmap

  // fileHandle's destructor will take care of closing the file
}

void khOpacityMask::Save(const std::string &filename) {
  // calculate totalFileSize
  uint32 opacityBufferOffset = sizeof(OpacityHeader) +
                               numLevels * sizeof(OpacityLevelRecord);
  uint32 totalFileSize = opacityBufferOffset;
  for (uint i = 0; i < NumFusionLevels; ++i) {
    if (levels[i]) {
      totalFileSize += levels[i]->BufferSize();
    }
  }

  // Use local tmp file if indicated (i.e. for NFS async mode)
  geLocalBufferWriteFile proxyFile;
  // open file, mmap it, and zero it out
  (void) ::umask(000);
  proxyFile.Open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
  int fd = proxyFile.FileId();

  // presize & fill w/ 0's
  khFillFile(fd, 0, totalFileSize);
  uchar *filebuf =
      reinterpret_cast<uchar*>(mmap(0, totalFileSize, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, fd, 0));
  if (filebuf == MAP_FAILED) {
    throw khErrnoException(kh::tr("Unable to mmap %1").arg(filename));
  }
  khMunmapGuard munmapGuard(filebuf, totalFileSize);


  // write header
  OpacityHeader *header =
    new(filebuf) OpacityHeader(numLevels, totalFileSize);
  header->HostToLittleEndian();

  OpacityLevelRecord *lrec = reinterpret_cast<OpacityLevelRecord*>(header+1);
  for (uint i = 0; i < NumFusionLevels; ++i) {
    if (levels[i]) {
      // write the level header rec
      (void) new(lrec) OpacityLevelRecord(opacityBufferOffset,
                                          *levels[i]);
      lrec->HostToLittleEndian();
      ++lrec;

      // write the level's opacity buffer
      memcpy(filebuf + opacityBufferOffset,
             &levels[i]->buf[0], levels[i]->BufferSize());

      opacityBufferOffset += levels[i]->BufferSize();
    }
  }

  // geTmpLocalFile's destructor will save proxy file to actual file

  // munmapGuard's destructor will call msync and munmap
}


khOpacityMask::OpacityType ComputeOpacity(const AlphaProductTile &tile) {
  uchar sample = tile.bufs[0][0];
  if ((sample != 0) && (sample != 255)) {
    return khOpacityMask::Amalgam;
  }
  for (uint pos = 0; pos < AlphaProductTile::BandPixelCount; ++pos) {
    if (tile.bufs[0][pos] != sample)
      return khOpacityMask::Amalgam;
  }
  return (sample == 0) ? khOpacityMask::Transparent : khOpacityMask::Opaque;
}

khOpacityMask::OpacityType ComputeOpacity(const AlphaProductTile &tile,
                                          const khExtents<uint32>& extents) {
  uchar sample = tile.bufs[0][(extents.beginRow() * AlphaProductTile::TileWidth)
                               + extents.beginCol()];
  if ((sample != 0) && (sample != 255)) {
    return khOpacityMask::Amalgam;
  }
  for (uint32 row = extents.beginRow(); row < extents.endRow(); ++row) {
    for (uint32 col = extents.beginCol(); col < extents.endCol(); ++col) {
      if (tile.bufs[0][(row * AlphaProductTile::TileWidth) + col] != sample)
        return khOpacityMask::Amalgam;
    }
  }
  return (sample == 0) ? khOpacityMask::Transparent : khOpacityMask::Opaque;
}


khOpacityMask::OpacityType khOpacityMask::GetOpacity(
    const khTileAddr &addr) const {
  assert(addr.level < NumFusionLevels);
  if (!numLevels) {
    // simplify logic below by catching this degenerate case
    return Unknown;
  }
  if (levels[addr.level]) {
    return levels[addr.level]->GetOpacity(addr.row, addr.col);
  } else if (addr.level < beginLevel) {
    khLevelCoverage minCoverage(addr.MagnifiedToLevel(beginLevel));
    if (levels[beginLevel]->extents.contains(minCoverage.extents)) {
      // get the first one as a baseline
      OpacityType minOpacity =
        levels[beginLevel]->GetOpacity(minCoverage.extents.beginRow(),
                                       minCoverage.extents.beginCol());
      // save ourselves the trouble, just return now ...
      if (minOpacity == Amalgam)
        return Amalgam;
      for (uint minRow = minCoverage.extents.beginRow();
           minRow < minCoverage.extents.endRow(); ++minRow) {
        for (uint minCol = minCoverage.extents.beginCol();
             minCol < minCoverage.extents.endCol(); ++minCol) {
          if (levels[beginLevel]->GetOpacity(minRow, minCol)
              != minOpacity)
            return Amalgam;
        }
      }
      return minOpacity;
    } else {
      return Unknown;
    }
  } else {
    // see if our maxlevel has enough information to allow us to answer the
    // query
    assert(addr.level >= endLevel);
    khTileAddr maxAddr(addr.MinifiedToLevel(endLevel-1));
    OpacityType magOpacity =
      levels[maxAddr.level]->GetOpacity(maxAddr.row, maxAddr.col);
    if ((magOpacity == Transparent) || (magOpacity == Opaque))
      return magOpacity;
    else
      return Unknown;
  }
}
