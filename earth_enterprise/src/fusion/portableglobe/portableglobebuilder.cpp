// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
// Copyright 2020 Open GEE Contributors
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


// Methods for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes.

#include "fusion/portableglobe/portableglobebuilder.h"
#include <notify.h>
#include <algorithm>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/etencoder.h"
#include "common/khConstants.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/packetcompress.h"
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "mttypes/DrainableQueue.h"
#include "mttypes/WaitBaseManager.h"


namespace fusion_portableglobe {

/**
 * Constructor. If this is the last node in the path, mark it as
 * a leaf. Otherwise, recursively call the constructor to make
 * the next node in the path.
 */
HiresNode::HiresNode(std::string path) {
  if (path.empty()) {
    is_leaf = true;
  } else {
    is_leaf = false;
    AddChild(path[0], new HiresNode(path.substr(1)));
  }
}

/**
 * Returns pointer to child node for the given char
 * or returns NULL if there is no such child. The
 * node_num char shoud be '0', '1', '2' or '3'.
 */
HiresNode* const HiresNode::GetChild(char node_num) const {
  size_t idx = GetChildIndex(node_num);
  if (idx < children.size())
    return children[idx];

  return NULL;
}

/**
 * Adds given node as a child in the position specified by
 * node_num. The node_num char shoud be '0', '1', '2' or '3'.
 */
void HiresNode::AddChild(char node_num, HiresNode* child) {
  if (children.size() < 4) {
    children.resize(4);
  }

  children[GetChildIndex(node_num)] = child;
}

/**
 * Debugging routine that recursively shows which nodes are
 * in a tree.
 */
void HiresNode::Show(std::string indent) const {
  if (children.size() == 0) {
    std::cout << indent << "L" << std::endl;
    return;
  }

  for (int i = 0; i < 4; ++i) {
    if (children[i] != NULL) {
      std::cout << indent << i << std::endl;
      children[i]->Show(indent + " ");
    }
  }
}


/**
 * Recursively checks if path is in the tree starting with
 * this node as the root.
 * TODO: Consider eliminating this method and just
 * TODO: calculating this iteratively in the tree method.
 *
 * @return whether this path is in the tree.
 */
bool HiresNode::IsTreePath(const std::string& path) const {
  // If path is empty, we have a match.
  if (path.empty()) {
    return true;
  }

  // If we are at a leaf, everything below is a match.
  if (is_leaf) {
    return true;
  }

  // See if we can match child for next node in path.
  const HiresNode* child = GetChild(path[0]);
  if (child != NULL) {
    return child->IsTreePath(path.substr(1));
  }

  // If not, the path is not in the tree.
  return false;
}


size_t HiresNode::GetChildIndex(char node_num) const {
  return (size_t) node_num - ZERO_CHAR;
}


/**
 * Adds node for the specified path. If a ancestor path is already in the
 * tree, the node is ignored since all descendents are already assumed.
 * For the polygon to qt node algorithm, this would be unexpected because
 * it tries not to pass any redundant information.
 */
void HiresTree::AddHiResQtNode(HiresNode* node, const std::string& path) {
  // Go until we find nodes that haven't been created yet.
  for (size_t i = 0; i < path.size(); ++i) {
    if (node->IsLeaf()) {
      notify(NFY_WARN, "Lower-level parent node already in place.");
      return;
    }

    HiresNode* child_node = node->GetChild(path[i]);
    if (child_node == NULL) {
      // Add remaining needed nodes to the tree.
      node->AddChild(path[i], new HiresNode(path.substr(i + 1)));
      return;
    }

    // Keep descending through the existing tree nodes.
    node = child_node;
  }

  node->MakeLeaf();
  notify(NFY_WARN, "%s node already in place, may be losing specificity.",
         path.c_str());
}

void HiresTree::LoadHiResQtNodeFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  LoadHiResQtNodes(&fin);
}

/**
 * Load quadtree node paths into tree structure so we can quick check if a
 * quadtree node is a leaf in the tree or is a child of a leaf in the tree.
 * We don't care about the ordering of the leaves in this stream, the
 * same tree should be built independent of the order if all are leaves.
 * TODO: Use QuadTreePath instead of std::string.
 */
void HiresTree::LoadHiResQtNodes(std::istream* fin) {
  std::string next_node_path;

  while (!fin->eof()) {
    next_node_path = "";
    *fin >> next_node_path;
    if (!next_node_path.empty()) {
      AddHiResQtNode(hires_tree_, next_node_path);
    }
  }
}

