/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 * Copyright 2020 Open GEE Contributors
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


// Classes for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes. Data referenced by quadtree nodes that are not pruned
// are stored in packet bundles.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/portableglobe/shared/bounds_tracker.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "fusion/portableglobe/portablepacketwriter.h"

namespace fusion_portableglobe {

/**
 * Class for determining if a quadtree node is one of the high resolution
 * nodes to be saved, or whether it should be pruned away.
 */
class HiresNode {
 public:
  /**
   * Empty contructor is only used for the root of the tree.
   */
  HiresNode() : is_leaf(false) { }

  explicit HiresNode(std::string path);

  /**
   * Returns child node for node nums 0-3. If there is no child node
   * for that node num, it returns null.
   */
  HiresNode* const GetChild(char node_num) const;

  /**
   * Adds node as a child of this node in the given node_num (0-3)
   * position.
   */
  void AddChild(char node_num, HiresNode* child);

  /**
   * Returns whether this node is a leaf (i.e. it has no
   * children).
   */
  bool IsLeaf() const { return is_leaf; }

  /**
   * Make node a leaf.
   */
  void MakeLeaf() { is_leaf = true; }

  /**
   * Returns whether the given path specifies a node that is encompassed
   * by the tree for which this node is the root.
   */
  bool IsTreePath(const std::string& path) const;

  /**
   * Debugging routine for showing the content of the tree starting
   * from this node as the root.
   */
  void Show(std::string indent) const;

 private:
  static const size_t ZERO_CHAR = (size_t) '0';

  bool is_leaf;
  // If not a leaf, then it must have between 1 and 4 children (inclusive).
  std::vector<HiresNode*> children;

  /**
   * Returns index associated with the given path character.
   */
  size_t GetChildIndex(char node_num) const;

  DISALLOW_COPY_AND_ASSIGN(HiresNode);
};


/**
 * Class for maintaining a tree of HiresNodes.
 *
 * The leaf nodes are nodes that have been explicitly marked to be kept.
 * The paths to the leaf nodes consist of nodes at higher levels that
 * encompass the specified leaf node. The path encodes the expanding area
 * being represented that ensures that insets in specified hi-res areas
 * are visible at all levels of the globe.
 */
class HiresTree {
 public:
  explicit HiresTree()
      : hires_tree_(new HiresNode)
        { }

  /**
   * Load hi-res nodes from a list in a file.
   */
  void LoadHiResQtNodeFile(const std::string& file_name);

  /**
   * Load hi-res nodes from an input stream.
   */
  void LoadHiResQtNodes(std::istream* fin);

  /**
   * Returns whether the given path specifies a node that is encompassed
   * by any of the trees in this set of trees.
   */
  bool IsTreePath(const std::string& path);

 private:
  HiresNode* hires_tree_;

  void AddHiResQtNode(HiresNode* node, const std::string& path);
};


// Concatenates qt_path requests till a chunk of bunch_size is formed.
// When concatenated qt_path requests become bunch_size it calls the callback_.
// At the end one needs to flush remaining queries in cache_ by FlushCache.
template< typename T >
class RequestBundlerForPacketSize {
 public:
  typedef std::pair<std::string, int> QtPathBundle;
  typedef std::map< std::pair<std::uint32_t, std::uint32_t>, QtPathBundle > Bundle;
  typedef void (T::*FunctionType)(const std::string& paths, size_t num_paths,
                                  std::uint32_t version, std::uint32_t channel);

 private:
  Bundle bundle_;

  const size_t bunch_size_;
  T* const caller_;
  FunctionType const callback_;
 public:
  RequestBundlerForPacketSize(int bunch_size, T* builder,
                              FunctionType func);
  void GetPacketsSize(
      const std::string& qt_path, std::uint32_t version, std::uint32_t channel);
  void FlushCache();
};


// Qt packet requests take most of the time in connection to the GEE server.
// The alternative to request from the local file-bundle is no good as it has
// file based binary search and too slow (without a cache). So we do a in-order
// traversal and store enough Qt packets, by requesting ahead. This way the
// connection time is amoratized over a set of requests. The Con is some
// of such in-order successors may have QT packets associated with those, and we
// don't know about that, as we don't have parent QT packets
// node->children.GetBit to know about it.
template< typename T >
class QtPacketLookAheadRequestor {
 public:
  typedef bool (T::*KeepNodeType)(const std::string& qtpath) const;
  typedef void (T::*DownloadQtPacketBunchType)(
      const std::vector<std::string>& qt_paths,
      LittleEndianReadBuffer* buffer_array);

