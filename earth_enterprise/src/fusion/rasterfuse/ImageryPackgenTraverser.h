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

/******************************************************************************
File:        ImageryPackgenTraverser.h
Description:

Changes: Support separate alpha-mask tiles and creating packet in proto-format.

******************************************************************************/
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_IMAGERYPACKGENTRAVERSER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_IMAGERYPACKGENTRAVERSER_H_

#include <string>
#include <vector>

#include "common/khTileAddr.h"
#include "common/khGuard.h"
#include "common/compressor.h"
#include "common/geFilePool.h"
#include "common/packetfile/packetfilewriter.h"
#include "common/proto_streaming_imagery.h"
#include "fusion/khraster/MagnifyQuadrant.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/khraster/khOpacityMask.h"
#include "fusion/rasterfuse/PackgenTraverser.h"
#include "fusion/rasterfuse/RasterBlender.h"
#include "fusion/rasterfuse/RasterMerger.h"

class AttributionByExtents;

namespace fusion {
namespace rasterfuse {

// Tiles config
class ImageryTilesConfig {
 public:
  typedef ImageryProductTile DataTile;
  typedef AlphaProductTile AlphaTile;
  typedef TilespaceConfig<DataTile, AlphaTile> SpaceConfig;
};

// ImageryPrepItem
template <class TilesConfig>
class ImageryPrepItem {
  typedef typename TilesConfig::DataTile DataTile;
  typedef typename TilesConfig::AlphaTile AlphaTile;

 public:
  khOpacityMask::OpacityType opacity;    // Tile opacity
  DataTile         product_tile;         // Data tile
  AlphaTile        alpha_tile;           // Alpha tile
  khOffset<std::uint32_t> product_tile_origin;  // in targetTilespace
  khLevelCoverage  work_coverage;        // in targetTilespace

  inline ImageryPrepItem(void)
         : opacity(khOpacityMask::Unknown) {
  }
};


// Base class for ImageryWorkItem-classes.
template <class TilesConfig>
class ImageryWorkItemBase {
  typedef typename TilesConfig::SpaceConfig SpaceConfig;

 public:
  virtual ~ImageryWorkItemBase() {}

 protected:
  class Piece {
   public:
    khTileAddr target_addr;
    // Tile opacity, populated from ImageryPrepItem.
    khOpacityMask::OpacityType opacity;
    khDeleteGuard<Compressor> compressor;
    khDeleteGuard<Compressor> compressor_alpha;

    explicit Piece(const SpaceConfig &_space_config);
  };

  ImageryWorkItemBase(const SpaceConfig &_space_config,
                      PacketFileWriter* const _writer,
                      const AttributionByExtents &_attributions);

  const SpaceConfig &space_config_;
  PacketFileWriter* const writer;
  const AttributionByExtents &attributions;
  std::vector<unsigned char> interleaved_buf;
  std::vector<unsigned char> alpha_buf;

  khDeletingVector<Piece> pieces;
  unsigned int num_pieces_used;
};

// Class ProtobufImageryWorkItem prepares packet in protobuf-format
// (Earth's DB packets) and write it into packet-bundle file.
template <class TilesConfig>
class ProtobufImageryWorkItem : public ImageryWorkItemBase<TilesConfig> {
  typedef ImageryWorkItemBase<TilesConfig> BaseClass;
  typedef ImageryPrepItem<TilesConfig> PrepItem;

  typedef typename TilesConfig::DataTile DataTile;
  typedef typename TilesConfig::AlphaTile AlphaTile;
  typedef typename TilesConfig::SpaceConfig SpaceConfig;

 public:
  ProtobufImageryWorkItem(const SpaceConfig &_space_config,
                          PacketFileWriter* const _writer,
                          const AttributionByExtents &_attributions);

  ~ProtobufImageryWorkItem() { }
  void DoWork(PrepItem *prep);
  void DoWrite(khMTProgressMeter *progress);

 protected:
  using BaseClass::space_config_;
  using BaseClass::writer;
  using BaseClass::attributions;
  using BaseClass::interleaved_buf;
  using BaseClass::alpha_buf;
  using BaseClass::pieces;
  using BaseClass::num_pieces_used;

  // Packet builder to create packet in protobuf-format.
  geProtobufPacketBuilder packet_builder;

