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


/* Routines to construct a quadtree packet from index information.*/

#include <map>
#include <string>
#include <vector>

#include "keyhole/jpeg_comment_date.h"
#include "protobuf/quadtreeset.pb.h"
#include "common/qtpacket/quadtreepacket.h"
#include "common/qtpacket/quadtree_utils.h"
#include "common/qtpacket/quadtree_processing.h"
#include "common/khConstants.h"
#include "common/khMisc.h"
#include "common/khStringUtils.h"

namespace qtpacket {

// DatedTileInfo is a local struct for passing around the basic info
// related to a DatedImagery tile.
struct DatedTileInfo {
  std::int32_t date_int_;  // Year-Month-Day.
  std::int32_t time_int_;  // Milliseconds after midnight.
  std::int32_t epoch_;
  std::int32_t provider_;

  DatedTileInfo() {}

  // Initialization given date and time.
  DatedTileInfo(std::int32_t date, std::int32_t time, std::int32_t epoch, std::int32_t provider)
      : date_int_(date),
        time_int_(time),
        epoch_(epoch),
        provider_(provider) {
  }

  // Initialization given a time string.
  DatedTileInfo(const std::string& date, std::int32_t epoch, std::int32_t provider)
      : date_int_(keyhole::kUnknownJpegCommentDate.AsYearMonthDayKey()),
        time_int_(0),
        epoch_(epoch),
        provider_(provider) {
    // The "date" should be a valid ISO 8601 UTC date string.
    struct tm time = {0};

    if (ParseUTCTime(date, &time)) {
      keyhole::JpegCommentDate::YearMonthDayKey jpeg_date_int;
      keyhole::JpegCommentDate::YearMonthDayKeyFromInts(
          time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, &jpeg_date_int);
      date_int_ = jpeg_date_int;
      time_int_ = MillisecondsFromMidnight(time);
    }
  }
};

// Add a DatedImagery info to a Format2 quadtree node.
static void AddDatesToNode(const std::map<std::string, DatedTileInfo>&
                           dated_tiles,
                           std::int32_t epoch,
                           const std::string& shared_tile_date,
                           keyhole::QuadtreeNode* node);

// InsertChannel - insert a new channel entry in ascending channel order
void InsertChannel(std::uint16_t channel,
                   std::uint16_t version,
                   KhQuadTreeQuantum16 *node) {
  assert(node);
  for (size_t i = 0; i < node->channel_type.size(); ++i) {
    if (channel < node->channel_type[i]) {
      node->channel_type.insert(node->channel_type.begin() + i, channel);
      node->channel_version.insert(node->channel_version.begin() + i, version);
      return;
    }
  }

  node->channel_type.push_back(channel);
  node->channel_version.push_back(version);
}

// Format 1: uses QuadtreePacket version for Google Earth Client 4.3 and
// earlier.
void ComputeQuadtreePacketFormat1(
    const QuadsetGroup<QuadtreePacketItem> &group,
    int quadtree_packet_version,
    KhQuadTreePacket16 &packet) {
  int num_nodes = group.size();

  const QuadtreeNumbering &numbering =
    QuadtreeNumbering::Numbering(group.quadset_num());

  int num_expected_quadnodes = 0;
  for (int i = 0; i < num_nodes; ++i) {
    if (group.has_children_inorder(i) || !group.Node(i).empty()) {
      ++num_expected_quadnodes;
    }
  }

  packet.Init(num_expected_quadnodes);

  //
  // Main loop: Generate a KhQuadTreeQuantum16 for each non-empty node
  // in the quadset.
  //

  int node_count = 0;
  for (int i = 0; i < num_nodes; ++i) {
    // The nodes must appear in the quadtree packet in inorder
    // traversal of the tree.
    if (group.has_children_inorder(i) || !group.Node(i).empty()) {
      assert(node_count < num_expected_quadnodes);

      KhQuadTreeQuantum16 *node = packet.GetPtr(node_count);
      ++node_count;
      memset(node, 0, sizeof(KhQuadTreeQuantum16));

      const QuadsetGroup<QuadtreePacketItem>::NodeContents
        &contents = group.Node(i);

      // Iterate over channels, recording the highest version number
      // for each one.
      std::vector<std::uint16_t> &channel_type = node->channel_type;
      std::vector<std::uint16_t> &channel_version = node->channel_version;
      int image_data_provider = 0;
      int terrain_data_provider = 0;
      for (size_t j = 0; j < contents.size(); ++j) {
        const QuadtreePacketItem &item = contents[j];
        switch (item.layer_id()) {
          case QuadtreePacketItem::kLayerImagery:
            node->image_version = std::max(node->image_version,
                                           static_cast<std::uint16_t>(item.version()));
            image_data_provider = item.copyright();
            break;

          case QuadtreePacketItem::kLayerTerrain:
            node->terrain_version =
              std::max(node->terrain_version,
                       static_cast<std::uint16_t>(item.version()));
            terrain_data_provider = item.copyright();
            break;

          case QuadtreePacketItem::kLayerDatedImagery:
            // Avoid writting anything for dated imagery in
            // ComputeQuadtreePacketFormat1. Otherwise it gets confused with
            // vectors.
            break;

          default: {
            // Have we already seen this channel number?
            bool found = false;
            for (size_t k = 0; k < channel_type.size(); ++k)
              if (channel_type[k]
                  == item.layer_id() - QuadtreePacketItem::kLayerVectorMin) {
                channel_version[k] =
                  std::max(channel_version[k],
                           static_cast<std::uint16_t>(item.version()));
                found = true;
                break;
              }

            // No; add a new channel (keep channels in order)
            if (!found) {
              InsertChannel(
                  item.layer_id() - QuadtreePacketItem::kLayerVectorMin,
                  item.version(),
                  node);
            }
          }
        }
      }

      // Fill in data provider
      node->image_data_provider = image_data_provider;
      node->terrain_data_provider = terrain_data_provider;

      // Set the flags for the info we have.  "children" is poorly
      // named, in that it holds flags for the info in this node, as
      // well as the cache bit, which is set for nodes that have
      // children.
      if (node->image_version > 0)
        node->children.SetImageBit();

      if (node->terrain_version > 0)
        node->children.SetTerrainBit();

      if (channel_type.size() > 0)
        node->children.SetDrawableBit();

      // Set child flags.  For interior nodes, there's a flag bit for
      // each of the 4 potential children.  For leaves, there is a
      // single "cache node bit" that indicates whether there's any
      // data below the node (in other quadsets).
      int child_indices[4];
      if (numbering.GetChildrenInorder(i, child_indices)) {
        // Interior node: look for child nodes
        for (int k = 0; k < 4; ++k) {
          int child = child_indices[k];
          if (!group.Node(child).empty()
              || group.has_children_inorder(child)) {
            node->children.SetBit(k);
          }
        }
      } else {
        // Bottom level of the quadset: set cache node bit if there's
        // anything below us.
        if (group.has_children_inorder(i)) {
          node->children.SetCacheNodeBit();
          node->cnode_version = quadtree_packet_version;
        }
      }
    }
  }

  // Make sure we got exactly the number of nodes we expected
  assert(num_expected_quadnodes == node_count);
}

// Format 2: uses QuadtreePacket protocol buffer based class, version as of
// Google Earth Client 4.4 (for timemachine).
void ComputeQuadtreePacketFormat2(
    const QuadsetGroup<QuadtreePacketItem> &group,
    int quadtree_packet_version,
    KhQuadTreePacketProtoBuf* packet) {
  int num_nodes = group.size();

  const QuadtreeNumbering &numbering =
    QuadtreeNumbering::Numbering(group.quadset_num());

  // Figure out which nodes have valid information.
  // We precompute this but it could be as easily computed within
  // the inner loop, but this makes it a bit easier to read
  std::vector<bool> is_valid_group(num_nodes, false);
  int num_expected_quadnodes = 0;
  for (int i = 0; i < num_nodes; ++i) {
    const QuadsetGroup<QuadtreePacketItem>::NodeContents&
      contents = group.Node(i);

    if (group.has_children_inorder(i) || !contents.empty()) {
      ++num_expected_quadnodes;
      is_valid_group[i] = true;
    }
  }
  keyhole::QuadtreePacket& packet_proto_buffer = packet->ProtocolBuffer();

  packet_proto_buffer.set_packet_epoch(quadtree_packet_version);

  // Main loop: Generate a SparseQuadtreeNode for each non-empty node
  // in the quadset.
  int node_count = 0;
  for (int inorder_index = 0; inorder_index < num_nodes; ++inorder_index) {
    // The nodes must appear in the quadtree packet in inorder
    // traversal of the tree.
    if (is_valid_group[inorder_index]) {
      ++node_count;
      assert(node_count <= num_expected_quadnodes);

      // Get the node contents from the group so we can fill out
      // our QuadtreeNode.
      const QuadsetGroup<QuadtreePacketItem>::NodeContents&
        contents = group.Node(inorder_index);

      // Add the SparseQuadtreeNode to the protocol buffer packet.
      SparseQuadtreeNode* sparse_node =
        packet_proto_buffer.add_sparsequadtreenode();

      // each sparse node has an associated subindex (one of 3 numbering schemes
      // used for nodes).
      int subindex = numbering.InorderToSubindex(inorder_index);
      sparse_node->set_index(subindex);

      // The real contents of the node are in the subnode.
      keyhole::QuadtreeNode* node = sparse_node->mutable_node();

      // Iterate over channels, recording the highest version number
      // for each one.
      // Also add each layer to the node as we walk through it and collect
      // up our flags for the types of data in this node.
      google::protobuf::uint32 node_flags = 0;
      std::vector<std::uint16_t> channel_types;
      std::vector<std::uint16_t> channel_versions;
      // Collect info on the dated imagery.
      std::map<std::string, DatedTileInfo> dated_tiles;
      std::string shared_tile_date;  // date of main imagery layer.
      for (size_t j = 0; j < contents.size(); ++j) {
        const QuadtreePacketItem &item = contents[j];
        if (item.layer_id() == QuadtreePacketItem::kLayerDatedImagery) {
          // Do not create a layer...we will have one layer for all the dated
          // imagery layers in this quadtree path.
          node_flags |= 1 << keyhole::QuadtreeNode::NODE_FLAGS_IMAGE_BIT;
          // Collect the info for this dated imagery tile.
          // We'll add all the dated imagery info to the node in one shot at
          // the end.
          DatedTileInfo tile(item.date_string(), item.version(),
                             item.copyright());
          dated_tiles[item.date_string()] = tile;
        } else {
          // Other layers should create a layer object in this node.
          keyhole::QuadtreeLayer* layer = node->add_layer();
          layer->set_layer_epoch(item.version());

          switch (item.layer_id()) {
            case QuadtreePacketItem::kLayerImagery: {
              node_flags |= 1 << keyhole::QuadtreeNode::NODE_FLAGS_IMAGE_BIT;
              // Main imagery layer
              layer->set_type(keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY);
              layer->set_provider(item.copyright());
              shared_tile_date = item.date_string();

              // Also, track the current imagery layer as a dated layer.
              // Client seems to expect this.
              {
                dated_tiles[item.date_string()] = DatedTileInfo(
                    item.date_string(), item.version(), item.copyright());
              }

              // GE client requires the default layer
              // to be dated using the oldest POSSIBLE date as well.
              {
                dated_tiles[kTimeMachineOldestDateString] =
                  DatedTileInfo(kTimeMachineOldestDateString, item.version(),
                                item.copyright());
              }
            } break;

            case QuadtreePacketItem::kLayerDatedImagery:
              assert(false);  // Should never get here
              break;

            case QuadtreePacketItem::kLayerTerrain: {
              node_flags |= 1 << keyhole::QuadtreeNode::NODE_FLAGS_TERRAIN_BIT;
              layer->set_type(keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN);
              layer->set_provider(item.copyright());
            } break;

            default: {
              // Vector layers are "Drawables"
              node_flags |= 1 << keyhole::QuadtreeNode::NODE_FLAGS_DRAWABLE_BIT;
              layer->set_type(keyhole::QuadtreeLayer::LAYER_TYPE_VECTOR);
              // Have we already seen this channel number?
              bool found = false;
              // The layer channel type ids in the packets start at 3 for this
              // purpose (seems like trouble but that's what it is).
              std::uint16_t item_channel_id =
                item.layer_id() - QuadtreePacketItem::kLayerVectorMin;
              for (size_t k = 0; k < channel_types.size(); ++k)
                if (channel_types[k] == item_channel_id) {
                  channel_versions[k] =
                    std::max(channel_versions[k],
                             static_cast<std::uint16_t>(item.version()));
                  found = true;
                  break;
                }

              // Not already added: add a new channel (keep channels in order)
              if (!found) {
                // Add the channels to the node.
                keyhole::QuadtreeChannel* channel = node->add_channel();
                channel->set_type(item_channel_id);
                channel->set_channel_epoch(item.version());
              }
            } break;
          }  // switch
        }  // else
      }  // for

      // Set the flags for the info we have.  "children" is poorly
      // named, in that it holds flags for the info in this node, as
      // well as the cache bit, which is set for nodes that have
      // children.

      // Set child flags.  For interior nodes, there's a flag bit for
      // each of the 4 potential children.  For leaves, there is a
      // single "cache node bit" that indicates whether there's any
      // data below the node (in other quadsets).
      int child_inorder_indices[4];
      if (numbering.GetChildrenInorder(inorder_index, child_inorder_indices)) {
        // Interior node: look for child nodes
        for (int k = 0; k < 4; ++k) {
          int child_inorder_index = child_inorder_indices[k];
          if (!group.Node(child_inorder_index).empty()
              || group.has_children_inorder(child_inorder_index)) {
            node_flags |= 1 << k;  // The lowest 4 bits are the
                                   // child occupancy bits.
          }
        }
      } else {
        // Bottom level of the quadset: set cache node bit if there's
        // anything below us.
        if (group.has_children_inorder(inorder_index)) {
          node_flags |= 1 << keyhole::QuadtreeNode::NODE_FLAGS_CACHE_BIT;
          node->set_cache_node_epoch(quadtree_packet_version);
        }
      }
      node->set_flags(node_flags);

      // Add the dated imagery info, if any.
      AddDatesToNode(dated_tiles, quadtree_packet_version,
                     shared_tile_date, node);
    }
  }

  // Make sure we got exactly the number of nodes we expected
  assert(num_expected_quadnodes == node_count);
}

void AddDatesToNode(const std::map<std::string, DatedTileInfo>& dated_tiles,
                    std::int32_t epoch,
                    const std::string& shared_tile_date,
                    keyhole::QuadtreeNode* node) {
  // Since the current imagery layer is part of the dated_tiles
  // (once for it's date and another for 0001:01:01),
  // we only really have dates to add if there's more than 2.
  if (dated_tiles.size() <= 2)
    return;
  keyhole::QuadtreeLayer *quad_tree_layer = node->add_layer();
  quad_tree_layer->set_type(keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY);
  quad_tree_layer->set_layer_epoch(epoch);
  keyhole::QuadtreeImageryDates *date_buffer =
    quad_tree_layer->mutable_dates_layer();
  if (!shared_tile_date.empty()) {
    DatedTileInfo info(shared_tile_date, epoch, 0);
    date_buffer->set_shared_tile_date(info.date_int_);
    date_buffer->set_shared_tile_milliseconds(info.time_int_);
  }
  // Add the dated tiles in ascending date order
  std::map<std::string, DatedTileInfo>::const_iterator iter =
    dated_tiles.begin();
  // No valid date would be -1, so use -1 to trigger the first dated_tiles.
  int last_date = -1;
  keyhole::QuadtreeImageryDatedTile *dated_tile = NULL;
  for (; iter != dated_tiles.end(); ++iter) {
    const DatedTileInfo& info = iter->second;
    if (last_date != info.date_int_) {
      dated_tile = date_buffer->add_dated_tile();
      dated_tile->set_date(info.date_int_);
      dated_tile->set_dated_tile_epoch(info.epoch_);
      dated_tile->set_provider(info.provider_);
      last_date = info.date_int_;
    }
    keyhole::QuadtreeImageryTimedTile* timed_tile =
      dated_tile->add_timed_tiles();
    timed_tile->set_milliseconds(info.time_int_);
    timed_tile->set_timed_tile_epoch(info.epoch_);
    if (info.provider_ != dated_tile->provider()) {
      timed_tile->set_provider(info.provider_);
    }
  }
}

}  // namespace qtpacket
