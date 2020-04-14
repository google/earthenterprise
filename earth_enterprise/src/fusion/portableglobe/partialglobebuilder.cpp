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


// Methods for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes.

#include "fusion/portableglobe/partialglobebuilder.h"
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
#include "common/notify.h"
#include "common/packetcompress.h"
#include "common/qtpacket/quadtreepacket.h"
#include "common/qtpacket/quadtreepacketitem.h"
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "mttypes/DrainableQueue.h"
#include "mttypes/WaitBaseManager.h"


namespace fusion_portableglobe {

// Names of subdirectories within the portable globe.
const std::uint16_t PortableGlobeBuilder::kMaxPacketDepth = 7;  // 4 levels / packet
const std::uint16_t PortableGlobeBuilder::kMaxPacketLevel = 4;  // 4 levels / packet
const std::string PortableGlobeBuilder::kDataDirectory = "/data";
const std::string PortableGlobeBuilder::kImageryDirectory = "/img";
const std::string PortableGlobeBuilder::kTerrainDirectory = "/ter";
const std::string PortableGlobeBuilder::kVectorDirectory = "/vec";
const std::string PortableGlobeBuilder::kMercatorQtPacketDirectory = "/mapqtp";
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
// Estimate: 100,000 / GB.
const std::uint32_t  PortableGlobeBuilder::kImageryPacketsPerMark = 200000;

/**
 * Constructor.
 */
PortableGlobeBuilder::PortableGlobeBuilder(
    const std::string& source,
    std::uint16_t default_level,
    std::uint16_t min_level,
    std::uint16_t max_level,
    const std::string& hires_qt_nodes_file,
    const std::string& dbroot_file,
    const std::string& globe_directory,
    const std::string& additional_args,
    const std::string& qtpacket_version,
    std::uint16_t min_imagery_version,
    bool no_write,
    bool use_post,
    const std::string& data_type,
    std::uint32_t imagery_packets_per_mark)
    : globe_directory_(globe_directory),
    default_level_(default_level),
    min_level_(min_level),
    max_level_(max_level),
    source_(source),
    additional_args_(additional_args),
    qtpacket_version_(qtpacket_version),
    min_imagery_version_(min_imagery_version),
    cut_imagery_packets_(true),
    cut_terrain_packets_(true),
    cut_vector_packets_(true),
    is_no_write_(no_write),
    imagery_packets_per_mark_(imagery_packets_per_mark),
    qt_packet_look_ahead_cache_(kBundleSizeForPacketQuery, this,
                                &PortableGlobeBuilder::DownloadQtPacketBunch),
    image_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                              &PortableGlobeBuilder::GetImagePacketsSize),
    terrain_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                                &PortableGlobeBuilder::GetTerrainPacketsSize),
    vector_pac_size_req_cache_(kBundleSizeForSizeQuery, this,
                               &PortableGlobeBuilder::GetVectorPacketsSize) {
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

  if (data_type != kCutAllDataFlag) {
    if (data_type != kCutImageryDataFlag) {
      cut_imagery_packets_ = false;
    }
    if (data_type != kCutTerrainDataFlag) {
      cut_terrain_packets_ = false;
    }
    if (data_type != kCutVectorDataFlag) {
      cut_vector_packets_ = false;
    }
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
 * Builds a pruned quadtree that only includes information for data packets
 * that are within the specified cut.
 * Generates:
 *   /qtp/index
 *   /qtp/pbundle_0000
 *   /qtp/pbundle_000.crc
 */
void PortableGlobeBuilder::BuildQuadtree() {
  num_image_packets = 0;
  num_terrain_packets = 0;
  num_vector_packets = 0;
  max_image_version = 0;
  max_terrain_version = 0;
  max_vector_version.clear();
  image_size = 0;
  terrain_size = 0;
  vector_size = 0;
  total_size = 0;

  // Process the quadtree packets.
  if (!is_no_write_) {
    // TODO: Consider ways to only keep modified portions of
    // TODO: quadtree.
    bool is_delta = false;
    // Last two arguments are moot when is delta is false.
    std::cout << "kQtPacketDirectory: " << kQtPacketDirectory << std::endl;
    AddWriter(kQtPacketDirectory, 0, is_delta, true, "");
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
}


/**
 * Builds a Mercator quadtree based on the flat quadtree.
 *
 * The basic approach is to walk the flat quadtree, and wherever there
 * are data, add corresponding Mercator data nodes to an in-memory tree
 * structure. Then that tree structure is used to generate a Mercator
 * quadtree. It is necessary to do this in two passes because the
 * added Mercator nodes may be (way) out of quadtree order.
 *
 * Since we are looking at the Mercator nodes that the flat
 * nodes *might* overlap, we are necessarily conservative and the
 * Mercator quadtree may mark some nodes as having data when they
 * in fact don't have any. The subsequent tools that use this
 * quadtree should be prepared to handle this situation.
 */
void PortableGlobeBuilder::BuildMercatorQuadtree() {
  num_image_packets = 0;
  num_terrain_packets = 0;
  num_vector_packets = 0;
  image_size = 0;
  terrain_size = 0;
  vector_size = 0;
  total_size = 0;

  quadtree_ = new HiresTree();
  mercator_quadtree_ = new HiresTree();
  std::cout << "kMercatorQtPacketDirectory: "
      << kMercatorQtPacketDirectory << std::endl;
  AddWriter(kMercatorQtPacketDirectory, 0, false, true, "");

  // First save the dbroot.
  std::cout << "WriteDbRootPacket: " << std::endl;
  WriteDbRootPacket(kDbRootPacket);

  // Create a reader for already built quadtree.
  std::cout << "Create a reader for already built quadtree: " << std::endl;
  PacketBundleReader qt_reader(globe_directory_ + kQtPacketDirectory);

  // Start at the root and recursively process the quadtree.
  std::cout << "BuildMercatorDataTree: " << std::endl;
  if (LoadQtPacket("", 0, qt_reader)) {
    std::cout << "BuildMercatorDataTree start: " << std::endl;
    BuildMercatorDataTree("", 1, 0, qt_reader);
  }

  mercator_quadtree_->Show();
  std::cout << "BuildQuadtreePackets: " << std::endl;
  BuildQuadtreePackets(mercator_quadtree_->Root(), "", 0);

  // Finish up writing all of the packet bundles.
  DeleteWriter();
}


/**
 * Builds a vector quadtree based on just the limits of the
 * default_level, max_level, and polygon. Used to limit the
 * space in which we search for vector packets.
 */
void PortableGlobeBuilder::BuildVectorQuadtree() {
  num_image_packets = 0;
  num_terrain_packets = 0;
  num_vector_packets = 0;
  image_size = 0;
  terrain_size = 0;
  vector_size = 0;
  total_size = 0;

  std::cout << "kQtPacketDirectory: "
      << kQtPacketDirectory << std::endl;
  AddWriter(kQtPacketDirectory, 0, false, true, "");

  quadtree_ = new HiresTree();
  std::cout << "BuildVectorDataTree start: " << std::endl;
  BuildVectorDataTree("");
  quadtree_->Show();

  std::cout << "BuildQuadtreePackets: " << std::endl;
  BuildQuadtreePackets(quadtree_->Root(), "", 0);

  // Finish up writing all of the packet bundles.
  DeleteWriter();
}


/**
 * Recursively walk all of the nodes in the packet. Show packets with
 * imagery data.
 */
void PortableGlobeBuilder::ShowQuadtreePacket(
    const std::string& qtpath,
    std::uint16_t packet_level,
    std::uint16_t packet_depth,
    PacketBundleReader& qt_reader) {

  qtpacket::KhQuadTreeQuantum16* node =
      qtpacket_[packet_depth]->GetPtr(qtnode_index_[packet_depth]);

  // Add a node for any data type.
  // TODO: Consider parameter control.
  if (node->children.GetImageBit()
      || node->children.GetTerrainBit()
      || node->children.GetDrawableBit()) {
    std::cout << qtpath << std::endl;
  }

  // If packet_level has reached 4 descend to next depth; if cannot then return.
  if (packet_level == kMaxPacketLevel) {
    // If we are at a leaf node, we are done here.
    if (!node->children.GetCacheNodeBit()) {
      return;
    }

    ++packet_depth;
    if (!LoadQtPacket(qtpath, packet_depth, qt_reader)) {
      std::cout << "Unable to download: " << qtpath << std::endl;
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
      ShowQuadtreePacket(child_qtpath,
                         packet_level,
                         packet_depth,
                         qt_reader);
     }
  }
}

/**
 * List nodes in the quadtree with data.
 */
void PortableGlobeBuilder::ShowQuadtree(std::string path) {
  // Create a reader for already built quadtree.
  std::string full_path = globe_directory_ + path;
  std::cout << "Reading quadtree at: " << full_path << std::endl;
  PacketBundleReader qt_reader(full_path);
  if (LoadQtPacket("", 0, qt_reader)) {
    ShowQuadtreePacket("", 1, 0, qt_reader);
  }
}


/**
 * Builds a partial globe that is a subset of the specified source (i.e. the
 * master globe) as a packet bundle labeled by the given file_index. The packet
 * bundle is filled with all of the data packets that are within cut and that
 * have quadtree addresses such that partial_start <= qt_address < partial_end.
 * Three files are generated, an index, a packet bundle, and a crc file, and
 * all three are put into a subdirectory labeled with the given file_index.
 * E.g. if the base directory is /data and file_index is 5, then it will
 * generate:
 *   /data/p0005/index
 *   /data/p0005/pbundle_0005
 *   /data/p0005/pbundle_0005.crc
 */
void PortableGlobeBuilder::BuildPartialGlobe(std::uint32_t file_index,
                                             const std::string& partial_start,
                                             const std::string& partial_end,
                                             bool create_delta,
                                             const std::string& base_glb) {
  num_image_packets = 0;
  num_terrain_packets = 0;
  num_vector_packets = 0;
  image_size = 0;
  terrain_size = 0;
  vector_size = 0;
  total_size = 0;

  // Create a reader for already built quadtree.
  PacketBundleReader qt_reader(globe_directory_ + kQtPacketDirectory);

  // Add writers for the different packet types.
  char partial_directory[256];
  snprintf(partial_directory, sizeof(partial_directory), "/p%04d", file_index);
  AddWriter(kDataDirectory + partial_directory,
            file_index,
            create_delta,
            false,
            base_glb);

  // Second pass: save data from all nodes still in the quadtree.
  // Start at the root and recursively process the quadtree.
  if (LoadQtPacket("", 0, qt_reader)) {
    RequestBundlerForPacket write_cache(
        PortableGlobeBuilder::kBundleSizeForPacketQuery,
        this, writer_, true, 16);
    write_cache_ = &write_cache;
    qtnode_index_[0] = 0;
    // Level 0 quadtree packet is only 3 levels deep. So start from level 1.
    ProcessDataInQtPacketNodes("", 1, 0, partial_start, partial_end, qt_reader);
    write_cache_ = NULL;
    // write_cache goes out of scope and the destructor flushes the cache.
  }

  // Finish up writing all of the packet bundles.
  DeleteWriter();
}


/**
 * Uses imagery_packets_per_mark_ as the number of imagery packets
 * to visit before outputting the next quadtree address. Use 0
 * for imagery_packets_per_mark_ if you want to output the
 * quadtree address of every imagery packet. Addresses are only
 * output if they are greater than or equal to the partial_start
 * and less than the partial_end.
 *
 * The logic for traversing the quadtree is the same as for
 * BuildPartialGlobe, but here focuses only on imagery packets
 * and not on the writing of packets.
 */
void PortableGlobeBuilder::OutputPartialGlobeQtAddresses(
    const std::string& file,
    const std::string& partial_start,
    const std::string& partial_end) {
  // below override is because marks are treated as the upper bound
  // non-inclusive and the lower bound inclusive. So this ensures
  // that the first block ["", qtmark0) gets at least 1 packet, assuming
  // num_packets_per_qtmark >= 1.
  num_image_packets = -1;

  // Create a reader for already built quadtree.
  PacketBundleReader qt_reader(globe_directory_ + kQtPacketDirectory);

  std::ofstream fp_out;
  fp_out.open(file.c_str(), std::ios::out);
  if (LoadQtPacket("", 0, qt_reader)) {
    qtnode_index_[0] = 0;
    // Level 0 quadtree packet is only 3 levels deep. So start from level 1.
    OutputQtAddressesInQtPacketNodes(
        &fp_out, "", 1, 0, partial_start, partial_end, qt_reader);
  }
  fp_out.close();
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

  if (KeepNode(packet_qtpath)) {
    QuadtreePath qtpath(packet_qtpath);
    // TODO: Get rid of special case call here now that
    // TODO: John made the decoder (decrypter) generally
    // TODO: available.
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
 * Load quadtree packet from already parsed quadtree. Since it is already
 * parsed, we don't need to check KeepNode.
 */
bool PortableGlobeBuilder::LoadQtPacket(const std::string& packet_qtpath,
                                        std::uint16_t packet_depth,
                                        PacketBundleReader& qt_reader) {
  std::string compressed_data;
  compressed_data.reserve(1024 * 256);

  // Packet reader requires leading "0" on path.
  std::string qtpath = "0" + packet_qtpath;
  // Channel is always 0 (meaningless) for quadtree packets.
  if (qt_reader.ReadPacket(qtpath, kQtpPacket, 0, &compressed_data)) {
    LittleEndianReadBuffer buffer;
    etEncoder::DecodeWithDefaultKey(&compressed_data[0],
                                    compressed_data.size());
    if (KhPktDecompress(compressed_data.data(),
                        compressed_data.size(),
                        &buffer)) {
      buffer >> *qtpacket_[packet_depth];
      return true;
    }

    std::cout << "Unable to decompress data." << std::endl;
  }

  notify(NFY_WARN, "Unable to read qt packet: %s", packet_qtpath.c_str());
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
    // If we're at max depth, then we already know if there is data in this
    // node and don't need to load the next quadtree packet.
    if (packet_qtpath.size() < max_level_) {
      PruneQtPacketNodes(packet_qtpath, 0, packet_depth, &child_packets);
    }
  }

  // Save the modified quadtree packet to a packet bundle.
  if (is_no_write_) {
    // Add the estimated size of compressed QT packet only if QT packet had
    // something to add to size. Else ignore it.
    if (last_total_size != total_size) {
      WriteQtPacket(packet_qtpath, packet_depth);
    }
  } else {
    WriteQtPacket(packet_qtpath, packet_depth);
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
}  // namespace


/**
 * Recursively walk all of the nodes in the packet. Assume packet has
 * already been pruned. Convert all nodes containing data to Mercator node
 * addresses and add them to a tree, which will later be converted to
 * a Mercator quadtree. We expect redundancy, since we are looking at
 * all possibly overlapping nodes, so it is ok to get warnings
 * from the tree saying that a node already exists.
 *
 * Currently, this checks for any data (imagery, terrain, or vector). In
 * the future, we might want to control this via a parameter.
 */
bool PortableGlobeBuilder::BuildMercatorDataTree(
    const std::string& qtpath,
    std::uint16_t packet_level,
    std::uint16_t packet_depth,
    PacketBundleReader& qt_reader) {

  qtpacket::KhQuadTreeQuantum16* node =
      qtpacket_[packet_depth]->GetPtr(qtnode_index_[packet_depth]);

  bool has_data = false;
  // TODO: Consider parameter control for data type.
  // TODO: Consider checking version so only get new stuff.
  if (node->children.GetImageBit()
      || node->children.GetTerrainBit()
      || node->children.GetDrawableBit()) {
    has_data = true;
  }

  // If packet_level has reached 4 descend to next depth; if cannot then return.
  if (packet_level == kMaxPacketLevel) {
    // If we are at a leaf node, we are done here.
    if (!node->children.GetCacheNodeBit()) {
      if (has_data) {
        AddMercatorNode(qtpath);
      }
      return has_data;
    }

    ++packet_depth;
    if (!LoadQtPacket(qtpath, packet_depth, qt_reader)) {
      std::cout << "Unable to download: " << qtpath << std::endl;
      return has_data;  // if cannot get quadtree packet then return
    }

    // go for next depth
    qtnode_index_[packet_depth] = 0;
    node = qtpacket_[packet_depth]->GetPtr(0);
    packet_level = 0;
  }
  const QuadtreePath this_qtpath(qtpath);
  ++packet_level;
  bool is_leaf = true;
  for (int i = 0; i < 4; ++i) {
    if (node->children.GetBit(i)) {
      const std::string child_qtpath = this_qtpath.Child(i).AsString();
      qtnode_index_[packet_depth] += 1;
      if (BuildMercatorDataTree(child_qtpath,
                                packet_level,
                                packet_depth,
                                qt_reader)) {
        is_leaf = false;
        has_data = true;
      }
    }
  }

  // Add corresponding mercator nodes for any data type.
  if (is_leaf && has_data) {
    AddMercatorNode(qtpath);
  }
  return has_data;
}


/**
 * Recursively walk all of the nodes in the defined tree and add the
 * nodes as potentially containing vector data.
 */
void PortableGlobeBuilder::BuildVectorDataTree(const std::string& qtpath) {
  if (!KeepNode(qtpath)) {
    return;
  }
  quadtree_->AddNode(qtpath);
  BuildVectorDataTree(qtpath + '0');
  BuildVectorDataTree(qtpath + '1');
  BuildVectorDataTree(qtpath + '2');
  BuildVectorDataTree(qtpath + '3');
}


/**
 * Recursively count the number of nodes that have data in a
 * Mercator quadtree packet. Used to create a packet of the correct
 * size.
 */
int PortableGlobeBuilder::CountMercatorNodes(
    const HiresNode* root, int packet_level) {

  if ((packet_level == kMaxPacketLevel) || root->IsLeaf()) {
    return 1;
  }

  packet_level++;
  char base_node = '0';
  int num_nodes = 1;
  for (int i = 0; i < 4; i++) {
    char next_node = base_node + i;
    HiresNode* new_root = root->GetChild(next_node);
    if (new_root != NULL) {
      num_nodes += CountMercatorNodes(new_root, packet_level);
    }
  }

  return num_nodes;
}

/**
 * Recursively convert tree indicating where there is data into a
 * set of corresponding quadtree packets.
 */
void PortableGlobeBuilder::BuildQuadtreePackets(
    const HiresNode* root, const std::string address, int packet_depth) {

  // Fill in data info.
  // Confusing piece is that for addresses that signify a new quadtree
  // packet (quad set), whether that node has data is stored in the
  // previous quadtree packet. So if it is a leaf, we need to know if
  // it has data, but we don't need to add the corresponding quadtree
  // packet.

  if (root->IsLeaf()) {
    return;
  }

  int start_level = 0;
  std::uint16_t len = address.size() + 1;
  if (len == 1) {
    // Special case for first level (only 3 levels instead of 4).
    len--;
    start_level = 1;
  }

  // Check if address is one of a quadtree packet
  if (!(len & 3)) {
    int num_nodes_in_packet = CountMercatorNodes(root, start_level);
    qtpacket::KhQuadTreePacket16 packet;
    packet.Init(num_nodes_in_packet);

    // Fill in data based on the tree for all nodes in this quadset.
    FillMercatorQuadtreePacket(address, 0, &packet, root, start_level);
    WriteQtPacket(address, packet);

    // Add quadnode to vector.
    // Add its index to stack at index = len / 4
    packet_depth++;
  }

  char base_node = '0';
  for (int i = 0; i < 4; i++) {
    char next_node = base_node + i;
    HiresNode* new_root = root->GetChild(next_node);
    if (new_root != NULL) {
      BuildQuadtreePackets(
          new_root, address + next_node, packet_depth);
    }
  }
}

/**
 * Recursively fill in quadset where there is data based on tree.
 */
int PortableGlobeBuilder::FillMercatorQuadtreePacket(
    std::string address,
    int index,
    qtpacket::KhQuadTreePacket16* packet,
    const HiresNode* root,
    int packet_level) {

  if (packet_level > kMaxPacketLevel) {
    return index;
  }

  qtpacket::KhQuadTreeQuantum16* node = packet->GetPtr(index++);
  node->children.Clear();

  // For now, just use the imagery bit to indicate data.
  // TODO: Consider parameter control of which data to indicate.
  node->children.SetImageBit();
  node->image_version = 1;

  char base_node = '0';
  node->children.ClearCacheNodeBit();
  for (int i = 0; i < 4; i++) {
    char next_node = base_node + i;
    HiresNode* new_root = root->GetChild(next_node);
    node->children.ClearBit(i);
    if (new_root != NULL) {
      // If we are not at a leaf, indicate the children.
      if (packet_level < kMaxPacketLevel) {
        node->children.SetBit(i);
      // If we are at a leaf, indicate the need to load the next packet.
      } else {
        node->children.SetCacheNodeBit();
      }
      index = FillMercatorQuadtreePacket(
          address + next_node, index, packet, new_root, packet_level + 1);
    }
  }

  packet_level++;
  return index;
}

/**
 * Convert quadtree address to Mercator addresses and add those nodes to the
 * quadtree.
 */
void PortableGlobeBuilder::AddMercatorNode(const std::string& qtpath) {
  quadtree_->AddNode(qtpath);
  std::vector<std::string> mercator_qtpaths;
  // Expects leading "0".
  std::string qtaddress = "0" + qtpath;
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtpaths);
  for (size_t i = 0; i < mercator_qtpaths.size(); ++i) {
    std::string merc_qtpath = mercator_qtpaths[i];
    mercator_quadtree_->AddNode(merc_qtpath.substr(1));
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
    const std::string& qtpath,
    std::uint16_t packet_level,
    std::uint16_t packet_depth,
    const std::string& begin_qtpath,
    const std::string& end_qtpath,
    PacketBundleReader& qt_reader) {

  // If we are past the last node, then no use going further.
  // More complicated for "too early" case because once in a qtnode
  // we need to traverse it to get the right index.
  if (qtpath >= end_qtpath) {
    return;
  }

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
  if ((qtpath >= begin_qtpath) && (qtpath < end_qtpath)) {
    if (node->children.GetImageBit()
        && (node->image_version >= min_imagery_version_)) {
      WriteImagePacket(qtpath, node->image_version);
    }
    if (node->children.GetTerrainBit()) {
      WriteTerrainPacket(qtpath, node->terrain_version);
    }
    if (node->children.GetDrawableBit()) {
      for (size_t i = 0; i < node->num_channels(); ++i) {
        // Assuming that channel_type is in increasing order,
        // otherwise we are going to need to sort.
        WriteVectorPacket(qtpath,
                          node->channel_type[i],
                          node->channel_version[i]);
      }
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

    // If we haven't reached the first node, then only go further if
    // it is a prefix node (i.e. so we pick up the base qt packets).
    if (qtpath < begin_qtpath) {
      if (qtpath != begin_qtpath.substr(0, qtpath.length())) {
        return;
      }
    }

    ++packet_depth;
    // Eliminated Manas' lookahead scheme since we don't necessarily run
    // the qtp generation within the same process any more. It would
    // probably be better to read these from the qtp file since these
    // should be in order, so no "lookup" phase should be necessary.
    if (!LoadQtPacket(qtpath, packet_depth, qt_reader)) {
      std::cout << "Unable to download: " << qtpath << std::endl;
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
      ProcessDataInQtPacketNodes(child_qtpath,
                                 packet_level,
                                 packet_depth,
                                 begin_qtpath,
                                 end_qtpath,
                                 qt_reader);
     }
  }
}


/**
 * Recursively walk all of the nodes in the packet. Assume packet has
 * already been pruned. Output quadtree addresses wherever there is
 * imagery data.
 */
void PortableGlobeBuilder::OutputQtAddressesInQtPacketNodes(
    std::ofstream* fp_out,
    const std::string& qtpath,
    std::uint16_t packet_level,
    std::uint16_t packet_depth,
    const std::string& begin_qtpath,
    const std::string& end_qtpath,
    PacketBundleReader& qt_reader) {

  // If we are past the last qtaddress, then no use going further.
  // More complicated for "too early" case (< begin_qtpath)
  // because we still need to traverse the quadset to get to the
  // correct starting point.
  if (qtpath >= end_qtpath) {
    return;
  }

  if (qtnode_index_[packet_depth]
      >= static_cast<std::uint16_t>(
      qtpacket_[packet_depth]->packet_header().num_instances)) {
    notify(NFY_WARN, "Index extended beyond number of instances.\n");
    return;
  }

  qtpacket::KhQuadTreeQuantum16* node =
      qtpacket_[packet_depth]->GetPtr(qtnode_index_[packet_depth]);

  if ((qtpath >= begin_qtpath) && (qtpath < end_qtpath)) {
    if (node->children.GetImageBit()
        && (node->image_version >= min_imagery_version_)) {
      num_image_packets++;
      if (num_image_packets >= imagery_packets_per_mark_) {
        // Output the quadtree address as a qtmark, indicating that we
        // have moved through another #imagery_packets_per_mark_ imagery
        // packets.
        fp_out->write(qtpath.c_str(), qtpath.size());
        fp_out->put('\n');
        num_image_packets = 0;
      }
    }
  }

  // If packet_level has reached the bottom of the quadset,
  // descend to next quadset; if cannot, then return.
  if (packet_level == kMaxPacketLevel) {
    // If we are at a leaf node, we are done here.
    if (!node->children.GetCacheNodeBit()) {
      return;
    }

    // If we haven't reached the first node, then only go further if
    // it is a prefix node (i.e. so we pick up the base qt packets).
    if (qtpath < begin_qtpath) {
      if (qtpath != begin_qtpath.substr(0, qtpath.length())) {
        return;
      }
    }

    ++packet_depth;
    if (!LoadQtPacket(qtpath, packet_depth, qt_reader)) {
      std::cout << "Unable to download: " << qtpath << std::endl;
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
      OutputQtAddressesInQtPacketNodes(fp_out,
                                       child_qtpath,
                                       packet_level,
                                       packet_depth,
                                       begin_qtpath,
                                       end_qtpath,
                                       qt_reader);
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

  int level = qtpath.size();
  qtpacket::KhQuadTreeQuantum16* node =
      qtpacket_[packet_depth]->GetPtr(qtnode_index_[packet_depth]);

  // If we are filtering a pre-existing node, clear all data.
  const QuadtreePath this_qtpath(qtpath);
  const bool keep_node = KeepNode(qtpath);
  // If CacheNodeBit is set, we are not in a leaf node.
  if (keep_node && (packet_level == kMaxPacketLevel) &&
      node->children.GetCacheNodeBit()) {
    // If we are at the max_depth, we don't need the next packet.
    // We should just clear the cached node bit so the Earth client
    // doesn't look any further either.
    if (qtpath.size() == max_level_) {
      node->children.ClearCacheNodeBit();
    // Otherwise, queue next quadtree packet to be loaded.
    } else {
      child_packets->push_back(qtpath);
      if (!is_no_write_) {
        qt_packet_look_ahead_cache_.Append(qtpath);
      }
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
  } else {
    // If below the min level, don't show any data.
    if (level < min_level_) {
      node->children.ClearImageBit();
      node->children.ClearTerrainBit();
      node->children.ClearDrawableBit();

    // Otherwise, collect data stats.
    } else {
      if (!cut_imagery_packets_) {
        node->children.ClearImageBit();
      }
      if (!cut_terrain_packets_) {
        node->children.ClearTerrainBit();
      }
      if (!cut_vector_packets_) {
        node->children.ClearDrawableBit();
      }

      if (node->children.GetImageBit()) {
        max_image_version = std::max(max_image_version, node->image_version);
        ++num_image_packets;
      }

      if (node->children.GetTerrainBit()) {
         max_terrain_version =
             std::max(max_terrain_version, node->terrain_version);
         ++num_terrain_packets;
      }
      if (node->children.GetDrawableBit()) {
        for (size_t i = 0; i < node->num_channels(); ++i) {
          // Assuming that channel_type is in increasing order,
          // otherwise we are going to need to sort.
          ++num_vector_packets;
          max_vector_version[node->channel_type[i]] =
              std::max(max_vector_version[node->channel_type[i]],
                       node->channel_version[i]);
        }
      }
      // TODO: Eliminate no_write option. Just use quadtree cut.
      if (is_no_write_) {
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
            vector_pac_size_req_cache_.GetPacketsSize(
                qtpath, node->channel_version[i], node->channel_type[i]);
          }
        }
      }
    }
  }
}


/**
 * Create directory if needed and create packet writer.
 */
void PortableGlobeBuilder::AddWriter(const std::string& sub_directory,
                                     std::uint32_t file_id,
                                     bool create_delta,
                                     bool is_qtp_bundle,
                                     const std::string& base_glb) {
  std::string directory = globe_directory_ + sub_directory;
  khEnsureParentDir(directory + "/index");
  if (create_delta) {
    writer_ = new PacketBundleWriter(directory,
                                     base_glb,
                                     is_qtp_bundle,
                                     PacketBundle::kMaxFileSize,
                                     file_id);
  } else {
    writer_ = new PacketBundleWriter(
        directory, PacketBundle::kMaxFileSize, file_id);
  }
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
                                         std::uint16_t packet_depth) {
  WriteQtPacket(qtpath, *qtpacket_[packet_depth]);
}

/**
 * Write the updated quadtree packet to the packet bundle.
 */
void PortableGlobeBuilder::WriteQtPacket(
    const std::string& qtpath, const qtpacket::KhQuadTreePacket16& qtpacket) {

  LittleEndianWriteBuffer write_buffer;
  write_buffer.reset();
  write_buffer << qtpacket;
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
    writer_->AppendPacket(full_qtpath, kQtpPacket, 0, raw_packet);
  } else {
    notify(NFY_WARN, "Unable to recompress quadtree node.");
  }
}

}  // namespace fusion_portableglobe