 private:
  void ExtractAndCompressAlphaPiece(typename BaseClass::Piece *piece,
                                    const std::string& date,
                                    const PrepItem *prep,
                                    std::uint32_t subrow, std::uint32_t subcol);
};


// class MercatorImageryWorkItem prepares packet for Map's DB (JPEG-format
// packet if tile has no alpha-data, and 4-channel PNG - if tile contains
// alpha-data) and write it into packet-bundle file.
template <class TilesConfig>
class MercatorImageryWorkItem : public ImageryWorkItemBase<TilesConfig> {
  typedef typename TilesConfig::DataTile DataTile;
  typedef typename TilesConfig::AlphaTile AlphaTile;
  typedef typename TilesConfig::SpaceConfig SpaceConfig;
  typedef ImageryPrepItem<TilesConfig> PrepItem;
  typedef ImageryWorkItemBase<TilesConfig> BaseClass;

 public:
  MercatorImageryWorkItem(const SpaceConfig &_space_config,
                          PacketFileWriter* const _writer,
                          const AttributionByExtents &_attributions);
  ~MercatorImageryWorkItem() { }
  void DoWork(PrepItem *prep);
  void DoWrite(khMTProgressMeter *progress);

 protected:
  using BaseClass::space_config_;
  using BaseClass::writer;
  using BaseClass::attributions;
  using BaseClass::interleaved_buf;
  using BaseClass::alpha_buf;
  using BaseClass::pieces;
  using BaseClass::num_pieces_used;
};

// ProtobufImageryPackgenBaseConfig
template <class TilesConfig>
class ProtobufImageryPackgenBaseConfig {
 public:
  typedef typename TilesConfig::DataTile  DataTile;
  typedef typename TilesConfig::AlphaTile AlphaTile;
  typedef typename TilesConfig::SpaceConfig SpaceConfig;
  typedef ImageryPrepItem<TilesConfig> PrepItem;
  typedef ProtobufImageryWorkItem<TilesConfig> WorkItem;
  typedef ImageryRasterBlender Blender;
  typedef ImageryRasterMerger Merger;
};

// MercatorImageryPackgenBaseConfig
template <class TilesConfig>
class MercatorImageryPackgenBaseConfig {
 public:
  typedef typename TilesConfig::DataTile  DataTile;
  typedef typename TilesConfig::AlphaTile AlphaTile;
  typedef typename TilesConfig::SpaceConfig SpaceConfig;
  typedef ImageryPrepItem<TilesConfig> PrepItem;
  typedef MercatorImageryWorkItem<TilesConfig> WorkItem;
  typedef ImageryRasterBlender Blender;
  typedef ImageryRasterMerger Merger;
};

// ImageryPackgenTraverser
template <class Config>
class ImageryPackgenTraverser
    : public PackgenTraverser<Config> {
  typedef PackgenTraverser<Config> BaseClass;
  typedef typename BaseClass::PrepItem PrepItem;
  typedef typename Config::SpaceConfig SpaceConfig;

 public:
  ImageryPackgenTraverser(const PacketLevelConfig &config,
                          geFilePool &file_pool_,
                          const std::string &output);

  PrepItem* NewPrepItem(void);
  void Traverse(unsigned int numcpus);

 protected:
  typedef typename BaseClass::WorkItem WorkItem;

  void PrepAddr(const khTileAddr &productAddr);
  virtual void PrepLoop(void);
  using BaseClass::IsMercator;
  using BaseClass::DoTraverse;

  SpaceConfig item_space_config_;
  PacketFileWriter writer_;
  using BaseClass::config;
  using BaseClass::productCoverage;
  using BaseClass::fetcher;
  using BaseClass::progress;
  using BaseClass::prepEmpty;
  using BaseClass::prepFull;
};

// ProtobufImageryPackgenTraverser
class ProtobufImageryPackgenTraverser
    : public ImageryPackgenTraverser <ProtobufImageryPackgenBaseConfig<
                                       ImageryTilesConfig> > {
 public:
  typedef ImageryPackgenTraverser <ProtobufImageryPackgenBaseConfig<
                                    ImageryTilesConfig> > BaseClass;

  ProtobufImageryPackgenTraverser(const PacketLevelConfig &config,
                                  geFilePool &file_pool_,
                                  const std::string &output);

  virtual WorkItem* NewWorkItem(void);
};

// MercatorImageryPackgenTraverser
class MercatorImageryPackgenTraverser
    : public ImageryPackgenTraverser <MercatorImageryPackgenBaseConfig<
                                       ImageryTilesConfig> > {
 public:
  typedef ImageryPackgenTraverser <MercatorImageryPackgenBaseConfig<
                                    ImageryTilesConfig> > BaseClass;

  MercatorImageryPackgenTraverser(const PacketLevelConfig &config,
                                  geFilePool &file_pool_,
                                  const std::string &output);

  virtual WorkItem* NewWorkItem(void);
};

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_IMAGERYPACKGENTRAVERSER_H_
