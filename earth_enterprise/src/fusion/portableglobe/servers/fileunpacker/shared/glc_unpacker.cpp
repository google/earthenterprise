// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


// Uses unpacker to get data from the glc file.

#include "./glc_unpacker.h"
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include <cstring>
#include <cassert>
#include "./file_package.h"
#include "./file_unpacker.h"
#include "./glc_reader.h"

const char* LAYER_INFO = "earth/layer_info.txt";
// Types for layers
const char* IMAGERY_LAYER_TYPE = "IMAGE";
const char* TERRAIN_LAYER_TYPE = "TERRAIN";
const char* VECTOR_LAYER_TYPE = "VECTOR";
const char* MAP_LAYER_TYPE = "MAP";

// GEE identifying bytes in all glxs.
const std::uint32_t GEE_ID = 0x47454547;
const std::uint64_t GEE_ID_OFFSET = 8;

/**
 * Constructor. Load the index to all of the files and
 * packages in the composite file. If this is a package file,
 * treat it as a composite with a single layer that is the
 * whole file. If files_only flag is true, only read
 * top level files (not used for serving tiles).
 */
GlcUnpacker::GlcUnpacker(const GlcReader& glc_reader,
                         bool is_composite,
                         bool files_only)
    : glc_reader_(glc_reader),
    is_composite_(is_composite),
    is_gee_(false),
    is_2d_(false),
    is_3d_(false) {
  length_ = glc_reader_.Size();

  // Check file is a GEE file first.
  std::uint32_t id;
  std::int64_t offset = length_ - GEE_ID_OFFSET;
  if (!glc_reader_.ReadData(&id, offset, sizeof(id))) {
    std::cerr << "Unable to read file id." << std::endl;
    return;
  }
  if (id != GEE_ID) {
    std::cerr << "Id does not match expected GEE id." << std::endl;
    return;
  } else {
    is_gee_ = true;
  }

  // Get offset to index.
  offset = length_ - Package::kIndexOffsetOffset;
  std::uint32_t num_files = 0;
  std::int64_t index_offset;
  if (!glc_reader_.ReadData(
      &index_offset, offset, Package::kIndexOffsetSize)) {
    std::cerr << "Unable to read file offset." << std::endl;
    return;
  }
  // If negative, use as offset from end of file.
  if (index_offset < 0) {
    reader_offset_ = length_ + index_offset;
  } else {
    reader_offset_ = index_offset;
  }

  // Read in the index header.
  if (!Read(&num_files, sizeof(num_files))) {
    std::cerr << "Unable to read number of files in file." << std::endl;
    return;
  }

  // Read in the index entries.
  std::string path;
  PackageFileLoc file_loc;
  for (std::uint32_t i = 0; i < num_files; ++i) {
    // Read in the relative path of the index entry.
    std::uint16_t path_len;
    if (!Read(&path_len, sizeof(path_len))) {
      std::cerr << "Unable to read file name length." << std::endl;
      return;
    }

    path.resize(path_len);
    if (!Read(&path[0], path_len)) {
      std::cerr << "Unable to read file name." << std::endl;
      return;
    }

    // Read in the size and offset of the index entry.
    if (!Read(&file_loc, sizeof(PackageFileLoc))) {
      std::cerr << "Unable to read file size and offset." << std::endl;
      return;
    }
    // If offsets in index are negative, add them to file size to get offset
    // relative to beginning of file.
    file_loc.NormalizeNegativeOffset(length_);
    index_[path] = file_loc;
  }

  // For a real composite, set up file_unpacker for each layer.
  if (is_composite) {
    SetUpLayerUnpackers(files_only);

  // If this is just a glb or glm, set up a single layer 0.
  } else {
    std::string suffix = glc_reader_.Suffix();
    if (suffix == "glb") {
      is_3d_ = true;
    } else if (suffix == "glm")  {
      is_2d_ = true;
    } else {
      std::cout << "Unknown suffix: " << suffix << std::endl;
      return;
    }

    if (files_only) {
      return;
    }

    unpacker_index_[0] = new FileUnpacker(glc_reader_, 0, length_);
  }

  // Read in earth/info.txt
  std::map<std::string, PackageFileLoc>::iterator it
      = index_.find("earth/info.txt");
  if (it != index_.end()) {
    file_loc = it->second;
    if (!glc_reader_.Read(&info_, file_loc.Offset(), file_loc.Size())) {
      std::cerr << "Unable to read earth/info.txt" << std::endl;
    }
  }
}

