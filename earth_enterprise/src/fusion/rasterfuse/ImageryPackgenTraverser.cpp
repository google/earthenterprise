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


/******************************************************************************
File:        ImageryPackgenTraverser.cpp
Description:

******************************************************************************/
#include "fusion/rasterfuse/ImageryPackgenTraverser.h"

#include <string.h>
#include <functional>

#include "fusion/khraster/Interleave.h"
#include "fusion/khraster/AttributionByExtents.h"
#include "common/khTileTraversal.h"
#include "common/khstl.h"
#include "common/khFileUtils.h"
#include "common/khConstants.h"
#include "fusion/autoingest/geAssetRoot.h"
#include "fusion/autoingest/.idl/storage/AssetDefs.h"


namespace fusion {
namespace rasterfuse {

// JPEQ quality parameter for compression imagery product tiles.
const int kJpegQuality = 75;

// ****************************************************************************
// ***  ImageryWorkItem
// ****************************************************************************
template <class TilesConfig>
ImageryWorkItemBase<TilesConfig>::ImageryWorkItemBase(
    const SpaceConfig &_space_config,
    PacketFileWriter* const _writer,
    const AttributionByExtents &_attributions)
    : space_config_(_space_config),
      writer(_writer),
      attributions(_attributions),
      interleaved_buf(space_config_.TargetBufSize()),
      alpha_buf(space_config_.TargetAlphaBufSize()),
      pieces(),
      num_pieces_used(0) {
  unsigned int ratio = space_config_.RasterProductTileSize() /
               space_config_.TargetTileSize();
  unsigned int totalPieces = ratio * ratio;
  pieces.reserve(totalPieces);
  for (unsigned int i = 0; i < totalPieces; ++i) {
    pieces.push_back(new Piece(space_config_));
  }
}

template <class TilesConfig>
ImageryWorkItemBase<TilesConfig>::Piece::Piece(
    const SpaceConfig &_space_config)
    : target_addr(0, 0, 0),
      opacity(khOpacityMask::Unknown),
      compressor(TransferOwnership(
          new JPEGCompressor(_space_config.TargetTileSize(),
                             _space_config.TargetTileSize(),
                             _space_config.DataNumComp(),
                             kJpegQuality))),
      // create 1- or 4-channel(mercator DB) PNG compressor.
      compressor_alpha(TransferOwnership(
          NewPNGCompressor(
              _space_config.TargetTileSize(),
              _space_config.TargetTileSize(),
              (_space_config.product_tilespace.IsMercator() ?
               (_space_config.DataNumComp() +
                _space_config.AlphaNumComp()) :
               _space_config.AlphaNumComp())))) {
  if (!compressor->IsValid()) {
    throw khException(kh::tr("compressor initialization failed!"));
  }

  if (!compressor_alpha->IsValid()) {
    throw khException(kh::tr("alpha-data compressor initialization failed!"));
  }
}


// ProtobufImageryWorkItem
template <class TilesConfig>
ProtobufImageryWorkItem<TilesConfig>::ProtobufImageryWorkItem(
    const SpaceConfig &_space_config,
    PacketFileWriter* const _writer,
    const AttributionByExtents &_attributions)
    : BaseClass(_space_config, _writer, _attributions),
      // Calculate size of packet data:
      // packet_data_size = (sizeof_image_data / jpeg_compression_factor) +
      //     (sizeof_image_alpha / png_compression_factor);
      // typical compression ratio jpeg(Q75): at least 10:1,
      // png for alpha band: assume at least 10:1.
      // We use jpeg_compression_factor equal 5 and png_compression_factor
      // equal 4 to be sure that we allocate large enough memory for our packet
      // to exclude reallocation.
      packet_builder(
          (space_config_.TargetTileSize() *
           space_config_.TargetTileSize() *
           space_config_.DataNumComp()) / kJpegCompressionRatio +
          (space_config_.TargetTileSize() *
           space_config_.TargetTileSize() *
           space_config_.AlphaNumComp()) / kPngCompressionAlphaRatio) {
}

template <class TilesConfig>
void ProtobufImageryWorkItem<TilesConfig>::DoWork(PrepItem *prep) {
  num_pieces_used = 0;
  for (std::uint32_t row = prep->work_coverage.extents.beginRow();
       row < prep->work_coverage.extents.endRow(); ++row) {
    for (std::uint32_t col = prep->work_coverage.extents.beginCol();
         col < prep->work_coverage.extents.endCol(); ++col) {
      typename BaseClass::Piece *piece = pieces[num_pieces_used++];
      piece->target_addr = khTileAddr(prep->work_coverage.level,
                                      row, col);
      // extract my piece of the bigger tile
      const std::uint32_t subrow = row - prep->product_tile_origin.row();
      const std::uint32_t subcol = col - prep->product_tile_origin.col();
      const std::uint32_t tile_size = space_config_.TargetTileSize();

      ExtractAndInterleave(
          prep->product_tile,
          subrow * tile_size, subcol * tile_size,
          khSize< unsigned int> (tile_size, tile_size),
          space_config_.target_tilespace.orientation,
          (typename DataTile::PixelType*)&interleaved_buf[0]);

      // Add the acquisition date here.
      const std::string& date =
          attributions.GetAcquisitionDate(piece->target_addr);
      piece->compressor->SetAcquisitionDate(date);

      // Compress the small tile
      if (piece->compressor->compress(
              reinterpret_cast<char*>(&interleaved_buf[0])) == 0) {
        throw khException(kh::tr("Unable to compress tile"));
      }

      piece->opacity = prep->opacity;
      if (khOpacityMask::Opaque != piece->opacity) {
        ExtractAndCompressAlphaPiece(piece, date, prep, subrow, subcol);
      } else {
        assert(khOpacityMask::Opaque == piece->opacity);
        if (prep->work_coverage.level == 1) {
          // For the tiles at level 1, fill the alpha band areas beyond
          // 90/-90 degrees with transparent value.
          // Note: The orientation (LowerLeft/UpperLeft) of raster data in
          // product tile is not important since we symmetrically fill north
          // and south areas of alpha product tile.
          khExtents<std::uint32_t> extents(
              khOffset<std::uint32_t>(RowColOrder,
                               subrow * tile_size + subrow * (tile_size >> 1),
                               subcol * tile_size),
              khSize< unsigned int> (tile_size, tile_size >> 1));
          prep->alpha_tile.FillExtentWithZeros(extents);

          // Set opacity to Amalgam since half of tile is transparent, while
          // other half is opaque.
          piece->opacity = khOpacityMask::Amalgam;
          ExtractAndCompressAlphaPiece(piece, date, prep, subrow, subcol);
        }
      }
    }
  }
}

template <class TilesConfig>
void ProtobufImageryWorkItem<TilesConfig>::DoWrite(
    khMTProgressMeter *progress) {
  for (unsigned int i = 0; i < num_pieces_used; ++i) {
    typename BaseClass::Piece* piece = pieces[i];

    // Set image data.
    packet_builder.SetImageData(piece->compressor->data(),
                                piece->compressor->dataLen());

    // Set image alpha.
    if (khOpacityMask::Opaque != piece->opacity) {
      packet_builder.SetImageAlpha(piece->compressor_alpha->data(),
                                   piece->compressor_alpha->dataLen());
    }

    // Build packet.
    char *packet_buf;
    std::uint32_t packet_size;
    packet_builder.Build(&packet_buf, &packet_size);

    // Write packet.
    std::uint32_t inset_id = attributions.GetInsetId(piece->target_addr);
    writer->WriteAppendCRC(piece->target_addr,
                           packet_buf,
                           packet_size,
                           inset_id);
  }
  progress->incrementDone(num_pieces_used);
}

template <class TilesConfig>
void ProtobufImageryWorkItem<TilesConfig>::ExtractAndCompressAlphaPiece(
    typename BaseClass::Piece *piece,
    const std::string& date,
    const PrepItem *prep, std::uint32_t subrow, std::uint32_t subcol) {
  const std::uint32_t tile_size = space_config_.TargetTileSize();
  // Extract my piece of bigger alpha tile.
  ExtractAndInterleave(
      prep->alpha_tile,
      subrow * tile_size, subcol * tile_size,
      khSize< unsigned int> (tile_size, tile_size),
      space_config_.target_tilespace.orientation,
      (typename AlphaTile::PixelType*)&alpha_buf[0]);

  // Add the acquisition date here.
  piece->compressor_alpha->SetAcquisitionDate(date);
  // Compress the small alpha tile.
  if (piece->compressor_alpha->compress(
          reinterpret_cast<char*>(&alpha_buf[0])) == 0) {
    throw khException(kh::tr("Unable to compress alpha tile"));
  }
}

// MercatorImageryWorkItem
template <class TilesConfig>
MercatorImageryWorkItem<TilesConfig>::MercatorImageryWorkItem(
    const SpaceConfig &_space_config,
    PacketFileWriter* const _writer,
    const AttributionByExtents &_attributions)
    : BaseClass(_space_config, _writer, _attributions) {
}

template <class TilesConfig>
void MercatorImageryWorkItem<TilesConfig>::DoWork(PrepItem *prep) {
  // Create packets in format for Map's DB:
  //  jpeg-packet for "opaque"-tiles(no alpha).
  //  4-channel png for other tiles(with alpha).
  num_pieces_used = 0;
  for (std::uint32_t row = prep->work_coverage.extents.beginRow();
       row < prep->work_coverage.extents.endRow(); ++row) {
    for (std::uint32_t col = prep->work_coverage.extents.beginCol();
         col < prep->work_coverage.extents.endCol(); ++col) {
      typename BaseClass::Piece *piece = pieces[num_pieces_used++];
      piece->target_addr = khTileAddr(prep->work_coverage.level,
                                      row, col);
      // extract my piece of the bigger tile
      std::uint32_t subrow = row - prep->product_tile_origin.row();
      std::uint32_t subcol = col - prep->product_tile_origin.col();
      std::uint32_t tile_size = space_config_.TargetTileSize();

      piece->opacity = prep->opacity;

      const std::string& date =
          attributions.GetAcquisitionDate(piece->target_addr);

      if (khOpacityMask::Opaque == piece->opacity) {  // Tile is opaque.
        //  Create jpeg-data.
        ExtractAndInterleave(
            prep->product_tile,
            subrow * tile_size, subcol * tile_size,
            khSize< unsigned int> (tile_size, tile_size),
            space_config_.target_tilespace.orientation,
            (typename DataTile::PixelType*)&interleaved_buf[0]);

        // Add the acquisition date here.
        piece->compressor->SetAcquisitionDate(date);

        // Compress the small tile
        if (piece->compressor->compress(
                reinterpret_cast<char*>(&interleaved_buf[0])) == 0) {
          throw khException(kh::tr("Unable to compress tile"));
        }
      } else {  // Tile is not opaque.
        //  Create png-data.
        // Add the acquisition date here.
        piece->compressor_alpha->SetAcquisitionDate(date);

        const std::uint32_t alpha_pos = DataTile::NumComp;
        const std::uint32_t step = DataTile::NumComp + AlphaTile::NumComp;
        // Extract my piece of bigger data tile.
        ExtractAndInterleave(
            prep->product_tile,
            subrow * tile_size, subcol * tile_size,
            khSize< unsigned int> (tile_size, tile_size),
            space_config_.target_tilespace.orientation,
            (typename DataTile::PixelType*)&alpha_buf[0],
            step);

        // Extract my piece of bigger alpha tile.
        ExtractAndInterleave(
            prep->alpha_tile,
            subrow * tile_size, subcol * tile_size,
            khSize< unsigned int> (tile_size, tile_size),
            space_config_.target_tilespace.orientation,
            (typename AlphaTile::PixelType*)&alpha_buf[alpha_pos],
            step);

        // Compress the small alpha tile.
        if (piece->compressor_alpha->compress(
                reinterpret_cast<char*>(&alpha_buf[0])) == 0) {
          throw khException(kh::tr("Unable to compress tile with alpha"));
        }
      }
    }
  }
}

template <class TilesConfig>
void MercatorImageryWorkItem<TilesConfig>::DoWrite(
    khMTProgressMeter *progress) {
  for (unsigned int i = 0; i < num_pieces_used; ++i) {
    typename BaseClass::Piece* piece = pieces[i];

    std::uint32_t inset_id = attributions.GetInsetId(piece->target_addr);
    if (khOpacityMask::Opaque == piece->opacity) {  // Tile is opaque.
      // compressor will have left at least kCRC32Size bytes available
      // on the end
      writer->WriteAppendCRC(piece->target_addr,
                             piece->compressor->data(),
                             piece->compressor->dataLen()+kCRC32Size,
                             inset_id);
    } else {   // Tile is not opaque.
      // compressor will have left at least kCRC32Size bytes available
      // on the end
      writer->WriteAppendCRC(piece->target_addr,
                             piece->compressor_alpha->data(),
                             piece->compressor_alpha->dataLen()+kCRC32Size,
                             inset_id);
    }
  }
  progress->incrementDone(num_pieces_used);
}


// ****************************************************************************
// ***  ImageryPackgenTraverser
// ****************************************************************************
template <class Config>
ImageryPackgenTraverser<Config>::ImageryPackgenTraverser(
    const PacketLevelConfig &cfg,
    geFilePool &file_pool_,
    const std::string &output)
    : BaseClass(cfg, output,
                TranslateLevelCoverage(ClientImageryTilespaceBase,
                                       cfg.coverage,
                                       RasterProductTilespaceBase),
                ffio::raster::ImageryCached),
      item_space_config_(RasterProductTilespace(IsMercator()),
                         ClientImageryTilespaceBase),
      writer_(file_pool_, output, true /* overwrite */) {
}

template <class Config>
typename ImageryPackgenTraverser<Config>::PrepItem*
ImageryPackgenTraverser<Config>::NewPrepItem(void) {
  return new PrepItem();
}

template <class Config>
void ImageryPackgenTraverser<Config>::PrepAddr(const khTileAddr &productAddr) {
  // Wait until it's OK for me to go again. This
  // throttles the reads.
  PrepItem *prep = prepEmpty.pop();

  // Fetch the containing product tile and its alpha mask.
  prep->opacity =
      fetcher->Fetch(productAddr, &prep->product_tile, &prep->alpha_tile);

  // Translate this productAddr into target_tilespace coverage
  khLevelCoverage thisTileCoverage
    (TranslateLevelCoverage(item_space_config_.product_tilespace,
                            khLevelCoverage(productAddr),
                            item_space_config_.target_tilespace));

  // Intersect it with what we really want
  khExtents<std::uint32_t> workExtents
    (khExtents<std::uint32_t>::Intersection(thisTileCoverage.extents,
                                     config.coverage.extents));

  if (prep->opacity == khOpacityMask::Transparent) {
    // The only way that opacity will be transparent is if I'm supposed
    // to skip transparent tiles.

    // figure how many we're skipping
    std::int64_t numSkipped = static_cast<std::int64_t>(workExtents.width()) *
                       static_cast<std::int64_t>(workExtents.height());
    progress->incrementSkipped(numSkipped);

    prepEmpty.push(prep);
  } else {
    prep->product_tile_origin = thisTileCoverage.extents.origin();
    prep->work_coverage = khLevelCoverage(config.coverage.level,
                                          workExtents);
    // Add it to the work queue.
    prepFull.push(prep);
  }
}

template <class Config>
void ImageryPackgenTraverser<Config>::PrepLoop(void) {
  try {
    TileAlignedTraversal
      (RasterProductTilespace(IsMercator()),
       productCoverage,
       ImageryToProductLevel(StartImageryLevel),
       unary_obj_closure(this, &ImageryPackgenTraverser::PrepAddr));
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "PrepLoop: %s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "PrepLoop: Unknown error");
  }
}

template <class Config>
void ImageryPackgenTraverser<Config>::Traverse(unsigned int numcpus) {
  DoTraverse(numcpus);
  writer_.Close();
}

// ProtobufImageryPackgenTraverser
ProtobufImageryPackgenTraverser::ProtobufImageryPackgenTraverser(
    const PacketLevelConfig &cfg,
    geFilePool &file_pool_,
    const std::string &output)
    : BaseClass(cfg, file_pool_, output) {
}

ProtobufImageryPackgenTraverser::WorkItem*
ProtobufImageryPackgenTraverser::NewWorkItem(void) {
    return new WorkItem(item_space_config_, &writer_, attributions);
}

// MercatorImageryPackgenTraverser
MercatorImageryPackgenTraverser::MercatorImageryPackgenTraverser(
    const PacketLevelConfig &cfg,
    geFilePool &file_pool_,
    const std::string &output)
    : BaseClass(cfg, file_pool_, output) {
}

MercatorImageryPackgenTraverser::WorkItem*
MercatorImageryPackgenTraverser::NewWorkItem(void) {
    return new WorkItem(item_space_config_, &writer_, attributions);
}

}  // namespace rasterfuse
}  // namespace fusion

// ****************************************************************************
// ***  explicit instantiaion of base class
// ****************************************************************************

#include "fusion/rasterfuse/PackgenTraverserImpl.h"
template class fusion::rasterfuse::ImageryPackgenTraverser <
  fusion::rasterfuse::ProtobufImageryPackgenBaseConfig<
    fusion::rasterfuse::ImageryTilesConfig> >;
template class fusion::rasterfuse::ImageryPackgenTraverser <
  fusion::rasterfuse::MercatorImageryPackgenBaseConfig<
    fusion::rasterfuse::ImageryTilesConfig> >;
