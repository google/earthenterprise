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


#include "fusion/khraster/ffioRasterWriter.h"

#include "common/khException.h"
#include "fusion/khraster/ffioRaster.h"
#include "fusion/khraster/Interleave.h"

template <class TileType>
ffio::raster::Writer<TileType>::Writer(
    ffio::raster::Subtype subtype,
    const std::string &outdir,
    const khInsetCoverage &coverage)
    : GridIndexedWriter(
        ffio::Raster, outdir, coverage,
        // only imagery ff's need to be split - they are
        // indexed into a khdb (which has a limit)
        (subtype == Imagery) ? ffio::Writer::DefaultSplitSize : 0),
      tilespace(TilespaceFromSubtype(subtype)) {
  // initialize compressDef
  switch (subtype) {
    case Imagery:
      if (getenv("KH_MOBILE_DXT_IMAGE_TILE")) {
        compressDef = CompressDef(CompressDXT1, 0);
      } else {
        compressDef = CompressDef(CompressJPEG, 75);
      }
      break;
    case ImageryCached:
    case HeightmapCached:
    case AlphaCached:
      compressDef = CompressDef(CompressLZ, 5);
  }

  // populate and set our type specific index data header
  {
    static_assert(TileType::TileWidth == TileType::TileHeight,
                        "Non Square Tile");
    IndexTypeData typeData(subtype,
                           TileType::TileWidth,
                           TileType::NumComp,
                           khTypes::Helper<PixelType>::Storage,
                           tilespace.orientation,
                           compressDef.mode);
    typeData.HostToLittleEndian();
    SetLittleEndianTypeData(&typeData);
  }
}

template <class TileType>
khTransferGuard<Compressor>
ffio::raster::Writer<TileType>::MakeCompressor(void) {
  khTransferGuard<Compressor> compressor(0);

  int width = TileType::TileWidth;
  int height = TileType::TileHeight;
  int comp = TileType::NumComp;

  // work-around until projects can specify output tile size
  const char* mobile_mode = getenv("KH_MOBILE_MINIFY");

  switch (compressDef.mode) {
    case CompressNone:
      // NoOp
      break;
    case CompressJPEG:
      assert(sizeof(PixelType) == 1);

      if (mobile_mode != NULL) {
        width = 64;
        height = 64;
      }

      compressor = new JPEGCompressor(width, height, comp,
                                      compressDef.quality);
      if (!compressor->IsValid())
        throw khException(kh::tr("JPG compressor initialization failed!"));

      if (mobile_mode != NULL) {
        notify(NFY_WARN, "KH_MOBILE_MINIFY is set!  "
               "Will minify image tiles 4:1 (256x256 -> 64x64)");
        compressor = new MinifyCompressor(compressor.take(),
                                          TileType::TileWidth,
                                          TileType::TileHeight,
                                          TileType::NumComp);
      }
      break;
    case CompressLZ:
      compressor = NewLZCompressor(compressDef.quality,
                                   TileType::TotalBufSize);
      break;
    case CompressDXT1:
      assert(sizeof(PixelType) == 1);
      compressor = new DXT1Compressor(GL_RGB, GL_UNSIGNED_BYTE,
                                      width, height);
      break;
    case CompressPNG:
      compressor = NewPNGCompressor(width, height, comp);
      if (!compressor->IsValid())
        throw khException(kh::tr("PNG compressor initialization failed!"));
      break;
  }

  return compressor;
}



template <class TileType>
void ffio::raster::Writer<TileType>::WriteTile(const khTileAddr &addr,
                                               const TileType &tile) {
  static_assert(TileType::TileWidth == TileType::TileHeight,
                      "Non Square Tile");

  // interleave the tile before writing - for now all ffio files of more
  // than one band must be interleaved
  unsigned char *buf;
  if ((tilespace.orientation != StartLowerLeft) ||
      (TileType::NumComp > 1)) {
    interleaveBuffer.resize(TileType::TotalBufSize);
    ExtractAndInterleave(tile, 0, 0,
                         khSize< unsigned int> (TileType::TileWidth,
                                      TileType::TileHeight),
                         tilespace.orientation,
                         (typename TileType::PixelType*)&interleaveBuffer[0]);
    buf = &interleaveBuffer[0];
  } else {
    buf = reinterpret_cast<unsigned char*>(tile.bufs[0]);
  }

  // make a compressor if we need it
  if (!compressor && (compressDef.mode != CompressNone)) {
    compressor = MakeCompressor();
  }

  if (compressor) {
    if (compressor->compress(reinterpret_cast<char*>(buf)) == 0) {
      throw khException(kh::tr("Unable to compress tile"));
    }
    WritePacket(compressor->data(),
                compressor->dataLen(),
                compressor->bufSize(),
                addr);
  } else {
    WritePacket(reinterpret_cast<char*>(buf),
                TileType::TotalBufSize,
                TileType::TotalBufSize,
                addr);
  }
}



// ****************************************************************************
// ***  explicit template instantiation
// ***
// ***  We know there will only ever be a small number of instantiations of
// ***  this class. Since we define some of the template's member functions
// ***  here in the .cpp, we need to explicitly instantiate the class.
// ****************************************************************************
template class ffio::raster::Writer<ImageryFFTile>;
template class ffio::raster::Writer<ImageryProductTile>;
template class ffio::raster::Writer<HeightmapFloat32ProductTile>;
template class ffio::raster::Writer<AlphaProductTile>;