bool HiresTree::IsTreePath(const std::string& path) {
  HiresNode* node = hires_tree_;
  std::string remaining_path = path;
  while (true) {
    // If path is empty, we have a match.
    if (remaining_path.empty()) {
      return true;
    }

    // If we are at a leaf, everything below is a match.
    if (node->IsLeaf()) {
      return true;
    }

    // See if we can match child for next node in path.
    node = node->GetChild(remaining_path[0]);
    if (node == NULL) {
      return false;
    }

    // So far so good: keep going.
    remaining_path = remaining_path.substr(1);
  }
}


// Names of subdirectories within the portable globe.
const std::uint16_t PortableGlobeBuilder::kMaxPacketDepth = 7;  // 4 levels / packet
const std::uint16_t PortableGlobeBuilder::kMaxPacketLevel = 4;  // 4 levels / packet
const std::string PortableGlobeBuilder::kDataDirectory = "/data";
const std::string PortableGlobeBuilder::kImageryDirectory = "/img";
const std::string PortableGlobeBuilder::kTerrainDirectory = "/ter";
const std::string PortableGlobeBuilder::kVectorDirectory = "/vec";
const std::string PortableGlobeBuilder::kQtPacketDirectory = "/qtp";
const std::string PortableGlobeBuilder::kQtPacket2Directory = "/qtp2";
const std::string PortableGlobeBuilder::kIconsDirectory = "/icons";
const std::string PortableGlobeBuilder::kPluginDirectory = "/earth";
const std::string PortableGlobeBuilder::kSearchDirectory = "/search_db";
// String that indicates to Earth Builder cutter that multiple packets
// are being requested.
const std::string PortableGlobeBuilder::kMultiPacket = "&m=1";
// This limit comes from HUGE_STRING_LEN of 8192 defined in httpd.h
const size_t      PortableGlobeBuilder::kBundleSizeForSizeQuery = 8000;
// This limit comes from 256 * 24 < 8192, and the size of return stream,
// (concatenation of lot of QT packets) should be reasonable.
const size_t      PortableGlobeBuilder::kBundleSizeForPacketQuery = 256;

/**
 * Constructor.
 */
PortableGlobeBuilder::PortableGlobeBuilder(
    const std::string& source,
    std::uint16_t default_level,
    std::uint16_t max_level,
    const std::string& hires_qt_nodes_file,
    const std::string& dbroot_file,
    const std::string& globe_directory,
    const std::string& additional_args,
    const std::string& qtpacket_version,
    const std::string& metadata_file,
    bool no_write,
    bool use_post)
    : globe_directory_(globe_directory),
    default_level_(default_level),
    max_level_(max_level),
    source_(source),
    additional_args_(additional_args),
    qtpacket_version_(qtpacket_version),
    is_no_write_(no_write),
    qt_packet_look_ahead_cache_(kBundleSizeForPacketQuery, this,
                                &PortableGlobeBuilder::DownloadQtPacketBunch),
    image_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                              &PortableGlobeBuilder::GetImagePacketsSize),
    terrain_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                                &PortableGlobeBuilder::GetTerrainPacketsSize),
    vector_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                               &PortableGlobeBuilder::GetVectorPacketsSize),
      metadata_file_(metadata_file) {
  // Set up the connection to the server.
  server_ = new gstSimpleEarthStream(source_, additional_args_, use_post);

  // Get path to dbroot if it has already been constructed.
  dbroot_file_ = dbroot_file;

  // Build a structure for filtering quadtree addresses.
  hires_tree_ = new HiresTree();
  if (!hires_qt_nodes_file.empty()) {
    hires_tree_->LoadHiResQtNodeFile(hires_qt_nodes_file);
  }

  qtpacket_.resize(kMaxPacketDepth);
  qtnode_index_.resize(kMaxPacketDepth);

  for (std::uint16_t i = 0; i < kMaxPacketDepth; ++i) {
    qtpacket_[i] = new qtpacket::KhQuadTreePacket16();
  }
}

/**
 * Test constructor. Doesn't connect to an actual source.
 */
