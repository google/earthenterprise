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


#include "pyrio.h"
#include <khEndian.h>
#include <khFileUtils.h>
#include <khException.h>
#include <notify.h>
#include <cerrno>
#include <khstl.h>
#include <sstream>
#include <iomanip>
#include "khRasterTile.h"
#include "CastTile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// ****************************************************************************
// ***  IndexStorage
// ****************************************************************************
namespace pyrio {
namespace IndexStorage {
// Low level classes written to disk at the begining of pyramid files

// At the time this code was written, 100% of our processing machines
// were LittleEndian and were expected to remain that way for the
// foreseable future. Accordingly, this format is LittleEndian.  If
// you're using the public APIs, the endianess of this header should be
// completely hidden.


// This class is based on the old pyramidio index format.
// Unfortunately the old format didn't take alignment into account,
// so we can't read/write this header in one go, but that's OK. :-)
struct Header {
 private:
  static const uint numHeaderElem = 16;
  static const uint32 EndianCheck = 0x11223344;
 public:
  static const char themagic[];
  char   magic[20];
  uint32 iparams[numHeaderElem];
  double dparams[numHeaderElem];
 private:
  uint formatMajorVersion(void) const {
    return magic[17] - '0';
  }
  uint formatMinorVersion(void) const {
    return magic[19] - '0';
  }
  uint formatVersion(void) const {
    return formatMajorVersion() * 10 + formatMinorVersion();
  }


  // valid for formatMajorVersion >= 1
  uint32& endianCheck(void)   { return iparams[0]; }
  uint32& numCols(void)       { return iparams[1]; }
  uint32& numRows(void)       { return iparams[2]; }
  uint32& level(void)         { return iparams[3]; }
  uint32& tilesize(void)      { return iparams[4]; }
  uint32& compression(void)   { return iparams[5]; } // CompressMode enum
  uint32& numComponents(void) { return iparams[6]; }
  uint32& componentSize(void) { return iparams[7]; }

  double& dataNorth(void)     { return dparams[0]; }
  double& dataSouth(void)     { return dparams[1]; }
  double& dataEast(void)      { return dparams[2]; }
  double& dataWest(void)      { return dparams[3]; }
  double& tileNorth(void)     { return dparams[4]; }
  double& tileSouth(void)     { return dparams[5]; }
  double& tileEast(void)      { return dparams[6]; }
  double& tileWest(void)      { return dparams[7]; }

  // valid for formatMajorVersion >= 2
  uint32& rasterType(void)    { return iparams[8]; } // khRasterProduct::RasterType
  uint32& componentType(void) { return iparams[9]; } // khTypes::StorageEnum


  inline void LittleEndianToHost(void) {
    // nothing to do for magic
    for (uint i = 0; i < numHeaderElem; ++i) {
      iparams[i] = ::LittleEndianToHost(iparams[i]);
    }
    for (uint i = 0; i < numHeaderElem; ++i) {
      dparams[i] = ::LittleEndianToHost(dparams[i]);
    }
  }
  // just another name for code clarity
  inline void HostToLittleEndian(void) {
    LittleEndianToHost();
  }

 public:
  static bool Write(int fd, const pyrio::Header &phdr);
  static bool Read(pyrio::Header &phdr, int fd);
};
static const off64_t FileIndexOffset = sizeof(Header().magic)
    + sizeof(Header().iparams) + sizeof(Header().dparams);
} // namespace IndexStorage

// [1] terrains cannot be mercator.
// [2] for mercator projection tile extent calculated based on degree should not
//     match with stored meter tileExtents.
bool Header::IsMercator() const {
  return (rasterType != RasterProductStorage::Heightmap &&
      tileExtents.width() != (RasterProductTilespaceBase.NormTileSize(level) *
                              levelSize.width * 360.0));
}

} // namespace pyrio


const char
pyrio::IndexStorage::Header::themagic[] = "Keyhole Pyramid  1.1";
// sizeof(thmagic) will include space for the null terminator
COMPILE_TIME_CHECK(sizeof(pyrio::IndexStorage::Header::themagic)-1 ==
                   sizeof(pyrio::IndexStorage::Header().magic),
                   InvalidMagicSize)


    bool
