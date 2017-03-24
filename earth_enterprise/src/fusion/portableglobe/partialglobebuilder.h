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


// Classes for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes. Data referenced by quadtree nodes that are not pruned
// are stored in packet bundles.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PARTIALGLOBEBUILDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PARTIALGLOBEBUILDER_H_

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/portableglobe/cutspec.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "fusion/portableglobe/portablepacketwriter.h"

namespace fusion_portableglobe {

// Concatenates qt_path requests till a chunk of bunch_size is formed.
// When concatenated qt_path requests become bunch_size it calls the callback_.
// At the end one needs to flush remaining queries in cache_ by FlushCache.
template< typename T >
class RequestBundlerForPacketSize {
 public:
  typedef std::pair<std::string, int> QtPathBundle;
  typedef std::map< std::pair<uint32, uint32>, QtPathBundle > Bundle;
  typedef void (T::*FunctionType)(const std::string& paths, size_t num_paths,
                                  uint32 version, uint32 channel);

 private:
  Bundle bundle_;

  const size_t bunch_size_;
  T* const caller_;
  FunctionType const callback_;
 public:
  RequestBundlerForPacketSize(int bunch_size, T* builder,
                              FunctionType func);
  void GetPacketsSize(
      const std::string& qt_path, uint32 version, uint32 channel);
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
 * TODO: Always use gepartialglobebuilder.
 * TODO: I.e. Remove geportableglobebuilder.
 */
class PortableGlobeBuilder : public PortableBuilder {
 public:
  // Names of subdirectories within the portable globe.
  static const std::string kDataDirectory;
  static const std::string kImageryDirectory;
  static const std::string kTerrainDirectory;
  static const std::string kVectorDirectory;
  static const std::string kMercatorQtPacketDirectory;
  static const std::string kQtPacketDirectory;
  static const std::string kQtPacket2Directory;
  static const std::string kIconsDirectory;
  static const std::string kPluginDirectory;
  static const std::string kSearchDirectory;
  // Argument indicating a multi-packet request.
  static const std::string kMultiPacket;
  static const size_t      kBundleSizeForSizeQuery;
  static const size_t      kBundleSizeForPacketQuery;
  static const uint16 kMaxPacketLevel;
  static const uint32 kImageryPacketsPerMark;


  // Used to accumulate size during a "no write" run.
  uint64 total_size;
  uint64 num_image_packets;
  uint64 num_terrain_packets;
  uint64 num_vector_packets;
  uint64 image_size;
  uint64 terrain_size;
  uint64 vector_size;
  uint16 max_image_version;
  uint16 max_terrain_version;
  // Handle multiple vector channels:
  // map[channel] = max_version
  std::map<uint16, uint16> max_vector_version;

  /**
   * Production constructor.
   */
  PortableGlobeBuilder(const std::string& source,
                       uint16 default_level,
                       uint16 min_level,
                       uint16 max_level,
                       const std::string& hires_qt_nodes_file,
                       const std::string& dbroot_file,
                       const std::string& globe_directory,
                       const std::string& additional_args,
                       const std::string& qtpacket_version,
                       const uint16  min_imagery_version,
                       bool no_write,
                       bool use_post,
                       const std::string& data_type,
                       uint32 imagery_packets_per_mark);

  /**
   * Testing constructor.
   */
  PortableGlobeBuilder(uint16 default_level,
                       uint16 max_level,
                       const std::string& hires_qt_nodes_file);

  ~PortableGlobeBuilder();

  /**
   * Build packet bundle for the quadtree and emit quadtree addresses for
   * each specified number of imagery packets (default is 200,000 or about
   * 2GB per packet bundle). The addresses can then be used to partially
   * cut roughly equal sized sections of the globe.
   */
  void BuildQuadtree();

  void BuildMercatorQuadtree();

  /**
   * Build packet bundle for the quadtree that indicates imagery data
   * at all potential nodes based on constraints, namely the default
   * level, max_level, and hires qtnodes (typically defined by one or
   * more polygons).
   */
  void BuildVectorQuadtree();

  void ShowQuadtree(std::string path);

  /**
   * Build packet bundle for all data types between the given starting
   * and ending quadtree addresses. Add data based on
   * whether corresponding quadtree node is to be kept in the globe.
   */
  void BuildPartialGlobe(uint32 file_index,
                         const std::string& partial_start,
                         const std::string& partial_end,
                         bool create_delta,
                         const std::string& base_glb);
  /**
   * Output partial globe quadtree addresses to the given file.
   */
  void OutputPartialGlobeQtAddresses(const std::string& file_name,
                                     const std::string& partial_start,
                                     const std::string& partial_end);
  /**
   * Returns whether quadtree node and the data it points to should be
   * included in the portable globe.
   */
  bool KeepNode(const std::string& qtpath) const;

  // 3rd param will not be used
  void GetImagePacketsSize(
      const std::string& paths, size_t num_paths, uint32 version, uint32);
  // 3rd param will not be used
  void GetTerrainPacketsSize(
      const std::string& paths, size_t num_paths, uint32 version, uint32);
  // Existense of 3rd param is to bring uniformity for this vector
  void GetVectorPacketsSize(const std::string& paths, size_t num_paths,
                            uint32 version, uint32 channel);

  void GetImagePacket(
    const std::string& qtpath, uint32 version, std::string* raw_packet);

  void GetTerrainPacket(
    const std::string& qtpath, uint32 version, std::string* raw_packet);

  void GetVectorPacket(
    const std::string& qtpath, uint32 channel, uint32 version,
    std::string* raw_packet);

 private:
  static const uint16 kMaxPacketDepth;