PortableGlobeBuilder::PortableGlobeBuilder(
    std::uint16_t default_level,
    std::uint16_t max_level,
    const std::string& hires_qt_nodes_file)
    : default_level_(default_level),
    max_level_(max_level),
    qt_packet_look_ahead_cache_(kBundleSizeForPacketQuery, this,
                                &PortableGlobeBuilder::DownloadQtPacketBunch),
    image_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                              &PortableGlobeBuilder::GetImagePacketsSize),
    terrain_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                                &PortableGlobeBuilder::GetTerrainPacketsSize),
    vector_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                               &PortableGlobeBuilder::GetVectorPacketsSize) {
  // Build a structure for filtering quadtree addresses.
  hires_tree_ = new HiresTree();
  if (!hires_qt_nodes_file.empty()) {
    hires_tree_->LoadHiResQtNodeFile(hires_qt_nodes_file);
  }

  server_ = NULL;

  qtpacket_.resize(kMaxPacketDepth);
  qtnode_index_.resize(kMaxPacketDepth);
  for (std::uint16_t i = 0; i < kMaxPacketDepth; ++i) {
    qtpacket_[i] = NULL;
  }
}

/**
 * Destructor.
 */
PortableGlobeBuilder::~PortableGlobeBuilder() {
  if (server_ != NULL) {
    delete server_;
  }

  if (hires_tree_ != NULL) {
    delete hires_tree_;
  }

  for (std::uint16_t i = 0; i < kMaxPacketDepth; ++i) {
    if (qtpacket_[i] != NULL) {
      delete qtpacket_[i];
    }
  }
}


/**
 * Builds a globe that is a subset of the specified source (i.e. the master
 * globe) as a set of packet bundles. The globe is built in two passes
 * because depth-first search is different for assets then it is
 * for quadtree packets: for assets it follows the quadtree node
 * order, but for quadtree packets, it cannot since a subtree exists
 * within the packet. Instead for quadtree packets, it follows quadtree
 * node order for every fourth level of nodes (except for the first
 * packet level which only has 3 levels of nodes).
 */
void PortableGlobeBuilder::BuildGlobe() {
  num_image_packets = 0;
  num_terrain_packets = 0;
  num_vector_packets = 0;
  image_size = 0;
  terrain_size = 0;
  vector_size = 0;
  total_size = 0;

  // Process the quadtree packets.
  if (!is_no_write_) {
    AddWriter(kQtPacketDirectory);
  }

  // First save the dbroot.
  WriteDbRootPacket(kDbRootPacket);

  // First pass: identify all nodes whose data will be used.
  // Start at the root and recursively traverse the quadtree,
  // saving all nodes to be kept as we go.
  if (DownloadQtPacket("", 0)) {
    PruneQtPacket("", 0);
  }
  if (is_no_write_) {
    image_pac_size_req_cache_.FlushCache();
    terrain_pac_size_req_cache_.FlushCache();
    vector_pac_size_req_cache_.FlushCache();
    return;
  }
  // Finish up writing all quadtree packets so we can use
  // them in the next pass to process the data.
  DeleteWriter();

  // Sort the qt bearing paths
  qt_packet_look_ahead_cache_.Sort();

  // Add writers for the different packet types.
  AddWriter(kDataDirectory);

  // Second pass: save data from all nodes still in the quadtree.
  // Start at the root and recursively process the quadtree.
  if (DownloadQtPacket("", 0)) {
    RequestBundlerForPacket write_cache(
        PortableGlobeBuilder::kBundleSizeForPacketQuery,
        this, writer_, true, 16);
    write_cache_ = &write_cache;
    qtnode_index_[0] = 0;
    // Level 0 quadtree packet is only 3 levels deep. So start from level 1.
    ProcessDataInQtPacketNodes("", 1, 0);
    write_cache_ = NULL;
    // write_cache goes out of scope and the destructor flushes the cache.
  }

  if (!metadata_file_.empty()) {
    bounds_tracker_.write_json_file(metadata_file_);
  }

  // Finish up writing all of the packet bundles.
  DeleteWriter();
}



template< typename T >
QtPacketLookAheadRequestor<T>::QtPacketLookAheadRequestor(
    size_t bunch_size, T* builder, DownloadQtPacketBunchType func) :
    bunch_size_(bunch_size), caller_(builder), download_qt_packet_bunch_(func),
    cached_packets_(TransferOwnership(new LittleEndianReadBuffer[bunch_size_])),
    cache_index_(0), prefetched_(0) {
  cached_paths_.reserve(bunch_size_);
}

