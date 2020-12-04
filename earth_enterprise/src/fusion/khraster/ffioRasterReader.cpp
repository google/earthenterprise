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


#include "ffioRasterReader.h"
#include <khException.h>
#include "CastTile.h"
#include "Interleave.h"

template <class OutTile>
ffio::raster::Reader<OutTile>::Reader(const std::string &ffdir) :
    ffreader(ffdir) {
  static_assert(OutTile::TileWidth == OutTile::TileHeight,
                      "Non Square Tile");

  if (ffreader.type() != ffio::Raster) {
    throw khException(kh::tr("%1 is not a raster ff").arg(ffdir.c_str()));
  }

  // fetch and verify the raster specific data store in the index
  IndexTypeData typeData;
  static_assert(sizeof(typeData) ==
                      sizeof(ffio::IndexReader("").typeData),
                      "Invalid Type Data Size");
  memcpy(&typeData, ffreader.typeData(), sizeof(typeData));
  typeData.LittleEndianToHost();
  if (OutTile::TileWidth != typeData.tilesize) {
    throw khException(kh::tr("Incompatible tile size  %1 and %2")
                      .arg(OutTile::TileWidth).arg(typeData.tilesize));
  }
  if (OutTile::NumComp != typeData.numcomp) {
    throw khException(kh::tr("Incompatible number of components %1 and %2")
                      .arg(OutTile::NumComp).arg(typeData.numcomp));
  }

  // initialize the rest of my members from my typeData
  datatype    = (khTypes::StorageEnum)typeData.datatype;
  subtype_    = (Subtype)typeData.subtype;
  orientation = (TileOrientation)typeData.orientation;
  uncompressedTileSize = typeData.tilesize * typeData.tilesize *
                         typeData.numcomp *
                         khTypes::StorageSize(datatype);


  // configure the decompressor
  switch ((CompressMode)typeData.compression) {
    case CompressNone:
      // nothing to do
      break;
    case CompressLZ:
      decompressor = TransferOwnership(
          NewLZCompressor(0 /* level - not used here */,
                          uncompressedTileSize));
      break;
    case CompressJPEG:
      if (khTypes::StorageSize(datatype) != 1) {
        throw khException
          (kh::tr
           ("Internal Error: JPEG compression on non-byte type %1")
           .arg(khTypes::StorageName(datatype)));
      }
      decompressor = TransferOwnership(
          new JPEGCompressor(typeData.tilesize, typeData.tilesize,
                             typeData.numcomp,
                             75 /* quality - not used here */));
      if (!decompressor->IsValid()) {
        throw khException(kh::tr
                          ("Unable to initialize JPEG decompressor"));
      }
      break;
    case CompressDXT1:
      throw khException(kh::tr("Unable to initialize DXT1 decompressor"));
      break;
    case CompressPNG:
      throw khException(kh::tr("Unable to decompress PNG"));
  }
}


template <class OutTile>
bool
ffio::raster::Reader<OutTile>::ReadTile(const std::string &filename,
                                        std::uint64_t fileOffset, std::uint32_t dataLen,
                                        OutTile &destTile) const {
  const std::uint32_t srcBufSize = OutTile::TotalPixelCount *
                            khTypes::StorageSize(datatype);


  // ***** Special Case - we can read directly into our destTile *****
  if ((OutTile::Storage == datatype) &&
      (OutTile::NumComp == 1) &&
      !decompressor) {
    if (dataLen != srcBufSize) {
      notify(NFY_WARN,
             "Invalid ff raster data size %u != %u",
             dataLen, srcBufSize);
      return false;
    }

    return ffreader.ReadBlock(filename, fileOffset, dataLen,
                              (unsigned char*)destTile.bufs[0]);
  }

  // ***** read the raw data from the ff *****
  readBuf.resize(std::max(dataLen, uncompressedTileSize));
  if (!ffreader.ReadBlock(filename, fileOffset, dataLen, &readBuf[0])) {
    // NFY_WARN already emitted
    return false;
  }

  // ***** uncompress if we have too *****
  unsigned char *uncompressedBuf;
  std::uint32_t uncompressedSize;
  if (decompressor) {
    if ((OutTile::Storage == datatype) &&
        (OutTile::NumComp == 1)) {
      // we can uncompress directly into our destTile
      uncompressedBuf = (unsigned char*)destTile.bufs[0];
    } else {
      tmpBuf.resize(srcBufSize);
      uncompressedBuf = &tmpBuf[0];
    }
    uncompressedSize =
      decompressor->decompress((char*)&readBuf[0], readBuf.size(),
                               (char*)uncompressedBuf);
    if (uncompressedSize == 0) {
      notify(NFY_WARN,
             "Unable to decompress ff raster data from %s", filename.c_str());
      return false;
    }
  } else {
    uncompressedBuf  = &readBuf[0];
    uncompressedSize = readBuf.size();
  }
  if (uncompressedSize != srcBufSize) {
    notify(NFY_WARN,
           "Invalid ff raster data size read from %s: %u != %u",
           filename.c_str(), uncompressedSize, srcBufSize);
    return false;
  }
  if ((OutTile::Storage == datatype) &&
      (OutTile::NumComp == 1)) {
    // we already have our answer where we want it, just return
    assert(uncompressedBuf == (unsigned char*)destTile.bufs[0]);
    return true;
  }
  assert(uncompressedBuf != (unsigned char*)destTile.bufs[0]);


  // ***** massage the data into the format we want *****
  if (OutTile::NumComp > 1) {
    if (OutTile::Storage == datatype) {
      // we need to extract from interleaved format
      SeparateComponents(destTile,
                         (typename OutTile::PixelType*)uncompressedBuf,
                         orientation);
    } else {
      // we need to extract from interleaved format and convert
      SeparateComponents(destTile, uncompressedBuf, orientation);
    }
  } else {
    // we just need to convert the datatype of a single band
    void *srcBufs[1] = {
      uncompressedBuf
    };
    CastTile(destTile, srcBufs, datatype);
  }
  return true;
}


template <class OutTile>
bool
ffio::raster::Reader<OutTile>::FindReadTile(const khTileAddr &addr,
                                            OutTile &destTile) const {
  std::string filename;
  std::uint64_t fileOffset = 0;
  std::uint32_t dataLen = 0;
  if (FindTile(addr, filename, fileOffset, dataLen)) {
    return ReadTile(filename, fileOffset, dataLen, destTile);
  } else {
    notify(NFY_WARN,
           "Unable to find tile (lrc) %u,%u,%u in %s",
           addr.level, addr.row, addr.col, ffreader.ffdir().c_str());
    return false;
  }
}


// ****************************************************************************
// ***  explicit template instantiation
// ***
// ***  We know there will only ever be a small number of instantiations of
// ***  this class. Since we define some of the template's member functions
// ***  here in the .cpp, we need to explicitly instantiate the class.
// ****************************************************************************
// templates on read tile (FF)
template class ffio::raster::Reader<ImageryProductTile>;
template class ffio::raster::Reader<HeightmapFloat32ProductTile>;
template class ffio::raster::Reader<AlphaProductTile>;