pyrio::IndexStorage::Header::Read(pyrio::Header &phdr, int fd)
{
  Header hdr;
  if (!khReadAll(fd, hdr.magic, sizeof(hdr.magic))) {
    notify(NFY_WARN, "Unable to read magic from pyramid file: %s",
           khstrerror(errno).c_str());
    return false;
  }
  // check magic ignoring version
  if (memcmp(hdr.magic, themagic, sizeof(hdr.magic)-3) != 0) {
    notify(NFY_WARN, "Invalid magic in pyramid file");
    return false;
  }
  if (!khReadAll(fd, hdr.iparams, sizeof(hdr.iparams)) ||
      !khReadAll(fd, hdr.dparams, sizeof(hdr.dparams))) {
    notify(NFY_WARN, "Unable to read header from pyramid file: %s",
           khstrerror(errno).c_str());
    return false;
  }
  // convert to host byte order
  hdr.LittleEndianToHost();

  if (hdr.endianCheck() != Header::EndianCheck) {
    notify(NFY_WARN, "Corrupted pyramid header (bad endian check)");
    return false;
  }
  phdr.formatVersion = hdr.formatVersion();
  switch (hdr.formatVersion()) {
    case 11:
      phdr.rasterType = (RasterProductStorage::RasterType)hdr.rasterType();
      phdr.componentType = (khTypes::StorageEnum)hdr.componentType();
      // fall through to 10
    case 10:
      phdr.numComponents = hdr.numComponents();
      phdr.level = hdr.level();
      phdr.levelSize = khSize<uint32>(hdr.numCols(), hdr.numRows());
      phdr.dataExtents = khExtents<double>(NSEWOrder,
                                              hdr.dataNorth(),
                                              hdr.dataSouth(),
                                              hdr.dataEast(),
                                              hdr.dataWest());
      phdr.tileExtents = khExtents<double>(NSEWOrder,
                                              hdr.tileNorth(),
                                              hdr.tileSouth(),
                                              hdr.tileEast(),
                                              hdr.tileWest());
      phdr.compressMode = (CompressMode)hdr.compression();

      if (hdr.tilesize() != RasterProductTileResolution) {
        notify(NFY_WARN,
               "Corrupted pyramid header (bad tile size: %d)",
               hdr.tilesize());
        return false;
      }

      if (hdr.formatVersion() < 11) {

        // Earlier versions of pyramidio only supported since byte
        // components. It also left the unused fields as garbage. :-(
        // When support for int16 conponents was put in, they staeted
        // filling in the componentsize. So in this old header, if the
        // component size isn't 2, make it one. This is the same backwards
        // compatibility logic that was in older version of pyramidio
        if (hdr.componentSize() != 2) {
          hdr.componentSize() = 1;
        }


        // deduce rasterType and componentType from numComponents() and
        // componentSize()
        switch (hdr.numComponents()) {
          case 3:
            switch (hdr.componentSize()) {
              case 1:
                phdr.rasterType = RasterProductStorage::Imagery;
                phdr.componentType = khTypes::UInt8;
                break;
              default:
                notify(NFY_WARN,
                       "Corrupted pyramid header (bad imagery component size: %d)",
                       hdr.componentSize());
                return false;
            }
            break;
          case 1:
            switch (hdr.componentSize()) {
              case 1:
                phdr.rasterType = RasterProductStorage::AlphaMask;
                phdr.componentType = khTypes::UInt8;
                break;
              case 2:
                phdr.rasterType = RasterProductStorage::Heightmap;
                phdr.componentType = khTypes::Int16;
                break;
              default:
                notify(NFY_WARN,
                       "Corrupted pyramid header (bad component size: %d)",
                       hdr.componentSize());
                return false;
            }
            break;
          default:
            notify(NFY_WARN,
                   "Unsupported pyramid header (bad num components: %d)",
                   hdr.numComponents());
            return false;

        }
      }
      break;
    default:
      notify(NFY_WARN, "Unsupported pyramid header (bad version: %d.%d)",
             hdr.formatMajorVersion(), hdr.formatMinorVersion());
      return false;
  }
  if (khTypes::StorageSize(phdr.componentType) != hdr.componentSize()) {
    notify(NFY_WARN,
           "Corrupted pyramid header (mismatched type and size: %s,%d)",
           khTypes::StorageName(phdr.componentType),
           hdr.componentSize());
    return false;
  }





  return true;
}