template< typename T >
bool QtPacketLookAheadRequestor<T>::DownloadQtPacket(
    const std::string& qtpath_str, qtpacket::KhQuadTreePacket16* packet) {
  if (cache_index_ == cached_paths_.size()) {
    if (cached_paths_.size()) {
      cached_paths_.clear();
    }
    const size_t limit_prefetched = std::min(paths_with_qt_packets_.size(),
                                             prefetched_ + bunch_size_);
    for (cache_index_ = 0; prefetched_ < limit_prefetched;
         ++prefetched_, ++cache_index_) {
      cached_paths_.push_back(paths_with_qt_packets_[prefetched_].AsString());
      cached_packets_[cache_index_].SetValue("");
    }
    cache_index_ = 0;
    (caller_->*download_qt_packet_bunch_)(cached_paths_, &cached_packets_[0]);
  }
  bool ret_val = false;
  if (qtpath_str == cached_paths_[cache_index_]) {
    LittleEndianReadBuffer& buffer = cached_packets_[cache_index_];
    if (buffer.empty()) {
      notify(NFY_WARN, "Unable to download packet: %s", qtpath_str.c_str());
    } else {
      buffer >> *packet;
      ret_val = true;
    }
    ++cache_index_;
  } else {
    fprintf(stderr, "expect: %s\n", qtpath_str.c_str());
    fprintf(stderr, "Got   : %s\n", cached_paths_[cache_index_].c_str());
    fflush(stderr);
    assert(0);  // How come we have prefetched pre-order successor of qtpath,
                // but not qtpath!
  }
  return ret_val;
}


void PortableGlobeBuilder::DownloadQtPacketBunch(
    const std::vector<std::string>& qt_paths,
    LittleEndianReadBuffer* buffer_array) {
  std::vector<std::string> paths;
  for (size_t i = 0; i < qt_paths.size(); ++i) {
    if (KeepNode(qt_paths[i])) {
      paths.push_back(qt_paths[i]);
    }
  }
  if (paths.size() != 0) {
    server_->GetQuadTreePacketBunch(
       paths, buffer_array,
       "&version=" + qtpacket_version_ +  kMultiPacket + additional_args_);
  }
}

/**
 * Download quadtree packet from the server.
 */
bool PortableGlobeBuilder::DownloadQtPacket(const std::string& packet_qtpath,
                                            std::uint16_t packet_depth) {
  std::string compressed_data;
  compressed_data.reserve(1024 * 256);
  std::string data;
  if (KeepNode(packet_qtpath)) {
    QuadtreePath  qtpath(packet_qtpath);
    if (server_->GetQuadTreePacket(
        qtpath, &compressed_data,
        "&version=" + qtpacket_version_ + additional_args_)) {
      LittleEndianReadBuffer buffer;
      if (KhPktDecompress(compressed_data.data(),
                          compressed_data.size(),
                          &buffer)) {
        buffer >> *qtpacket_[packet_depth];
        return true;
      }
    }
  }

  notify(NFY_WARN, "Unable to download packet: %s", packet_qtpath.c_str());
  return false;
}


/**
 * Prune the last downloaded quadtree packet at the given level. Filter any
 * nodes from the packet that weren't specified as keepers, and then save
 * it. Then process all of the quadtree packet's children by calling this
 * routine recursively.
 */
void PortableGlobeBuilder::PruneQtPacket(const std::string& packet_qtpath,
                                         const std::uint16_t packet_depth) {
  qtnode_index_[packet_depth] = 0;
  std::vector<std::string> child_packets;

  // Update all of nodes in the quadtree packet depending on whether they
  // were specified to be filtered out.

  const std::uint64_t last_total_size = total_size;

  // Level 0 quadtree packet is only 3 levels deep.
  if (packet_qtpath == "") {
    PruneQtPacketNodes(packet_qtpath, 1, packet_depth, &child_packets);

  // All others are 4 levels deep.
  } else {
    PruneQtPacketNodes(packet_qtpath, 0, packet_depth, &child_packets);
  }

  // Save the modified quadtree packet to a packet bundle.
  if (is_no_write_) {
    // Add the estimated size of compressed QT packet only if QT packet had
    // something to add to size. Else ignore it.
    if (last_total_size != total_size) {
      WriteQtPacket(packet_qtpath, packet_depth, kQtpPacket);
    }
  } else {
    WriteQtPacket(packet_qtpath, packet_depth, kQtpPacket);
  }

  if (child_packets.size() == 0) {
    return;
  }
  // Prune all of the child quadtree packets in order.
  khDeleteGuard<LittleEndianReadBuffer, ArrayDeleter> buffers(
      TransferOwnership(new LittleEndianReadBuffer[child_packets.size()]));
  DownloadQtPacketBunch(child_packets, &buffers[0]);
  for (size_t i = 0; i < child_packets.size(); ++i) {
    if (!buffers[i].empty()) {
      buffers[i] >> *qtpacket_[packet_depth+1];
      PruneQtPacket(child_packets[i], packet_depth+1);
    }
  }
}

