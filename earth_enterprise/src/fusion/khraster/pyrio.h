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

#ifndef __pyrio_h
#define __pyrio_h

#include <compressor.h>
#include <khGuard.h>
#include <khExtents.h>
#include <khTileAddr.h>
#include <khraster/.idl/RasterProductStorage.h>

class khRasterProductLevel;
class khRasterProduct;

namespace pyrio {
class Header {
 public:
  unsigned int formatVersion;                     // major*10 + minor
  unsigned int numComponents;                     // number of components
  unsigned int level;                             // level of pyramid (pixels)
  khSize<std::uint32_t> levelSize;               // number of tiles in this file
  khExtents<double> dataExtents;          // geographic extents of data,
                                          // may be either in degree or in meter
  khExtents<double> tileExtents;          // geographic extents of tiles
  CompressMode      compressMode;         // type of compression
  RasterProductStorage::RasterType rasterType;
  khTypes::StorageEnum componentType;     // datatype of each component

  std::uint32_t bandBufSize(void) const {
    return khTypes::StorageSize(componentType) *
      RasterProductTileResolution *
      RasterProductTileResolution;
  }
  off64_t FileDataOffset(void) const;
  bool IsMercator() const;
};


class Reader {
  friend class khRasterProduct;
  friend class khRasterProductLevel;
 private:
  Header header_;
  khDeleteGuard<Compressor> compressor;   // compressor for jpeg & LZ
  khReadFileCloser pyrfile;

  // buffer for reading one band of data
  mutable std::uint32_t readBufSize;
  mutable khDeleteGuard<unsigned char, ArrayDeleter> readBuf;

  mutable khDeleteGuard<unsigned char, ArrayDeleter> convertBuf;

  mutable khDeleteGuard<unsigned char, ArrayDeleter> flipBuf;

 public:
  Reader(const std::string &filename);
  inline const Header& header(void) const { return header_; }

  // row & col start at 0 for the first tiles in the pyramid file
  // Requests outside range will return false.
  // destBufs: where to write the pixels (assumed big enough)
  // bands:    which bands to read
  // numbands: how many enties in destBufs & bands
  bool ReadBandBufs(std::uint32_t row, std::uint32_t col,
                    unsigned char* destBufs[], unsigned int bands[],
                    unsigned int numBands,
                    khTypes::StorageEnum destType,
                    bool flipTopToBottom = false) const;


  // convenience routines
  inline bool ReadBandBuf(std::uint32_t row, std::uint32_t col, unsigned int band,
                          unsigned char* destBuf,
                          khTypes::StorageEnum destType,
                          bool flipTopToBottom = false) const {
    return ReadBandBufs(row, col, &destBuf, &band, 1,
                        destType, flipTopToBottom);
  }
  inline bool ReadAllBandBufs(std::uint32_t row, std::uint32_t col,
                              unsigned char* destBufs[],
                              khTypes::StorageEnum destType,
                              const unsigned int numBands,
                              bool flipTopToBottom = false) const {
    unsigned int bands[3] = { 0, 1, 2 };
    return ReadBandBufs(row, col, destBufs, bands,
                        numBands, destType,
                        flipTopToBottom);
  }
};


class Writer {
 private:
  Header header;
  khDeleteGuard<Compressor> compressor;   // compressor for jpeg & LZ
  khWriteFileCloser pyrfile;
  off64_t runningOffset;
 public:
  Writer(const std::string &filename,
         unsigned int numComponents,
         unsigned int level,
         const khSize<std::uint32_t> &levelSize,
         const khExtents<double> &dataExtents,
         const khExtents<double> &tileExtents,
         CompressMode       compressMode,
         RasterProductStorage::RasterType rasterType,
         khTypes::StorageEnum componentType,
         int compressionQuality);

  // SrcTile is of some khRasterTile type
  // row & col start at 0 for the first tiles in the pyramid file
  // Requests outside range will return false.
  // Data must match the type specified in the constructor
  template <class SrcTile>
  bool WritePyrTile(std::uint32_t row, std::uint32_t col, const SrcTile &src);
};

// splits srcfile into pyramid basename and suffix
// returns true if srcfile could be split
bool
SplitPyramidName(const std::string &srcfile,
                 std::string &basename,
                 std::string &suffix);


// composes a pyramid name from its pieces
std::string
ComposePyramidName(const std::string &basename,
                   const std::string &suffix,
                   unsigned int level);

// returns the minimum and maximum levels for the specified pyramid
// returns true if min & max could be found
bool
FindPyramidMinMax(const std::string &basename,
                  const std::string &suffix,
                  unsigned int &minRet, unsigned int &maxRet);

// returns the level of the highest resolution pyramid that matches
// the pyramid specified in srcfile.
// if fileRet is specified, the full filename of the top level file is set
// If one cannot be found, 0 is returned
uint
FindHighestResPyramidFile(const std::string &srcfile,
                          std::string *fileRet = 0);

// Like above but you specify a data pyramid file and it finds the highest
// res of the corresponding mask pyramid (if one exists)
uint
FindHighestResMaskPyramidFile(const std::string &srcfile,
                              std::string *fileRet = 0);

} // namespace pyrio



#endif /* __pyrio_h */