bool
pyrio::IndexStorage::Header::Write(int fd, const pyrio::Header &phdr)
{
  Header hdr;
  memset(&hdr, 0, sizeof(hdr));
  memcpy(hdr.magic, themagic, sizeof (hdr.magic));
  hdr.endianCheck()   = EndianCheck;
  hdr.numCols()       = phdr.levelSize.width;
  hdr.numRows()       = phdr.levelSize.height;
  hdr.level()         = phdr.level;
  hdr.tilesize()      = RasterProductTileResolution;
  hdr.compression()   = (uint32)phdr.compressMode;
  hdr.numComponents() = phdr.numComponents;
  hdr.componentSize() = khTypes::StorageSize(phdr.componentType);
  hdr.dataNorth()     = phdr.dataExtents.north();
  hdr.dataSouth()     = phdr.dataExtents.south();
  hdr.dataEast()      = phdr.dataExtents.east();
  hdr.dataWest()      = phdr.dataExtents.west();
  hdr.tileNorth()     = phdr.tileExtents.north();
  hdr.tileSouth()     = phdr.tileExtents.south();
  hdr.tileEast()      = phdr.tileExtents.east();
  hdr.tileWest()      = phdr.tileExtents.west();
  hdr.rasterType()    = (uint32)phdr.rasterType;
  hdr.componentType() = (uint32)phdr.componentType;

  // convert from host byte order
  hdr.HostToLittleEndian();


  if (!khWriteAll(fd, hdr.magic, sizeof(hdr.magic))) {
    notify(NFY_WARN, "Unable to write magic to pyramid file: %s",
           khstrerror(errno).c_str());
    return false;
  }
  if (!khWriteAll(fd, hdr.iparams, sizeof(hdr.iparams)) ||
      !khWriteAll(fd, hdr.dparams, sizeof(hdr.dparams))) {
    notify(NFY_WARN, "Unable to write header to pyramid file: %s",
           khstrerror(errno).c_str());
    return false;
  }

  return true;
}


// ****************************************************************************
// ***  pyrio::Header
// ****************************************************************************
off64_t
pyrio::Header::FileDataOffset(void) const
{
  return pyrio::IndexStorage::FileIndexOffset +
    (off64_t)(sizeof(off64_t) * (1 + numComponents) *
              levelSize.width * levelSize.height);
}


// ****************************************************************************
// ***  pyrio::Reader
// ****************************************************************************
pyrio::Reader::Reader(const std::string &filename) :
    compressor(),
    pyrfile(),
    readBufSize(0),
    readBuf(),
    convertBuf(),
    flipBuf()
{
  pyrfile = ::open(filename.c_str(), O_RDONLY|O_LARGEFILE);
  if (pyrfile.fd() < 0) {
    throw khErrnoException(kh::tr("Unable to open %1").arg(filename));
  }
  if (!pyrio::IndexStorage::Header::Read(header_, pyrfile.fd())) {
    throw khException(kh::tr("Error reading %1").arg(filename));
  }
  switch (header_.compressMode) {
    case CompressNone:
      // nothing to do
      break;
    case CompressJPEG:
      if (header_.componentType != khTypes::UInt8) {
        throw khException(kh::tr("Error reading %1: JPEG compression with non uint8 pixel type").arg(filename));
      }
      compressor = TransferOwnership(
          new JPEGCompressor(RasterProductTileResolution,
                             RasterProductTileResolution,
                             1, /* pyrio always compresses one band at a time */
                             0 /* quaility - unused for decomp */));
      if (!compressor->IsValid()) {
        throw khException(kh::tr("compressor initialization failed!"));
      }
      break;
    case CompressLZ:
      compressor = TransferOwnership(
          NewLZCompressor(0 /* level - unused for decompression */,
                          header_.bandBufSize()));
      break;
    case CompressDXT1:
      throw khException(kh::tr("Error reading %1: DXT1 decompression not supported").arg(filename));
      break;
    case CompressPNG:
      throw khException(kh::tr("Error reading %1: PNG decompression not supported").arg(filename));
      break;
  }

  // Make an initial guess for the read buffer size
  // Adding 10% should be enough for the edge cases where LZ and/or JPEG
  // bloat instead of compress.  If we guess too small, we'll just make it
  // bigger later.
  if (compressor) {
    readBufSize = (uint32)(RasterProductTileResolution *
                           RasterProductTileResolution *
                           1 * /* pyrio always compresses on band at a time */
                           khTypes::StorageSize(header_.componentType) *
                           1.1);
    readBuf = TransferOwnership(new uchar[readBufSize]);
  }
}