/**
 * Returns whether the given quadtree node should be kept (i.e. not pruned).
 * If the node is at or below (lower resolution) the default level or if
 * it is in the specified high-resolution nodes, then it should not be
 * pruned.
 */
bool PortableGlobeBuilder::KeepNode(const std::string& qtpath) const {
  std::uint16_t level = qtpath.size();

  // If we are at or below the default level, keep everything.
  if (level <= default_level_) {
    return true;
  }

  // If we are above the max level, keep nothing.
  if (level > max_level_) {
    return false;
  }

  // Check the quadtree path to see if it is one we are keeping.
  return hires_tree_->IsTreePath(qtpath);
}


namespace {
// Assumes level < 4
// Counts number of children under current node upto level 4
 std::uint32_t CountChildren(qtpacket::KhQuadTreePacket16* packet,
                     std::uint32_t index,
                     std::uint16_t level) {
  const std::uint32_t orig_index = index;
  std::uint16_t to_do[13];  // worst case to do, we are at level4 (4+3+3+3)
  int to_do_index = 0;
  to_do[to_do_index] = level;
  ++to_do_index;
  while (to_do_index) {
    level = to_do[--to_do_index];
    qtpacket::KhQuadTreeQuantum16* node = packet->GetPtr(index);
    ++index;  // next pre-order node is at this index
    if (++level > PortableGlobeBuilder::kMaxPacketLevel) {
      continue;
    }
    for (int i = 0; i < 4; ++i) {
      if (node->children.GetBit(i)) {
        to_do[to_do_index] = level;
        ++to_do_index;
      }
    }
  }
  return index - 1 - orig_index;
}
}


/**
 * Recursively walk all of the nodes in the packet. Assume packet has
 * already been pruned. Get all specified data in the nodes and put them
 * into the appropriate packet bundles. This method is separate from
 * the initial pruning to ensure that packet data is stored in the
 * correct order (depth first or preorder).
 * The packets are stored in bundles of nodes.
 * e.g depth 0 ->         level1, level2, level3, level4 -> 85 nodes (1+4+16+64)
 * e.g depth 1 -> level0, level1, level2, level3, level4 -> 341 nodes (1+4+16+64+256)
 * e.g depth 2 -> level0, level1, level2, level3, level4 -> 341 nodes
 * ...
 */
void PortableGlobeBuilder::ProcessDataInQtPacketNodes(
    const std::string& qtpath, std::uint16_t packet_level, std::uint16_t packet_depth) {
  if (qtnode_index_[packet_depth]
      >= static_cast<std::uint16_t>(
      qtpacket_[packet_depth]->packet_header().num_instances)) {
    notify(NFY_WARN, "Index extended beyond number of instances.\n");
    return;
  }

  qtpacket::KhQuadTreeQuantum16* node =
      qtpacket_[packet_depth]->GetPtr(qtnode_index_[packet_depth]);

  // If we are filtering a pre-existing node, clear all data.
  // If packet_level == 0 there is no data, it is only for children.
  // For this qt_path the data was there at packet_level == 4 rather.
  if (node->children.GetImageBit()) {
    bounds_tracker_.update(kGEImageryChannelId, kImagePacket, qtpath);
    WriteImagePacket(qtpath, node->image_version);
  }
  if (node->children.GetTerrainBit()) {
    bounds_tracker_.update(kGETerrainChannelId - 1, kTerrainPacket, qtpath);
    WriteTerrainPacket(qtpath, node->terrain_version);
  }
  if (node->children.GetDrawableBit()) {
    for (size_t i = 0; i < node->num_channels(); ++i) {
      // Assuming that channel_type is in increasing order,
      // otherwise we are going to need to sort.
      bounds_tracker_.update(node->channel_type[i], kVectorPacket, qtpath);
      WriteVectorPacket(qtpath,
                        node->channel_type[i],
                        node->channel_version[i]);
    }
  }
  if (qtpath.size() == std::max(default_level_, max_level_)) {
    // Skip nodes in this Quadtree Packet for children
    if (packet_level < kMaxPacketLevel) {
      qtnode_index_[packet_depth] += CountChildren(
          qtpacket_[packet_depth], qtnode_index_[packet_depth], packet_level);
    }
    return;
  }

  // If packet_level has reached 4 descend to next depth; if cannot then return.
  if (packet_level == kMaxPacketLevel) {
    // If we are at a leaf node, we are done here.
    if (!node->children.GetCacheNodeBit()) {
      return;
    }

    ++packet_depth;
    if (!qt_packet_look_ahead_cache_.DownloadQtPacket(
        qtpath, qtpacket_[packet_depth])) {
      return;  // if cannot get quadtree packet then return
    }
    // go for next depth
    qtnode_index_[packet_depth] = 0;
    node = qtpacket_[packet_depth]->GetPtr(0);
    packet_level = 0;
  }
  const QuadtreePath this_qtpath(qtpath);
  ++packet_level;
  for (int i = 0; i < 4; ++i) {
    if (node->children.GetBit(i)) {
      const std::string child_qtpath = this_qtpath.Child(i).AsString();
      qtnode_index_[packet_depth] += 1;
      if (KeepNode(child_qtpath)) {
        ProcessDataInQtPacketNodes(child_qtpath, packet_level, packet_depth);
      } else if (packet_level < kMaxPacketLevel) {
        // Skip nodes in this Quadtree Packet for children
        qtnode_index_[packet_depth] += CountChildren(
            qtpacket_[packet_depth], qtnode_index_[packet_depth], packet_level);
      }
    }
  }
}