  QtPacketLookAheadRequestor(size_t bunch_size,
                             T* builder,
                             DownloadQtPacketBunchType func);
  bool DownloadQtPacket(const std::string& qtpath,
                        qtpacket::KhQuadTreePacket16* packet);
  void Prefetch(const std::vector<std::string>& paths);
  void Append(QuadtreePath path) { paths_with_qt_packets_.push_back(path); }
  void Sort() {
    std::sort(paths_with_qt_packets_.begin(), paths_with_qt_packets_.end());
  }
 private:
  const size_t bunch_size_;
  T* const caller_;
  DownloadQtPacketBunchType const download_qt_packet_bunch_;
  std::vector<std::string> cached_paths_;
  khDeleteGuard<LittleEndianReadBuffer, ArrayDeleter> const cached_packets_;
  size_t cache_index_;
  size_t prefetched_;
  std::vector<QuadtreePath> paths_with_qt_packets_;
};

/**
 * Class for constructing a portable globe.
 * Traverses quadtree packets and prunes out the ones that
 * are not required. Writes all of packet bundles of the nodes
 * that are kept.
 */
class PortableGlobeBuilder : public PortableBuilder {
 public:
  // Names of subdirectories within the portable globe.
  static const std::string kDataDirectory;
  static const std::string kImageryDirectory;
  static const std::string kTerrainDirectory;
  static const std::string kVectorDirectory;
  static const std::string kQtPacketDirectory;
  static const std::string kQtPacket2Directory;
  static const std::string kIconsDirectory;
  static const std::string kPluginDirectory;
  static const std::string kSearchDirectory;
  // Argument indicating a multi-packet request.
  static const std::string kMultiPacket;
  static const size_t      kBundleSizeForSizeQuery;
  static const size_t      kBundleSizeForPacketQuery;
  static const std::uint16_t kMaxPacketLevel;

  // Used to accumulate size during a "no write" run.
  std::uint64_t total_size;
  std::uint64_t num_image_packets;
  std::uint64_t num_terrain_packets;
  std::uint64_t num_vector_packets;
  std::uint64_t image_size;
  std::uint64_t terrain_size;
  std::uint64_t vector_size;

  /**
   * Production constructor.
   */
  PortableGlobeBuilder(const std::string& source,
                       std::uint16_t default_level,
                       std::uint16_t max_level,
                       const std::string& hires_qt_nodes_file,
                       const std::string& dbroot_file,
                       const std::string& globe_directory,
                       const std::string& additional_args,
                       const std::string& qtpacket_version,
                       const std::string& metadata_file,
                       bool no_write,
                       bool use_post);

  /**
   * Testing constructor.
   */
  PortableGlobeBuilder(std::uint16_t default_level,
                       std::uint16_t max_level,
                       const std::string& hires_qt_nodes_file);

  ~PortableGlobeBuilder();

  /**
   * Build packet bundles for all specified data types. Add data based on
   * whether corresponding quadtree nodes is to be kept in the globe.
   */
  void BuildGlobe();

  /**
   * Returns whether quadtree node and the data it points to should be
   * included in the portable globe.
   */
  bool KeepNode(const std::string& qtpath) const;

  // 3rd param will not be used
  void GetImagePacketsSize(
      const std::string& paths, size_t num_paths, std::uint32_t version, std::uint32_t channel);
  // 3rd param will not be used
  void GetTerrainPacketsSize(
      const std::string& paths, size_t num_paths, std::uint32_t version, std::uint32_t channel);
  // Existense of 3rd param is to bring uniformity for this vector
  void GetVectorPacketsSize(const std::string& paths, size_t num_paths,
                            std::uint32_t version, std::uint32_t channel);

