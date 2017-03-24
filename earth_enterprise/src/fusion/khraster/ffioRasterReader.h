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


#ifndef __ffioRasterReader_h
#define __ffioRasterReader_h

#include <compressor.h>
#include <khGuard.h>
#include <ffio/ffioReader.h>
#include "ffioRaster.h"

namespace ffio {
namespace raster {
// currently only used for Heighmap and ImageryCached ff's
// tilesize and numcomp must match that store in the ff
template <class OutTile>
class Reader {
  ffio::Reader ffreader;
  khTypes::StorageEnum datatype;
  Subtype              subtype_;
  TileOrientation      orientation;
  uint32               uncompressedTileSize;

  mutable std::vector<uchar>    readBuf;
  mutable std::vector<uchar>    tmpBuf;
  khDeleteGuard<Compressor> decompressor;

  typedef typename OutTile::PixelType PixelType;
 public:
  Reader(const std::string &ffdir);

  khTypes::StorageEnum srcDatatype(void) const { return datatype; }
  Subtype              subtype(void) const { return subtype_; }

  inline bool FindTile(const khTileAddr &addr,
                       std::string &filename, uint64 &fileOffset,
                       uint32 &dataLen) const {
    return ffreader.FindTile(addr, filename, fileOffset, dataLen);
  }

  // low level read - used if you've already called FindTile
  bool ReadTile(const std::string &filename,
                uint64 fileOffset, uint32 dataLen,
                OutTile &destTile) const;

  // Find and read tile
  bool FindReadTile(const khTileAddr &addr, OutTile &readTile) const;

  inline const ffio::PresenceMask &presence(void) const {
    return ffreader.presence();
  }

  inline void PopulateCoverage(khInsetCoverage &cov) const {
    presence().PopulateCoverage(cov);
  }

  inline bool ValidLevel(uint level) const {
    return presence().ValidLevel(level);
  }

  inline khExtents<uint32> levelTileExtents(uint level) const {
    return presence().levelTileExtents(level);
  }

  inline khLevelCoverage levelCoverage(uint level) const {
    return khLevelCoverage(level, levelTileExtents(level));
  }

  inline void Close(void) const {
    ffreader.Close();
  }

  inline const std::string &ffdir(void) const { return ffreader.ffdir(); }

};

} // namespace ffio::raster
} // namespace ffio

#endif /* __ffioRasterReader_h */