/**
 * Recursively walk all of the nodes in the packet. Clear nodes
 * that are to be pruned. If we are at the bottom level in the packet
 * and a node not being pruned has children, add the children to
 * the queue of quadtree paths to be processed.
 */
void PortableGlobeBuilder::PruneQtPacketNodes(
    const std::string& qtpath,
    std::uint16_t packet_level,
    std::uint16_t packet_depth,
    std::vector<std::string>* child_packets) {
  if (qtnode_index_[packet_depth] >= static_cast<std::uint16_t>(
      qtpacket_[packet_depth]->packet_header().num_instances)) {
    notify(NFY_WARN, "Index extended beyond number of instances.\n");
    return;
  }

  qtpacket::KhQuadTreeQuantum16* node =
      qtpacket_[packet_depth]->GetPtr(qtnode_index_[packet_depth]);

  // If we are filtering a pre-existing node, clear all data.
  const QuadtreePath this_qtpath(qtpath);
  const bool keep_node = KeepNode(qtpath);
  // If CacheNodeBit is set, we are not in a leaf node.
  if (keep_node && (packet_level == kMaxPacketLevel) &&
      node->children.GetCacheNodeBit() &&
      (qtpath.size() < max_level_ || qtpath.size() < default_level_)) {
    child_packets->push_back(qtpath);
    if (!is_no_write_) {
      qt_packet_look_ahead_cache_.Append(qtpath);
    }
  }

  // Descend to children, using children bits in the packet
  if (packet_level < kMaxPacketLevel) {
    for (int i = 0; i < 4; ++i) {
      if (node->children.GetBit(i)) {
        const QuadtreePath child_qtpath(this_qtpath.Child(i));
        qtnode_index_[packet_depth] += 1;
        PruneQtPacketNodes(child_qtpath.AsString(),
                           packet_level + 1,
                           packet_depth,
                           child_packets);
      }
    }
  }

  // We do not clear until the end, so we can descend through children
  // that might need to be cleared.
  if (!keep_node) {
    // Only clear data (not children) for now, so we walk all packets.
    // Otherwise, index will be wrong.
    // TODO: Compact, and get rid of unused nodes.
    node->children.ClearImageBit();
    node->children.ClearTerrainBit();
    node->children.ClearDrawableBit();
    node->children.ClearCacheNodeBit();
  } else if (is_no_write_) {
    if (node->children.GetImageBit()) {
      image_pac_size_req_cache_.GetPacketsSize(
              qtpath, node->image_version, 0);  // a dummy channel
    }
    if (node->children.GetTerrainBit()) {
      terrain_pac_size_req_cache_.GetPacketsSize(
          qtpath, node->terrain_version, 0);  // a dummy channel
    }
    if (node->children.GetDrawableBit()) {
      for (size_t i = 0; i < node->num_channels(); ++i) {
        // Assuming that channel_type is in increasing order,
        // otherwise we are going to need to sort.
        vector_pac_size_req_cache_.GetPacketsSize(
            qtpath, node->channel_version[i], node->channel_type[i]);
      }
    }
  }
}


/**
 * Create directory if needed and create packet writer.
 */
void PortableGlobeBuilder::AddWriter(const std::string& sub_directory) {
  std::string directory = globe_directory_ + sub_directory;
  khEnsureParentDir(directory + "/index");
  writer_ = new PacketBundleWriter(directory);
}

/**
 * Delete packet writer.
 */
void PortableGlobeBuilder::DeleteWriter() {
  delete writer_;
}