  /**
   * Prune all nodes inside of the quadtree packet and its children
   * packets.
   */
  void PruneQtPacket(const std::string& packet_qtpath,
                     const uint16 packet_depth);

  void PruneQtPacketNodes(const std::string& qtpath,
                          uint16 packet_level,
                          uint16 packet_depth,
                          std::vector<std::string>* child_packets);

  bool BuildMercatorDataTree(
      const std::string& qtpath,
      uint16 packet_level,
      uint16 packet_depth,
      PacketBundleReader& qt_reader);

  void BuildVectorDataTree(const std::string& qtpath);

  int FillMercatorQuadtreePacket(
      std::string address,
      int index,
      qtpacket::KhQuadTreePacket16* packet,
      const HiresNode* root,
      int packet_level);

  void AddMercatorNode(const std::string& qtpath);

  int CountMercatorNodes(const HiresNode* root, int packet_level);

  int FillMercatorNodes(
      const HiresNode* root, int packet_level, int index);

  void BuildQuadtreePackets(const HiresNode* root,
                            const std::string address,
                            int packet_depth);

  void ProcessDataInQtPacketNodes(const std::string& qtpath,
                                  uint16 packet_level,
                                  uint16 packet_depth,
                                  const std::string& begin_qtpath,
                                  const std::string& end_qtpath,
                                  PacketBundleReader& qt_reader);

  void OutputQtAddressesInQtPacketNodes(std::ofstream* fp_out,
                                        const std::string& qtpath,
                                        uint16 packet_level,
                                        uint16 packet_depth,
                                        const std::string& begin_qtpath,
                                        const std::string& end_qtpath,
                                        PacketBundleReader& qt_reader);

  void ShowQuadtreePacket(const std::string& qtpath,
                          uint16 packet_level,
                          uint16 packet_depth,
                          PacketBundleReader& qt_reader);

  void AddWriter(PacketType packet_type,
                 PacketBundleWriter* writer);

  void WritePacket(PacketType packet_type,
                   std::string qtpath, std::string data);

  /**
   * Download the next quadtree packet from the server.
   */
  bool DownloadQtPacket(const std::string& packet_qtpath,
                        uint16 packet_depth);

  bool LoadQtPacket(const std::string& packet_qtpath,
                    uint16 packet_depth,
                    PacketBundleReader& qt_reader);

  void DownloadQtPacketBunch(const std::vector<std::string>& qt_paths,
                             LittleEndianReadBuffer* buffer_array);

  void AddWriter(const std::string& sub_directory,
                 uint32 file_id,
                 bool create_delta,
                 bool is_qtp_bundle,
                 const std::string& base_glb);

  void DeleteWriter();

  uint64 GetPacketSize(const std::string& url);

  void WriteImagePacket(const std::string& qtpath, const uint32 version);

  /**
   * Get terrain packet for the given quadtree path and add it to the
   * terrain packet bundle.
   */
  void WriteTerrainPacket(const std::string& qtpath, uint32 version);
  void WriteVectorPacket(const std::string& qtpath,
                         uint32 channel_type,
                         uint32 version);
  void WriteDbRootPacket(PacketType packet_type);
  void WriteQtPacket(const std::string& qtpath,
                     uint16 packet_depth);
  void WriteQtPacket(const std::string& qtpath,
                     const qtpacket::KhQuadTreePacket16& qtpacket);

  // Main directory that contains all portable globe data.
  std::string globe_directory_;
  // Path to temporary file for holding re-written dbroot.
  std::string dbroot_file_;
  // Level at or below which all assets are kept.
  uint16 default_level_;
  // Level below which no assets are kept.
  uint16 min_level_;
  // Level above which no assets are kept.
  uint16 max_level_;
  // Url to master globe server.
  const std::string source_;
  // Additional args to be appended to each query.
  std::string additional_args_;
  // Version associated with the quadtree packets.
  std::string qtpacket_version_;
  // Must be this version or higher for packet to be requested.
  uint16 min_imagery_version_;
  // Master globe server reader service.
  gstSimpleEarthStream* server_;
  // Tree identifying quadtree nodes to keep.
  HiresTree* hires_tree_;
  // Reconstructed quadtree for some channel.
  HiresTree* quadtree_;
  // Reconstructed quadtree for some channel.
  HiresTree* mercator_quadtree_;

  // Types for selective cutting.
  bool cut_imagery_packets_;
  bool cut_terrain_packets_;
  bool cut_vector_packets_;

  // Current quadtree node packet being processed.
  std::vector<qtpacket::KhQuadTreePacket16*> qtpacket_;
  std::vector<uint32> qtnode_index_;

  // Packet bundle writers for the different asset types.
  PacketBundleWriter* writer_;
  // Packet bundle reader for quadtree packets.
  khDeleteGuard<PacketBundleReader> qtp_reader_;
  // Flag indicating that data shouldn't be written, just checked for size.
  bool is_no_write_;
  // Imagery packets per quadtree address sent to std out.
  uint32 imagery_packets_per_mark_;
  RequestBundlerForPacket* write_cache_;
  QtPacketLookAheadRequestor<PortableGlobeBuilder>  qt_packet_look_ahead_cache_;
  RequestBundlerForPacketSize<PortableGlobeBuilder> image_pac_size_req_cache_;
  RequestBundlerForPacketSize<PortableGlobeBuilder> terrain_pac_size_req_cache_;
  RequestBundlerForPacketSize<PortableGlobeBuilder> vector_pac_size_req_cache_;

  DISALLOW_COPY_AND_ASSIGN(PortableGlobeBuilder);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PARTIALGLOBEBUILDER_H_
