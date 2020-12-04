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

//

#include "common/khPresenceMask.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/khInsetCoverage.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "common/khException.h"


// ****************************************************************************
// ***  Presence File format objects
// ****************************************************************************
// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 32 bytes long.
class PresenceHeader {
 public:
  static const char themagic[];

  char   magic[22];
  std::uint8_t  presenceFormatVersion;
  std::uint8_t  numLevels;
  std::uint32_t totalFileSize;
  std::uint32_t unused1;

  PresenceHeader(std::uint8_t numLevels, std::uint32_t totalSize);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }

  // sould only be used when reading from file
  PresenceHeader() { }
};
static_assert(sizeof(PresenceHeader) == 32, "Bad Presence Header Size");

const char
PresenceHeader::themagic[] = "Keyhole Presence Mask";
static_assert(sizeof(PresenceHeader::themagic)-1 ==
                   sizeof(PresenceHeader().magic),
                   "Invalid Magic Size");


PresenceHeader::PresenceHeader(std::uint8_t numLevels_, std::uint32_t totalSize)
    : presenceFormatVersion(1),
      numLevels(numLevels_),
      totalFileSize(totalSize),
      unused1(0) {
  memcpy(magic, themagic, sizeof(magic));
}

void
PresenceHeader::LittleEndianToHost(void) {
  // magic -- no need to swap
  presenceFormatVersion = ::LittleEndianToHost(presenceFormatVersion);
  numLevels             = ::LittleEndianToHost(numLevels);
  totalFileSize         = ::LittleEndianToHost(totalFileSize);
  unused1               = ::LittleEndianToHost(unused1);
}


// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 32 bytes long.
class PresenceLevelRecord {
 public:
  std::uint32_t presenceBufferOffset;
  std::uint32_t presenceBufferSize;
  std::uint32_t startRow;
  std::uint32_t startCol;
  std::uint32_t numRows;
  std::uint32_t numCols;
  std::uint8_t  levelNum;
  std::uint8_t  unused1;             // == 0
  std::uint16_t unused2;             // == 0
  std::uint32_t unused3;             // == 0

  PresenceLevelRecord(std::uint32_t presenceBufferOffset_,
                      const khLevelPresenceMask &level);
  void LittleEndianToHost(void);
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }
};
static_assert(sizeof(PresenceLevelRecord) == 32,
                   "Bad Presence Level Record Size");


PresenceLevelRecord::PresenceLevelRecord(std::uint32_t presenceBufferOffset_,
                                         const khLevelPresenceMask &level) :
    presenceBufferOffset(presenceBufferOffset_),
    presenceBufferSize(level.BufferSize()),
    startRow(level.extents.beginRow()),
    startCol(level.extents.beginCol()),
    numRows(level.extents.numRows()),
    numCols(level.extents.numCols()),
    levelNum(level.level),
    unused1(0),
    unused2(0),
    unused3(0) {
}