/**
 * Get size of packet normally returned by url. Url should already encode
 * the size request (&size=1).
 */
 std::uint64_t PortableGlobeBuilder::GetPacketSize(const std::string& url) {
  std::string raw_packet;

  server_->GetRawPacket(url, &raw_packet, false);
  if (!raw_packet.empty()) {
    std::istringstream buffer(raw_packet);
    std::uint64_t size;
    buffer >> size;
    return size;
  } else {
    std::cout << "No data for " << url << std::endl;
    return 0;
  }
}


template< typename T >
RequestBundlerForPacketSize<T>::RequestBundlerForPacketSize(
    int bunch_size, T* builder, FunctionType callback) :
    bunch_size_(bunch_size), caller_(builder), callback_(callback) {
}


template< typename T >
void RequestBundlerForPacketSize<T>::GetPacketsSize(
    const std::string& qt_path, std::uint32_t version, std::uint32_t channel) {
  Bundle::iterator i = bundle_.find(std::make_pair(channel, version));
  if (i == bundle_.end()) {
    i = bundle_.insert(std::make_pair(std::make_pair(channel, version),
                                      QtPathBundle())).first;
    i->second.first.reserve(bunch_size_);
    i->second.second = 0;  // number of paths
  }
  QtPathBundle& paths = i->second;
  size_t const size_after_append = paths.first.size() + qt_path.size() + 1;
  if (size_after_append >= bunch_size_) {
    (caller_->*callback_)(paths.first, paths.second, version, channel);
    paths.first.resize(0);
    paths.second = 0;
  }
  // Concatenate a vector of strings (quad-tree paths) by a separator '4'.
  // 4 is chosen since 4 doesn't occur in a quadtree path (only 0,1,2,3 occur).
  // Please note that the string ends with '4'.
  paths.first.append(qt_path);
  paths.first.push_back(kQuadTreePathSeparator);
  ++paths.second;
}


template< typename T >
void RequestBundlerForPacketSize<T>::FlushCache() {
  for (Bundle::iterator i = bundle_.begin(); i != bundle_.end(); ++i) {
    std::uint32_t channel = i->first.first;
    std::uint32_t version = i->first.second;
    QtPathBundle& paths = i->second;
    if (paths.second != 0) {
      (caller_->*callback_)(paths.first, paths.second, version, channel);
      paths.first.resize(0);
      paths.second = 0;
    }
  }
}


// Ignores 3rd param channel
// TODO: Switch to POST if we start using.
void PortableGlobeBuilder::GetImagePacketsSize(
    const std::string& paths, size_t num_paths, std::uint32_t version, std::uint32_t) {
  num_image_packets += num_paths;
  std::string url = source_ + "/query?request=ImageryGE&blist=" + paths +
                    "&channel=0&version=" + NumberToString(version) + "&size=1"
                    + additional_args_;
  std::uint64_t size = GetPacketSize(url);
  image_size += size;
  total_size += size;
}


void PortableGlobeBuilder::GetImagePacket(
    const std::string& qtpath, std::uint32_t version, std::string* raw_packet) {
  std::string url = source_ + "/query";
  std::string args = "request=ImageryGE&blist=" + qtpath +
                     "&channel=0&version=" + NumberToString(version) +
                     kMultiPacket + additional_args_;
  server_->PostRawPacket(url, args, raw_packet, false);
}


/**
 * Get image packet for the given quadtree path and add it to the
 * image packet bundle.
 */
void PortableGlobeBuilder::WriteImagePacket(const std::string& qtpath,
                                            std::uint32_t version) {
  num_image_packets += 1;
  write_cache_->WriteImagePacket(qtpath, version);
}

// Ignores 3rd param channel
// TODO: Switch to POST if we start using.
void PortableGlobeBuilder::GetTerrainPacketsSize(
    const std::string& paths, size_t num_paths, std::uint32_t version, std::uint32_t) {
  num_terrain_packets += num_paths;
  std::string url = source_ + "/query?request=Terrain&blist=" + paths +
                    "&channel=2&version=" + NumberToString(version) + "&size=1"
                    + additional_args_;
  std::uint64_t size = GetPacketSize(url);
  terrain_size += size;
  total_size += size;
}


void PortableGlobeBuilder::GetTerrainPacket(
    const std::string& qtpath, std::uint32_t version, std::string* raw_packet) {
  std::string url = source_ + "/query";
  std::string args = "request=Terrain&blist=" + qtpath +
                     "&channel=2&version=" + NumberToString(version) +
                     kMultiPacket + additional_args_;
  server_->PostRawPacket(url, args, raw_packet, false);
}