bool
pyrio::Reader::ReadBandBufs(uint32 row, uint32 col,
                            uchar* destBufs[], uint bandsRequested[],
                            uint numBandsRequested,
                            khTypes::StorageEnum destType,
                            bool flipTopToBottom) const
{
  const uint NumIndexElem = 1 + header_.numComponents;
  const uint32 PixelCount =
    RasterProductTileResolution * RasterProductTileResolution;
  const uint32 UncompressedBandSize =
    PixelCount * khTypes::StorageSize(header_.componentType);

  if ((row >= header_.levelSize.height) ||
      (col >= header_.levelSize.width)) {
    // To deal with old pyrs missing tiles, just fill with black. API
    // above this one will catch tile requests that are outside what
    // "should" be here. If we get to here it means this is an old pyr
    // that's missing tiles due to rounding errors
    for (uint b = 0; b < numBandsRequested; ++b) {
      memset(destBufs[b], 0,
             PixelCount * khTypes::StorageSize(destType));
    }
    return true;
  }

  off64_t indexinfo[NumIndexElem];

  // Load the tile offset and band sizes
  if (lseek64(pyrfile.fd(),
              pyrio::IndexStorage::FileIndexOffset +
              (off64_t)(sizeof(indexinfo) *
                        (row * header_.levelSize.width + col)),
              SEEK_SET) < 0) {
    notify(NFY_WARN, "Unable to seek to pyramid index: %s",
           khstrerror(errno).c_str());
    return false;
  }
  if (!khReadAll(pyrfile.fd(), indexinfo, sizeof(indexinfo))) {
    notify(NFY_WARN, "Unable to read pyramid index: %s",
           khstrerror(errno).c_str());
    return false;
  }
  for (uint i = 0; i < NumIndexElem; ++i) {
    indexinfo[i] = ::LittleEndianToHost(indexinfo[i]);
  }

  notify(NFY_VERBOSE,
         "    ReadBandBufs (lrc) %u,%u,%u: offset = %llu",
         header_.level, row, col,
         static_cast<long long unsigned>(indexinfo[0]));

  if ((header_.componentType != destType) && !convertBuf) {
    if (!convertBuf) {
      convertBuf = TransferOwnership(new uchar[UncompressedBandSize]);
    }
  }
  if (flipTopToBottom) {
    if (!flipBuf) {
      flipBuf = TransferOwnership(
          new uchar[PixelCount * khTypes::StorageSize(destType)]);
    }
  }

  // For mono-chromatic images read just one band and replicate if necessary.
  uint one_band = 0;
  uint* const bands = header_.numComponents == 1 ? &one_band : bandsRequested;
  const uint numBands = header_.numComponents == 1 ? 1 : numBandsRequested;

  for (uint i = 0; i < numBands; ++i) {
    // figure out which band to read next
    uint b = bands[i];
    if (b >= header_.numComponents) {
      notify(NFY_WARN, "Internal Error: Invalid band number: %u >= %u",
             b, header_.numComponents);
      return false;
    }
    uchar *bandBuf = destBufs[i];
    const uint32 toread = indexinfo[b+1];

    // calculate the offset of the band data (tile offset + sizes of
    // previous bands)
    off64_t bandOffset = indexinfo[0];
    for (uint j = 0; j < b; ++j)
      bandOffset += indexinfo[j+1];

    // setup tmpbuf so we can read into it
    uchar *tmpbuf;
    if (compressor) {
      // make sure my read buf is big enough
      if (readBufSize < toread) {
        readBufSize = toread;
        readBuf = TransferOwnership(new uchar[readBufSize]);
      }
      tmpbuf = &*readBuf;
    } else {
      if (toread != UncompressedBandSize) {
        notify(NFY_WARN,
               "Invalid pyramid data size: %u != %u",
               toread, UncompressedBandSize);
      }
      if (header_.componentType != destType) {
        tmpbuf = &*convertBuf;
      } else if (flipTopToBottom) {
        tmpbuf = &*flipBuf;
      } else {
        tmpbuf = bandBuf;
      }
    }

    // seek to the beginning of the band data and read it
    if (lseek64(pyrfile.fd(), bandOffset, SEEK_SET) < 0) {
      notify(NFY_WARN,
             "Unable to seek to pyramid tile band (offset = %llu): %s",
             static_cast<long long unsigned>(bandOffset),
             khstrerror(errno).c_str());
      return false;
    }
    if (!khReadAll(pyrfile.fd(), tmpbuf, toread)) {
      notify(NFY_WARN, "Unable to read %u bytes of pyramid data: %s",
             toread, khstrerror(errno).c_str());
      return false;
    }


    // decompress if necessary
    if (compressor) {
      char *uncompBuf;
      if (header_.componentType != destType) {
        uncompBuf = (char*)&*convertBuf;
      } else if (flipTopToBottom) {
        uncompBuf = (char*)&*flipBuf;
      } else {
        uncompBuf = (char*)bandBuf;
      }
      uint32 size = compressor->decompress((char*)tmpbuf, toread,
                                           uncompBuf);
      if (size != UncompressedBandSize) {
        if (size == 0) {
          notify(NFY_WARN, "Unable to decompress pyramid data");
          return false;
        } else {
          notify(NFY_WARN,
                 "Invalid decompressed pyramid data size: %u != %u",
                 size, UncompressedBandSize);
          return false;
        }
      }
      tmpbuf = (uchar*)uncompBuf;
    }
    if (header_.componentType != destType) {
      uchar *castbuf;
      if (flipTopToBottom) {
        castbuf = &*flipBuf;
      } else {
        castbuf = bandBuf;
      }
      CastBuf(castbuf, destType,
              tmpbuf, header_.componentType,
              RasterProductTileResolution * RasterProductTileResolution);
      tmpbuf = castbuf;
    }
    if (flipTopToBottom) {
      uint32 lineSize = RasterProductTileResolution *
                        khTypes::StorageSize(destType);
      for (uint i = 0; i < RasterProductTileResolution; ++i) {
        memcpy(bandBuf + lineSize * i,
               tmpbuf + (lineSize *
                         (RasterProductTileResolution - i - 1)),
               lineSize);
      }
    }
  }
  // Replicate if necessary
  if (header_.numComponents == 1) {
    const size_t num_bytes = PixelCount * khTypes::StorageSize(destType);
    for (uint i = 1; i < numBandsRequested; ++i) {
      memcpy(destBufs[i], destBufs[0], num_bytes);
    }
  }
  return true;
}