GlcUnpacker::~GlcUnpacker() {
  std::map<int, FileUnpacker*>::iterator it;
  for (it = unpacker_index_.begin(); it != unpacker_index_.end(); ++it) {
    delete it->second;
  }
}

/**
 * Helper method for the Constructor that creates a file_unpacker for each
 * layer listed in the layer_info.txt file.
 */
void GlcUnpacker::SetUpLayerUnpackers(bool files_only) {
  PackageFileLoc file_loc;
  if (!FindFile(LAYER_INFO, &file_loc)) {
    std::cerr << "Unable to load layer info." << std::endl;
    return;
  }

  std::string layer_info;
  if (!glc_reader_.Read(&layer_info, file_loc.Offset(), file_loc.Size())) {
    std::cerr << "Unable to read layer info." << std::endl;
    return;
  }

  std::stringstream stream(
      layer_info, std::stringstream::in);

  std::string layer_name;
  std::string layer_path;
  std::string layer_type;
  int layer_id;
  int parent_layer;

  // Read in layer info.
  while (true) {
    stream >> layer_name;
    stream >> layer_path;
    stream >> layer_type;
    stream >> layer_id;

    // Ensure that a full line of data was available.
    if (stream.eof()) {
      break;
    }

    stream >> parent_layer;

    // Open new unpacker for glb layer.
    if ((layer_type == IMAGERY_LAYER_TYPE) ||
        (layer_type == TERRAIN_LAYER_TYPE) ||
        (layer_type == VECTOR_LAYER_TYPE) ||
        (layer_type == MAP_LAYER_TYPE)) {
      if (layer_type == MAP_LAYER_TYPE) {
        is_2d_ = true;
      } else {
        is_3d_ = true;
      }
      if (!files_only) {
        std::map<std::string, PackageFileLoc>::iterator it =
            index_.find(layer_path);
        if (it != index_.end()) {
          unpacker_index_[layer_id] = new FileUnpacker(
              glc_reader_, it->second.Offset(), it->second.Size());
          parent_layer_[layer_id] = parent_layer;
          layer_type_[layer_id] = layer_type;
        } else {
          std::cerr << "Missing layer: " << layer_path << std::endl;
        }
      }
    } else if (layer_type == "DBROOT") {
      meta_dbroot_path_ = layer_path;
    }
  }
}

/**
 * Helper for reading in consecutive fields from the glc file.
 */
bool GlcUnpacker::Read(void* data, std::uint64_t size) {
  if (glc_reader_.ReadData(data, reader_offset_, size)) {
    reader_offset_ += size;
    return true;
  }
  return false;
}

/**
 * Find data packet and set offset and size for the packet. Data packets can
 * be imagery, terrain or vectors.
 */
bool GlcUnpacker::FindDataPacket(const char* qtpath,
                                 int packet_type,
                                 int channel,
                                 int layer,
                                 PackageFileLoc* data_loc) {
  if (!is_gee_) {
    std::cerr << "Not a GEE file." << std::endl;
    return false;
  }

  FileUnpacker* unpacker;
  while (true) {
    unpacker = GetUnpacker(layer);
    if (unpacker) {
      if (unpacker->FindDataPacket(qtpath, packet_type, channel, data_loc)) {
        return true;
      } else {
        layer = parent_layer_[layer];
        if (layer == 0) {
          return false;
        }
      }
    } else {
      std::cerr << "No unpacker for " << layer << std::endl;
      return false;
    }
  }
}

/**
 * Find map data packet and set offset and size for the packet.
 * Map data packets can be imagery or vectors.
 */
bool GlcUnpacker::FindMapDataPacket(const char* qtpath,
                                    int packet_type,
                                    int channel,
                                    int layer,
                                    PackageFileLoc* data_loc) {
  if (!is_gee_) {
    std::cerr << "Not a GEE file." << std::endl;
    return false;
  }

  FileUnpacker* unpacker;
  while (true) {
    unpacker = GetUnpacker(layer);
    if (unpacker) {
      if (unpacker->FindMapDataPacket(qtpath, packet_type, channel, data_loc)) {
        return true;
      } else {
        layer = parent_layer_[layer];
        if (layer == 0) {
          return false;
        }
      }
    } else {
      return false;
    }
  }
}


void GlcUnpacker::MapDataPacketWalker(const map_packet_walker& walker) const
{
  if (!is_gee_) {
    std::cerr << "Not a GEE file." << std::endl;
    return;
  }

  for( auto& index_item : unpacker_index_) {
    if (index_item.second->MapDataPacketWalker(index_item.first, walker) == true) {
      break;
    }
  }
}