/**
 * Get terrain packet for the given quadtree path and add it to the
 * terrain packet bundle.
 */
void PortableGlobeBuilder::WriteTerrainPacket(const std::string& qtpath,
                                              std::uint32_t version) {
  num_terrain_packets += 1;
  write_cache_->WriteTerrainPacket(qtpath, version);
}


// TODO: Switch to POST if we start using.
void PortableGlobeBuilder::GetVectorPacketsSize(const std::string& paths,
                                                size_t num_paths,
                                                std::uint32_t version,
                                                std::uint32_t channel) {
  num_vector_packets += num_paths;
  std::string url = source_ + "/query?request=VectorGE&blist=" + paths +
                    "&channel=" + NumberToString(channel) + "&version=" +
                    NumberToString(version) + "&size=1" + additional_args_;
  std::uint64_t size = GetPacketSize(url);
  vector_size += size;
  total_size += size;
}


void PortableGlobeBuilder::GetVectorPacket(
    const std::string& qtpath, std::uint32_t channel, std::uint32_t version,
    std::string* raw_packet) {
  std::string url = source_ + "/query";
  std::string args = "request=VectorGE&blist=" + qtpath +
                     "&channel=" + NumberToString(channel) + "&version=" +
                     NumberToString(version) + kMultiPacket + additional_args_;
  server_->PostRawPacket(url, args, raw_packet, false);
}

/**
 * Get vector packet for the given quadtree path and channel add them to the
 * terrain packet bundle.
 */
void PortableGlobeBuilder::WriteVectorPacket(const std::string& qtpath,
                                             std::uint32_t channel,
                                             std::uint32_t version) {
  num_vector_packets += 1;
  write_cache_->WriteVectorPacket(qtpath, channel, version);
}

/**
 * Get dbroot packet and add it to the quadtree packet bundle. The dbroot
 * is always stored as the first packet with its associated quadtree
 * packets.
 */
void PortableGlobeBuilder::WriteDbRootPacket(PacketType packet_type) {
  std::string raw_packet;

  if (dbroot_file_.empty()) {
    std::string url =
        source_ + "/" + kHttpDbRootRequest + "?" + additional_args_;
    server_->GetRawPacket(url, &raw_packet, false);
  } else {
    std::ifstream fin(dbroot_file_.c_str(), std::ios::in|std::ios::binary);

    // Get length of file.
    fin.seekg(0, std::ios::end);
    int length = fin.tellg();
    fin.seekg(0, std::ios::beg);

    // Allocate memory.
    if (length > 0) {
      raw_packet.resize(length);

      // Read in the dbroot.
      char* cptr = const_cast<char*>(raw_packet.c_str());
      fin.read(cptr, length);
    }

    fin.close();
  }

  // Store dbroot packet at the front of the quadtree bundle.
  if (is_no_write_) {
    total_size += raw_packet.size();
  } else {
    writer_->AppendPacket("0", kDbRootPacket, 0, raw_packet);
  }
}

/**
 * Write the updated quadtree packet to the packet bundle.
 */
void PortableGlobeBuilder::WriteQtPacket(const std::string& qtpath,
                                         std::uint16_t packet_depth,
                                         PacketType packet_type) {
  LittleEndianWriteBuffer write_buffer;
  write_buffer.reset();
  write_buffer << *qtpacket_[packet_depth];
  std::string full_qtpath = "0" +  qtpath;

  // Recompress packet
  std::string recompressed_buffer;
  size_t est_recompressed_size
      = KhPktGetCompressBufferSize(write_buffer.size());
  if (is_no_write_) {
    total_size += est_recompressed_size;
    return;
  }
  recompressed_buffer.resize(est_recompressed_size);
  size_t recompressed_size = est_recompressed_size;
  if (KhPktCompressWithBuffer(write_buffer.data(),
                              write_buffer.size(),
                              &recompressed_buffer[0],
                              &recompressed_size)) {
    // Add recompressed quadtree packet to the quadtree bundle.
    std::string raw_packet = recompressed_buffer.substr(0, recompressed_size);
    // Should I be checking the 8 byte boundary here?
    char* cptr = const_cast<char*>(raw_packet.c_str());
    etEncoder::EncodeWithDefaultKey(cptr, raw_packet.size());
    writer_->AppendPacket(full_qtpath, packet_type, 0, raw_packet);
  } else {
    notify(NFY_WARN, "Unable to recompress quadtree node.");
  }
}

}  // namespace fusion_portableglobe