// ****************************************************************************
// ***  pyrio::Writer
// ****************************************************************************
pyrio::Writer::Writer(const std::string &filename,
                      uint numComponents,
                      uint level,
                      const khSize<uint32> &levelSize,
                      const khExtents<double> &dataExtents,
                      const khExtents<double> &tileExtents,
                      CompressMode        compressMode,
                      RasterProductStorage::RasterType rasterType,
                      khTypes::StorageEnum componentType,
                      int compressionQuality)
    : compressor(),
      pyrfile(),
      runningOffset(0)
{
  if (compressMode == CompressJPEG &&
      componentType != khTypes::UInt8) {
    throw khException(kh::tr("Error writing %1: JPEG compression with non uint8 pixel type").arg(filename));
  }

  header.numComponents  = numComponents;
  header.level          = level;
  header.levelSize      = levelSize;
  header.dataExtents    = dataExtents;
  header.tileExtents    = tileExtents;
  header.compressMode   = compressMode;
  header.rasterType     = rasterType;
  header.componentType  = componentType;
  runningOffset         = header.FileDataOffset();

  pyrfile = ::open(filename.c_str(), O_WRONLY|O_LARGEFILE|O_CREAT|O_TRUNC, 0666);
  if (pyrfile.fd() < 0) {
    throw khErrnoException(kh::tr("Unable to open %1").arg(filename));
  }
  if (!pyrio::IndexStorage::Header::Write(pyrfile.fd(), header)) {
    throw khException(kh::tr("Error writing %1").arg(filename));
  }
  switch (header.compressMode) {
    case CompressNone:
      // nothing to do
      break;
    case CompressJPEG:
      compressor = TransferOwnership(
          new JPEGCompressor(RasterProductTileResolution,
                             RasterProductTileResolution,
                             1, /* pyrio always compresses one band at a time */
                             compressionQuality));
      if (!compressor->IsValid()) {
        throw khException(kh::tr("compressor initialization failed!"));
      }
      break;
    case CompressLZ:
      compressor = TransferOwnership(NewLZCompressor(compressionQuality,
                                                     header.bandBufSize()));
      break;
    default:
      throw khException(CompressModeName(header.compressMode) +
                        kh::tr(" compressor not supported for pyramid files!"));
      break;
  }
}