void
PresenceLevelRecord::LittleEndianToHost(void) {
  presenceBufferOffset = ::LittleEndianToHost(presenceBufferOffset);
  presenceBufferSize   = ::LittleEndianToHost(presenceBufferSize);
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
// ***  khPresenceMask
// ****************************************************************************
khPresenceMask::khPresenceMask(const khInsetCoverage &coverage,
                               bool set_present)
    : numLevels(coverage.numLevels()),
      beginLevel(coverage.beginLevel()),
      endLevel(coverage.endLevel()) {
  for (unsigned int i = beginLevel; i < endLevel; ++i) {
    levels[i] = TransferOwnership(
        new khLevelPresenceMask(i, coverage.levelExtents(i), set_present));
  }
}

khPresenceMask::khPresenceMask(const khPresenceMask &o)
    : numLevels(o.numLevels),
      beginLevel(o.beginLevel),
      endLevel(o.endLevel) {
  for (unsigned int i = beginLevel; i < endLevel; ++i) {
    levels[i] = TransferOwnership(new khLevelPresenceMask(*o.levels[i]));
  }
}

khPresenceMask& khPresenceMask::operator=(const khPresenceMask &o) {
  if (&o != this) {
    for (unsigned int i = beginLevel; i < endLevel; ++i) {
      levels[i].clear();
    }
    numLevels  = o.numLevels;
    beginLevel = o.beginLevel;
    endLevel   = o.endLevel;
    for (unsigned int i = beginLevel; i < endLevel; ++i) {
      levels[i] = TransferOwnership(new khLevelPresenceMask(*o.levels[i]));
    }
  }
  return *this;
}



khPresenceMask::khPresenceMask(const std::string &filename)
    : numLevels(0),
      beginLevel(MaxFusionLevel),  // opposites ends so we can
      endLevel(0) {                // load the real values
  std::uint64_t fileSize;
  time_t mtime;
  if (!khGetFileInfo(filename, fileSize, mtime)) {
    throw khErrnoException(
        kh::tr("Unable to get file size for %1").arg(filename.c_str()));
  }

  // open the file, read & verify the header
  khReadFileCloser fileHandle(::open(filename.c_str(), O_RDONLY));
  if (fileHandle.fd() < 0) {
    throw khErrnoException(kh::tr("Unable to open %1").arg(filename.c_str()));
  }
  PresenceHeader header;
  if (!khReadAll(fileHandle.fd(), &header, sizeof(header))) {
    throw khErrnoException(kh::tr("Unable to read header %1").
                           arg(filename.c_str()));
  }
  header.LittleEndianToHost();

  if (memcmp(header.magic, PresenceHeader::themagic,
             sizeof(header.magic)) != 0) {
    throw khException(kh::tr("Corrupted header (magic) %1").
                      arg(filename.c_str()));
  }

  if (header.presenceFormatVersion == 1) {
    // nothing to do here
  } else {
    throw khException(kh::tr
                      ("Unsupported format version (%1) for %2").
                      arg(header.presenceFormatVersion).arg(filename.c_str()));
  }

#if 0
  printf("presenceFormatVersion = %d\n", header.presenceFormatVersion);
  printf("totalFileSize = %u\n", header.totalFileSize);
  printf("numLevels = %u\n", header.numLevels);
#endif

  // some sanity checks
  if (header.totalFileSize != fileSize) {
    throw khException(kh::tr("Corrupted header (file size) %1").
                      arg(filename.c_str()));
  }
  if (header.numLevels > NumFusionLevels) {
    throw khException(kh::tr("Corrupted header (num levels) %1").
                      arg(filename.c_str()));
  }

  // set my member now that I know what it shold be
  numLevels = header.numLevels;

  // mmap the entire index
  unsigned char *filebuf = static_cast<unsigned char*>(mmap(0, header.totalFileSize, PROT_READ,
                                            MAP_PRIVATE, fileHandle.fd(), 0));
  if (filebuf == MAP_FAILED) {
    throw khErrnoException(kh::tr("Unable to mmap %1").arg(filename.c_str()));
  }
  khMunmapGuard munmapGuard(filebuf, header.totalFileSize);


  PresenceLevelRecord *recs =
      reinterpret_cast<PresenceLevelRecord*>(filebuf + sizeof(PresenceHeader));
  for (unsigned int i = 0; i < header.numLevels; ++i) {
    PresenceLevelRecord rec = recs[i];
    rec.LittleEndianToHost();

#if 0
    printf("    ----- level %d -----\n", rec.levelNum);
    printf("    presenceBufferOffset = %u\n", rec.presenceBufferOffset);
    printf("    presenceBufferSize   = %u\n", rec.presenceBufferSize);
    printf("    startRow = %u\n", rec.startRow);
    printf("    startCol = %u\n", rec.startCol);
    printf("    numRows = %u\n", rec.numRows);
    printf("    numCols = %u\n", rec.numCols);
#endif

    // sanity check the index (in case it got corrupted)
    if (rec.levelNum >= NumFusionLevels) {
      throw khException
        (kh::tr("Corrupted level header (levelNum) %1: level %2").
         arg(filename.c_str()).arg(rec.levelNum));
    }
    if (rec.presenceBufferSize !=
        khLevelPresenceMask::CalcBufferSize(rec.numRows,
                                            rec.numCols)) {
      throw khException
        (kh::tr("Corrupted level header (size) %1: level %2").
         arg(filename.c_str()).arg(rec.levelNum));
    }
    if (rec.presenceBufferOffset + rec.presenceBufferSize >
        header.totalFileSize) {
      throw khException
        (kh::tr("Corrupted level header (offset) %1: level %2").
         arg(filename.c_str()).arg(rec.levelNum));
    }


    levels[rec.levelNum] =
      TransferOwnership(
          new khLevelPresenceMask
          (rec.levelNum,
           khExtents<std::uint32_t>(RowColOrder,
                             rec.startRow,
                             rec.startRow + rec.numRows,
                             rec.startCol,
                             rec.startCol + rec.numCols),
           filebuf + rec.presenceBufferOffset));

    if (rec.levelNum >= endLevel)
      endLevel = rec.levelNum+1;
    if (rec.levelNum < beginLevel)
      beginLevel = rec.levelNum;
  }

  if (numLevels != endLevel - beginLevel) {
    throw khException
      (kh::tr("Corrupted header (min/max/num) %1").
       arg(filename.c_str()));
  }

  // munmapGuard's destructor will call munmap

  // fileHandle's destructor will take care of closing the file
}

void khPresenceMask::PopulateCoverage(khInsetCoverage &cov) const {
  if (numLevels == 0)
    return;

  std::vector<khExtents<std::uint32_t> > extentsList;
  extentsList.reserve(numLevels);
  for (unsigned int i = beginLevel; i < endLevel; ++i) {
    extentsList.push_back(levels[i]->extents);
  }

  cov = khInsetCoverage(beginLevel, endLevel, extentsList);
}

khExtents<std::uint32_t> khPresenceMask::levelTileExtents(unsigned int lev) const {
  assert(lev < NumFusionLevels);
  if (levels[lev]) {
    return levels[lev]->extents;
  } else {
    return khExtents<std::uint32_t>();
  }
}

void khPresenceMask::SetPresenceCascade(khTileAddr addr) {
  assert(addr.level < NumFusionLevels);

  while (levels[addr.level]) {
    if (levels[addr.level]->GetPresence(addr.row, addr.col)) {
      break;
    }
    levels[addr.level]->SetPresence(
        addr.row, addr.col, true, Int2Type<false>());
    if (addr.level <= beginLevel) {
      break;
    }
    addr.minifyBy(1);
  }
}

bool khPresenceMask::GetEstimatedPresence(const khTileAddr &addr) const {
  assert(addr.level < NumFusionLevels);
  if (levels[addr.level]) {
    return levels[addr.level]->GetPresence(addr.row, addr.col);
  } else if (addr.level >= endLevel) {
    // reach up to maxLevel (endLevel-1) and check the single tile there
    khTileAddr maxAddr(addr.MinifiedToLevel(endLevel-1));
    return levels[maxAddr.level]->GetPresence(maxAddr.row, maxAddr.col);
  } else {
    assert(addr.level < beginLevel);
    // reach down to beginLevel and traverse all tiles there
    // stop as soon as we find a hit
    khLevelCoverage minCoverage(addr.MagnifiedToLevel(beginLevel));
    khExtents<std::uint32_t> tocheck =
      khExtents<std::uint32_t>::Intersection(minCoverage.extents,
                                      levels[beginLevel]->extents);
    for (std::uint32_t row = tocheck.beginRow(); row < tocheck.endRow(); ++row) {
      for (std::uint32_t col = tocheck.beginCol(); col < tocheck.endCol(); ++col) {
        if (levels[beginLevel]->GetPresence(row, col)) {
          return true;
        }
      }
    }
  }
  return false;
}

// ****************************************************************************
// ***  khPresenceMaskWriter
// ****************************************************************************
 std::uint32_t khPresenceMaskWriter::DataOffset(void) {
  return (sizeof(PresenceHeader) +
          presence.numLevels * sizeof(PresenceLevelRecord));
}

 std::uint32_t khPresenceMaskWriter::TotalFileSize(void) {
  std::uint32_t totalFileSize = DataOffset();
  for (unsigned int i = 0; i < NumFusionLevels; ++i) {
    if (presence.levels[i]) {
      totalFileSize += presence.levels[i]->BufferSize();
    }
  }
  return totalFileSize;
}


khPresenceMaskWriter::khPresenceMaskWriter(const std::string &filename,
                                           const khInsetCoverage &coverage)
    : presence(coverage),
      munmapper() {
  // Go ahead and open/pre-size the file now. That way we can catch any
  // filesystem problems early
  const std::uint32_t totalFileSize = TotalFileSize();

  // Open file, mmap it, and zero it out
  (void) ::umask(000);
  proxyFile.Open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
  int fd = proxyFile.FileId();

  // presize & fill w/ 0's
  khFillFile(fd, 0, totalFileSize);
  unsigned char *filebuf = static_cast<unsigned char*>(
      mmap(0, totalFileSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
  if (filebuf == MAP_FAILED) {
    throw khErrnoException(kh::tr("Unable to mmap %1").arg(filename.c_str()));
  }
  munmapper.DelayedSet(filebuf, totalFileSize);


  // Leave file filled w/ 0's. That way if we abort before finishing,
  // the header won't be valid
}


khPresenceMaskWriter::~khPresenceMaskWriter(void) {
  std::uint32_t presenceBufferOffset = DataOffset();
  std::uint32_t totalFileSize        = TotalFileSize();

  unsigned char *filebuf = static_cast<unsigned char*>(munmapper.buffer());

  // write header
  PresenceHeader *header =
    new(filebuf) PresenceHeader(presence.numLevels, totalFileSize);
  header->HostToLittleEndian();

  PresenceLevelRecord *lrec =
      reinterpret_cast<PresenceLevelRecord*>(header + 1);
  for (unsigned int i = 0; i < NumFusionLevels; ++i) {
    if (presence.levels[i]) {
      // write the level header rec
      (void) new(lrec) PresenceLevelRecord(presenceBufferOffset,
                                           *presence.levels[i]);
      lrec->HostToLittleEndian();
      ++lrec;

      // write the level's presence buffer
      memcpy(filebuf + presenceBufferOffset,
             &presence.levels[i]->buf[0],
             presence.levels[i]->BufferSize());

      presenceBufferOffset += presence.levels[i]->BufferSize();
    }
  }

  // munmapper's destructor will call msync and munmap

  // geTmpLocalFile's destructor will take copy the file to its real path.
}
