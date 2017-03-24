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


// Class wraps a reader and an unpacker for a single file globe (glc).

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_GLC_UNPACKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_GLC_UNPACKER_H_

#include <map>
#include <string>
#include <vector>
#include "./file_package.h"
#include "./file_unpacker.h"
#include "./glc_reader.h"
#include "./packetbundle_finder.h"

/**
 * Class for unpacking files from a single file that is a composite of
 * package files. If it actually just a package file, this is supported
 * as well.
 */
class GlcUnpacker {
 public:
  GlcUnpacker(const GlcReader& glc_reader,
              bool is_composite,
              bool files_only = false);
  ~GlcUnpacker();

  /**
   * Find data packet and set offset and size for the packet. Data packets can
   * be imagery, terrain or vectors.
   * @param qtpath Quadtree path (with leading 0).
   * @param packet_type Type of packet data (e.g. imagery).
   * @param channel Channel id for the packet data.
   *                (fixed for imagery: 0 and terrain: 1).
   * @param layer Layer (side database) within the meta dbroot.
   * @param data_loc Returns location of packet data.
   * @return whether data packet was found.
   */
  bool FindDataPacket(const char* qtpath,
                      int packet_type,
                      int channel,
                      int layer,
                      PackageFileLoc* data_loc);

  /**
   * Find map data packet and set offset and size for the packet. Data packets
   * can be map imagery or vectors.
   * @param qtpath Quadtree path (with leading 0).
   * @param packet_type Type of packet data (e.g. imagery).
   * @param channel Channel id for the packet data.
   *                (fixed for imagery: 0 and terrain: 1).
   * @param layer Layer (side database) within the meta dbroot.
   * @param data_loc Returns location of packet data.
   * @return whether data packet was found.
   */
  bool FindMapDataPacket(const char* qtpath,
                         int packet_type,
                         int channel,
                         int layer,
                         PackageFileLoc* data_loc);

  /**
   * Find qtp packet and set offset and size for the packet. Qtp packets can
   * be quadtree packets or a dbroot packet.
   * @param qtpath Quadtree path (with leading 0).
   * @param packet_type Type of packet data (should be qtpacket).
   * @param channel Channel id for the packet data.
   *                (fixed for imagery: 0 and terrain: 1).
   * @param layer Layer (side database) within the meta dbroot.
   * @param data_loc Returns location of packet data.
   * @return whether qtp packet was found.
   */
  bool FindQtpPacket(const char* qtpath,
                     int packet_type,
                     int channel,
                     int layer,
                     PackageFileLoc* data_loc);

  /**
   * Find max LOD where there is data of some kind at the given address.
   * Does not look at resolutions higher than the given address so
   * possible values are 0 to len(qtpath) - 1.
   * @param qtpath Quadtree path of highest resolution node at which to begin
   *               looking for data (with leading 0).
   * @param is_2d whether looking for 2d data (otherwise, check for 3d data).
   * @return max LOD with tile that covers given node.
   */
  int MaxResolutionLod(const char* qtpath, bool is_2d);

  /**
   * Returns whether there are any data at the given address.
   * @param qtpath Quadtree path of node to check (with leading 0).
   * @param is_2d whether looking for 2d data (otherwise, check for 3d data).
   * @return whether there is any data at given quadtree node.
   */
  bool HasData(const char* qtpath, bool is_2d);

  /**
   * Find file in a particular layer and set offset and size for the
   * file content.
   * @param file_name Relative path to file in package.
   * @param file_loc Returns location of file data.
   * @return whether file was found.
   */
  bool FindLayerFile(
      const char* file_name, int layer, PackageFileLoc* file_loc);

  /**
   * Find file and set offset and size for the file content.
   * @param file_name Relative path to file in package.
   * @param file_loc Returns location of file data.
   * @return whether file was found.
   */
  bool FindFile(const char* file_name, PackageFileLoc* file_loc);

  /**
   * Find meta dbroot from a 3d glc file and set offset and size for the
   * content.
   * @param data_loc Returns location of meta dbroot data.
   * @return whether meta dbroot was found.
   */
  bool FindMetaDbRoot(PackageFileLoc* data_loc);

  /**
   * Find dbroot from a 3d glc or glb file and set offset and size for the
   * content.
   * @param data_loc Returns location of dbroot data.
   * @return whether dbroot was found.
   */
  bool FindDbRoot(PackageFileLoc* data_loc);

  /**
   * Return whether it's a legal GEE globe or map.
   */
  bool IsGee() const { return is_gee_; }

  /**
   * Return whether file contains 2d data.
   */
  bool Is2d() const { return is_2d_; }

  /**
   * Return whether file contains 3d data.
   */
  bool Is3d() const { return is_3d_; }

  /**
   * Return the file's id. The id is based on contents of the file
   * stored at earth/info.txt and the file's length.
   */
  const char* Id();

  /**
   * Return the data from earth/info.txt. Returns an empty string
   * if there isn't any or the file is not contained within the
   * bundle.
   */
  const char* Info() { return info_.c_str(); }

  /**
   * Get idx-th file in index.
   */
  const char* IndexFile(int idx);

  /**
   * Get path of file in index.
   */
  int IndexSize() { return index_.size(); }

  /**
   * Get number of layers in current globe or map.
   */
  int NumberOfLayers();

  /**
   * Get nth layer's index.
   */
  int LayerIndex(int n);

  /**
   * Get the index for debugging tools.
   */
  const std::map<std::string, PackageFileLoc>& Index() { return index_; }

 private:
  /**
   * Set up file_unpackers for each layer in the composite.
   */
  void SetUpLayerUnpackers(bool files_only);

  /**
   * Returns the unpacker to access the given layer.
   */
  FileUnpacker* GetUnpacker(int layer);

  /**
   * Helper for reading in consecutive fields from the glc file.
   */
  bool Read(void* data, uint64 size);

  /**
   * Helper for reading creation date for glc file
   * from info data.
   */
  std::string ExtractDateFromInfo();

  /**
   * Helper for creating a crc from info data.
   */
  uint32 InfoCrc();

  // Length of full packed file.
  uint64 length_;
  std::string info_;
  std::string id_;

  // Reads from the glc file.
  const GlcReader& glc_reader_;
  // Map of path to offset/size of packed file data.
  std::map<std::string, PackageFileLoc> index_;
  // Map of layer id to layer unpacker index.
  std::map<int, FileUnpacker*> unpacker_index_;
  // Map of layer id to parent layer id.
  std::map<int, int> parent_layer_;
  // Map of layer id to layer_type.
  std::map<int, std::string> layer_type_;
  // Path to meta dbroot.
  std::string meta_dbroot_path_;
  // Whether file is composite (glc) or simple (glb or glm).
  bool is_composite_;
  // Whether it's a legal GEE file.
  bool is_gee_;
  // Whether file contains 2d data.
  bool is_2d_;
  // Whether file contains 3d data.
  bool is_3d_;
  // Offset for consecutive reads from the reader.
  uint64 reader_offset_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_GLC_UNPACKER_H_