  void GetImagePacket(
    const std::string& qtpath, std::uint32_t version, std::string* raw_packet);

  void GetTerrainPacket(
    const std::string& qtpath, std::uint32_t version, std::string* raw_packet);

  void GetVectorPacket(
    const std::string& qtpath, std::uint32_t channel, std::uint32_t version,
    std::string* raw_packet);

 private:
  static const std::uint16_t kMaxPacketDepth;

  /**
   * Prune all nodes inside of the quadtree packet and its children
   * packets.
   */
  void PruneQtPacket(const std::string& packet_qtpath,
                     const std::uint16_t packet_depth);

  void PruneQtPacketNodes(const std::string& qtpath,
                          std::uint16_t packet_level,
                          std::uint16_t packet_depth,
                          std::vector<std::string>* child_packets);

  void ProcessDataInQtPacketNodes(const std::string& qtpath,
                                  std::uint16_t packet_level,
                                  std::uint16_t packet_depth);

  void AddWriter(PacketType packet_type,
                 PacketBundleWriter* writer);

  void WritePacket(PacketType packet_type,
                   std::string qtpath, std::string data);

  /**
   * Download the next quadtree packet from the server.
   */
  bool DownloadQtPacket(const std::string& packet_qtpath,
                        std::uint16_t packet_depth);

  void DownloadQtPacketBunch(const std::vector<std::string>& qt_paths,
                             LittleEndianReadBuffer* buffer_array);

  void AddWriter(const std::string& sub_directory);

  void DeleteWriter();

  std::uint64_t GetPacketSize(const std::string& url);

  void WriteImagePacket(const std::string& qtpath, const std::uint32_t version);

  /**
   * Get terrain packet for the given quadtree path and add it to the
   * terrain packet bundle.
   */
  void WriteTerrainPacket(const std::string& qtpath, std::uint32_t version);
  void WriteVectorPacket(const std::string& qtpath,
                         std::uint32_t channel_type,
                         std::uint32_t version);
  void WriteDbRootPacket(PacketType packet_type);
  void WriteQtPacket(const std::string& qtpath,
                     std::uint16_t packet_depth,
                     PacketType packet_type);

  // Main directory that contains all portable globe data.
  std::string globe_directory_;
  // Path to temporary file for holding re-written dbroot.
  std::string dbroot_file_;
  // Level at or below which all assets are kept.
  std::uint16_t default_level_;
  // Level above which no assets are kept.
  std::uint16_t max_level_;
  // Url to master globe server.
  const std::string source_;
  // Additional args to be appended to each query.
  std::string additional_args_;
  // Version associated with the quadtree packets.
  std::string qtpacket_version_;
  // Master globe server reader service.
  gstSimpleEarthStream* server_;
  // Forest of trees identifying quadtree nodes to keep.
  HiresTree* hires_tree_;

  // Current quadtree node packet being processed.
  std::vector<qtpacket::KhQuadTreePacket16*> qtpacket_;
  std::vector<std::uint32_t> qtnode_index_;

  // Packet bundle writers for the different asset types.
  PacketBundleWriter* writer_;
  // Packet bundle reader for quadtree packets.
  khDeleteGuard<PacketBundleReader> qtp_reader_;
  // Flag indicating that data shouldn't be written, just checked for size.
  bool is_no_write_;
  RequestBundlerForPacket* write_cache_;
  QtPacketLookAheadRequestor<PortableGlobeBuilder>  qt_packet_look_ahead_cache_;
  RequestBundlerForPacketSize<PortableGlobeBuilder> image_pac_size_req_cache_;
  RequestBundlerForPacketSize<PortableGlobeBuilder> terrain_pac_size_req_cache_;
  RequestBundlerForPacketSize<PortableGlobeBuilder> vector_pac_size_req_cache_;

  // Path to file that will hold metadata.
  std::string metadata_file_;
  // Structure to keep track of globe boundaries.
  BoundsTracker bounds_tracker_;

  DISALLOW_COPY_AND_ASSIGN(PortableGlobeBuilder);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_