template <class SrcTile>
bool
pyrio::Writer::WritePyrTile(uint32 row, uint32 col, const SrcTile &src)
{
  COMPILE_TIME_ASSERT(SrcTile::TileWidth == RasterProductTileResolution,
                      InvalidTileWidth);
  COMPILE_TIME_ASSERT(SrcTile::TileHeight == RasterProductTileResolution,
                      InvalidTileHeight);
  assert(SrcTile::NumComp == header.numComponents ||
         src.IsMonoChromatic() && header.numComponents == 1);
  assert(SrcTile::Storage == header.componentType);
  assert(row < header.levelSize.height);
  assert(col < header.levelSize.width);

  static const uint NumIndexElem = 1 +  header.numComponents;
  static const uint32 UncompressedBandSize = SrcTile::BandBufSize;
  off64_t indexinfo[NumIndexElem];

  indexinfo[0] = runningOffset;

  // seek to the beginning of the tile data
  if (lseek64(pyrfile.fd(), runningOffset, SEEK_SET) < 0) {
    notify(NFY_WARN, "Unable to seek to pyramid tile (offset = %llu): %s",
           static_cast<long long unsigned>(runningOffset),
           khstrerror(errno).c_str());
    return false;
  }
  for (uint b = 0; b <  header.numComponents; ++b) {
    uint32 writesize;
    void *tmpbuf;
    // compress the band data
    if (compressor) {
      writesize = compressor->compress((char*)src.bufs[b]);
      if (writesize == 0) {
        notify(NFY_WARN, "Unable to compress pyramid data");
        return false;
      }
      tmpbuf = compressor->data();
    } else {
      writesize = UncompressedBandSize;
      tmpbuf = src.bufs[b];
    }
    runningOffset += writesize;
    indexinfo[1+b] = writesize;
    // write the band data
    if (!khWriteAll(pyrfile.fd(), tmpbuf, writesize)) {
      notify(NFY_WARN, "Unable to write %u bytes of pyramid data: %s",
             writesize, khstrerror(errno).c_str());
      return false;
    }
  }

  // Load the tile offset and band sizes
  for (uint i = 0; i < NumIndexElem; ++i) {
    indexinfo[i] = ::HostToLittleEndian(indexinfo[i]);
  }
  if (lseek64(pyrfile.fd(),
              pyrio::IndexStorage::FileIndexOffset +
              (off64_t)(sizeof(indexinfo) *
                        (row * header.levelSize.width + col)),
              SEEK_SET) < 0) {
    notify(NFY_WARN, "Unable to seek to pyramid index: %s",
           khstrerror(errno).c_str());
    return false;
  }
  if (!khWriteAll(pyrfile.fd(), indexinfo, sizeof(indexinfo))) {
    notify(NFY_WARN, "Unable to write pyramid index: %s",
           khstrerror(errno).c_str());
    return false;
  }
  return true;
}