void GlcUnpacker::MapDataPacketWalker(int layer, const map_packet_walker& walker) const
{
  if (!is_gee_) {
    std::cerr << "Not a GEE file." << std::endl;
    return;
  }
  auto iter = unpacker_index_.find(layer);
  if (iter == unpacker_index_.end()) {
    std::cerr << "Layer " << layer << " not found." << std::endl;
    return;
  }
  iter->second->MapDataPacketWalker(layer, walker);
}

/**
 * Find the names of all files in each layer of a globe.
 */
bool GlcUnpacker::MapFileWalker(const map_file_walker& walker)
{
    for( auto& index_item : unpacker_index_) {
        if (MapFileWalker(index_item.first, walker)) {
          return true;
        }
    }
    return false;
}

bool GlcUnpacker::MapFileWalker(int layer, const map_file_walker& walker)
{
  FileUnpacker* unpacker = GetUnpacker(layer);

  for (int i = 0; i < unpacker->IndexSize(); ++i) {
    const char *package_file = unpacker->IndexFile(i);

    if (walker(layer, package_file) != true) {
      return true;
    }
  }
  return false;
}

/**
 * Find qtp packet and set offset and size for the packet. Qtp packets can
 * be quadtree packets or a dbroot packet.
 */
bool GlcUnpacker::FindQtpPacket(const char* qtpath,
                                int packet_type,
                                int channel,
                                int layer,
                                PackageFileLoc* data_loc) {
  if (!is_gee_) {
    std::cerr << "Not a GEE file." << std::endl;
    return false;
  }

  FileUnpacker* unpacker;
  while (true) {
    unpacker = GetUnpacker(layer);
    if (unpacker) {
      if (unpacker->FindQtpPacket(qtpath, packet_type, channel, data_loc)) {
        return true;
      } else {
        layer = parent_layer_[layer];
        if (layer == 0) {
          return false;
        }
      }
    } else {
      return false;
    }
  }
}

/**
 * Find max LOD where there is data of some kind at the given address.
 * Does not look at resolutions higher than the given address so
 * possible values are 0 to len(qtpath) - 1. As for all other qtpaths,
 * expects leading "0".
 */
int GlcUnpacker::MaxResolutionLod(const char* qtpath, bool is_2d) {
  std::string new_qtpath(qtpath);
  size_t len = new_qtpath.size();
  int lod = len - 1;
  for (size_t i = 1; i < len; ++i) {
    if (HasData(new_qtpath.c_str(), is_2d)) {
      return lod;
    }
    new_qtpath = new_qtpath.substr(0, lod);
    --lod;
  }

  return lod;
}

/**
 * Returns whether there are any data at the given address.
 */
bool GlcUnpacker::HasData(const char* qtpath, bool is_2d) {
  std::map<int, FileUnpacker*>::iterator it;
  for (it = unpacker_index_.begin(); it != unpacker_index_.end(); ++it) {
    int layer_id = it->first;

    // Skip 2d if we are looking for 3d data.
    if (layer_type_[layer_id] == MAP_LAYER_TYPE) {
      if (!is_2d) {
        continue;
      }
    // Skip 3d if we are looking for 2d data.
    } else {
      if (is_2d) {
        continue;
      }
    }

    FileUnpacker* unpacker = GetUnpacker(layer_id);
    if (unpacker->HasData(qtpath, is_2d)) {
      return true;
    }
  }

  return false;
}

/**
 * Find layer file and set the offset and size for retrieving the file from the
 * package.
 */
bool GlcUnpacker::FindLayerFile(const char* file_name,
                                int layer,
                                PackageFileLoc* file_loc) {
  FileUnpacker* unpacker = GetUnpacker(layer);
  if (unpacker) {
    return unpacker->FindFile(file_name, file_loc);
  } else {
    return false;
  }
}

/**
 * Find file and set the offset and size for retrieving the file from the
 * package.
 */
bool GlcUnpacker::FindFile(const char* file_name,
                           PackageFileLoc* file_loc) const {
  std::map<std::string, PackageFileLoc>::const_iterator it;
  it = index_.find(file_name);
  if (it != index_.end()) {
    file_loc->Set(it->second.Offset(), it->second.Size());
    return true;
  } else {
    std::cerr << "Unable to find: " << file_name << std::endl;
    file_loc->Set(0, 0);
    return false;
  }
}

/**
 * Find meta dbroot for the composite globe.
 */
bool GlcUnpacker::FindMetaDbRoot(PackageFileLoc* data_loc) {
  if (!meta_dbroot_path_.empty()) {
    return FindFile(meta_dbroot_path_.c_str(), data_loc);
  }

  data_loc->Set(0, 0);
  return false;
}