// ***** explicit instantiations since this is in the cpp file *****
template bool
pyrio::Writer::WritePyrTile<AlphaProductTile>
(uint32, uint32, const AlphaProductTile &);
template bool
pyrio::Writer::WritePyrTile<ImageryProductTile>
(uint32, uint32, const ImageryProductTile &);
template bool
pyrio::Writer::WritePyrTile<HeightmapFloat32ProductTile>
(uint32, uint32, const HeightmapFloat32ProductTile &);
template bool
pyrio::Writer::WritePyrTile<HeightmapInt16ProductTile>
(uint32, uint32, const HeightmapInt16ProductTile &);


bool
pyrio::SplitPyramidName(const std::string &srcfile,
                        std::string &basename,
                        std::string &suffix) {
  // pyramidio files will always be <filename>NN.pyr
  // pyramidio mask files will always be <filename>NN-mask.pyr
  //   where NN is the 2-digit pixel level 00 - 99
  std::string::size_type len = 0;
  if (EndsWith(srcfile, "-mask.pyr")) {
    len = srcfile.size() - 9;
  } else if (EndsWith(srcfile, ".pyr")) {
    len = srcfile.size() - 4;
  } else {
    // not a well formed pyramid name
    return false;
  }
  if ((len > 2) &&
      std::isdigit(srcfile[len-1]) &&
      std::isdigit(srcfile[len-2])) {
    basename = srcfile.substr(0, len-2);
    suffix   = srcfile.substr(len);
    return true;
  }

  // not a well formed pyramid name
  return false;
}

std::string
pyrio::ComposePyramidName(const std::string &basename,
                          const std::string &suffix,
                          uint level)
{
  std::ostringstream out;
  out << basename << std::setw(2) << std::setfill('0') << level << suffix;
  return out.str();
}


bool
pyrio::FindPyramidMinMax(const std::string &basename,
                         const std::string &suffix,
                         uint &minRet, uint &maxRet)
{
  bool foundTop = false;
  for (int lev = (int)MaxFusionLevel; lev >= 0; --lev) {
    std::string tocheck = ComposePyramidName(basename, suffix, lev);
    if (khExists(tocheck)) {
      if (!foundTop) {
        foundTop = true;
        maxRet = lev;
      }
      minRet = lev;
    } else if (foundTop) {
      // We've walked past the min. We can stop looking.
      break;
    }
  }
  return foundTop;
}

uint
pyrio::FindHighestResPyramidFile(const std::string &srcfile,
                                 std::string *fileRet)
{
  std::string basename;
  std::string suffix;
  if (SplitPyramidName(srcfile, basename, suffix)) {
    uint min, max;
    if (FindPyramidMinMax(basename, suffix, min, max)) {
      if (fileRet) {
        *fileRet = ComposePyramidName(basename, suffix, max);
      }
      return max;
    }
  }
  return 0;
}


uint
pyrio::FindHighestResMaskPyramidFile(const std::string &srcfile,
                                     std::string *fileRet)
{
  std::string filename = khBasename(srcfile);
  std::string dirname  = khDirname(srcfile);
  std::string dirext   = khExtension(dirname);
  if ((dirext == ".kip") ||
      (dirext == ".ktp") ||
      (dirext == ".kmp")) {
    // the pyramid file is part of a product, use new naming scheme
    std::string basename = khDropExtension(dirname) + ".kmp/level";
    uint min, max;
    if (FindPyramidMinMax(basename, ".pyr", min, max)) {
      if (fileRet) {
        *fileRet = ComposePyramidName(basename, ".pyr", max);
      }
      return max;
    }
  } else {
    // the pyramid file is NOT part of a product, use old naming scheme
    std::string basename;
    std::string suffix;
    if (SplitPyramidName(srcfile, basename, suffix)) {
      uint min, max;
      if (FindPyramidMinMax(basename, "-mask.pyr", min, max)) {
        if (fileRet) {
          *fileRet = ComposePyramidName(basename, "-mask.pyr", max);
        }
        return max;
      }
    }
  }
  return 0;
}