/**
 * Find dbroot for the composite globe.
 */
bool GlcUnpacker::FindDbRoot(PackageFileLoc* data_loc) {
  if (is_composite_) {
    return FindMetaDbRoot(data_loc);
  } else if (is_3d_) {
    assert(unpacker_index_[0]);
    return unpacker_index_[0]->FindDbRoot(data_loc);
  }

  data_loc->Set(0, 0);
  return false;
}

/**
 * Get idx-th file in index. Assumes iteration order is stable if
 * index is unchanged.
 */
const char* GlcUnpacker::IndexFile(int idx) const {
  std::map<std::string, PackageFileLoc>::const_iterator it;
  it = index_.begin();
  for (int i = 0; (i < idx) && (it != index_.end()); ++i, ++it) ;

  if (it != index_.end()) {
    return it->first.c_str();
  }

  return "";
}

/**
 * Get the unpacker associated with the given layer.  Returns null
 * if it is not found.
 */
FileUnpacker* GlcUnpacker::GetUnpacker(int layer) {
  std::map<int, FileUnpacker*>::iterator it = unpacker_index_.find(layer);
  if (it == unpacker_index_.end()) {
    std::cerr << "No layer " << layer << std::endl;
    return 0;
  }

  return it->second;
}

/**
 * Get number of layers in current globe or map.
 */
int GlcUnpacker::NumberOfLayers() {
  return unpacker_index_.size();
}

/**
 * Get nth layer's index.
 */
int GlcUnpacker::LayerIndex(int n) {
  if ((n < 0) || (n >= static_cast<int>(unpacker_index_.size()))) {
    return -1;
  }

  std::map<int, FileUnpacker*>::iterator it = unpacker_index_.begin();
  for (int j = 0; j < n; ++j, ++it) ;
  return it->first;
}

/**
 * Extracts the first timestamp from the info file, which should
 * be a GMT timestamp. Assumes date begins with 20 (fails in 2100).
 * The beginning of the info file is quite structured so this
 * should be reliable. It starts with 3 lines similar to the
 * following:
 *   Portable Globe
 *   2012-02-24 01:26:07 GMT
 */
std::string GlcUnpacker::ExtractDateFromInfo() {
  size_t idx = info_.find("\n20");
  return info_.substr(idx + 1, 4)
       + info_.substr(idx + 6, 2)
       + info_.substr(idx + 9, 2)
       + info_.substr(idx + 12, 2)
       + info_.substr(idx + 15, 2)
       + info_.substr(idx + 18, 2);
}

/**
 * Calculates a crc for the info data.
 */
 std::uint32_t GlcUnpacker::InfoCrc() {
  std::uint32_t crc = 0;
  unsigned char* ptr = reinterpret_cast<unsigned char*>(&info_[0]);
  for (std::uint32_t i = 0; i < info_.size(); ++i) {
    // Use the full bit space by shifting to one of the
    // four byte boundaries.
    std::uint32_t value = *ptr++ << ((i & 3) * 8);
    crc += value;
  }
  return crc;
}

/**
 * Calculate the id associated with this file. Assumes that the bundle
 * contains a file called "earth/info.txt" which uniquely identifies
 * the file. If not, it returns "noinfo-<file_size>".
 */
const char* GlcUnpacker::Id() {
  std::stringstream sstream;
  std::string info;
  if (id_.empty()) {
    if (info_.empty()) {
      sstream << "noinfo";
    } else {
      sstream << ExtractDateFromInfo();
      sstream << "-" << std::hex << InfoCrc();
    }
    sstream << "-" << std::hex << length_;
    id_ = sstream.str();
  }

  return id_.c_str();
}

std::map<int, std::string> GlcUnpacker::getKmlData() {
  std::map<int, std::string> kmlData;

  for (auto &mapLayer : unpacker_index_) {
    FileUnpacker *layerFile = mapLayer.second;
    auto layerId = mapLayer.first;

    for (int i = 0; i < layerFile->IndexSize(); ++i) {
      auto package_file = layerFile->IndexFile(i);

      if (strcmp(package_file, "earth/polygon.kml")) {
        continue;
      }

      PackageFileLoc file_loc;
      if (layerFile->FindFile(package_file, &file_loc)) {
        auto& kmlDataStr = kmlData[layerId];
        if (!glc_reader_.Read(&kmlDataStr, file_loc.Offset(), file_loc.Size())) {
            std::cerr << "Unable to read kml file data." << std::endl;
        }
      } else {
          std::cerr << "Unable to to position to the earth/polygon.kml file data." << std::endl;
      }

      break;
    }
  }

  return kmlData;
}
