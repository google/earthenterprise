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


// Tool for checking a glb, glm or glc package file.
// TODO: Move much functionality to another class (and file) and
// TODO: add tests for that class.

#include <notify.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include <iomanip>
#include <cstdint>
#include "common/etencoder.h"
#include "common/khAbortedException.h"
#include "common/khFileUtils.h"
#include "common/geterrain.h"
#include "common/khEndian.h"
#include "common/khGetopt.h"
#include "common/khSimpleException.h"
#include <cstdint>
#include "common/proto_streaming_imagery.h"
#include "common/packet.h"
#include "common/packetcompress.h"
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/file_package.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/packetbundle.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/portable_glc_reader.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h"

namespace {

// Calculate the x and y coordinates for the given btree (quadtree)
// and level. The btree is a 48-bit representation of up to a
// 24-level address. It is filled from the high bits down. The
// number of meaningful bits is level*2.
void GetXY(int level, std::uint64_t btree, std::uint64_t* x, std::uint64_t* y) {
  int shift = 48 - level * 2;
  *x = 0;
  *y = 0;
  std::uint64_t half_node = 1;
  for (int i = 0; i < level; ++i) {
    // Check next quadnode value (0 - 3).
    int next_node = (btree >> shift) & 3;
    switch (next_node) {
      case 0:
        *y += half_node;
        break;
      case 1:
        *x += half_node;
        *y += half_node;
        break;
      case 2:
        *x += half_node;
        break;
      case 3:
        break;
    }
    half_node <<= 1;  // double the half_node size.
    shift += 2;
  }
}

void writePacketToFile(const IndexItem& index_item,
                       const std::string& buffer,
                       const bool skipSmall,
                       const std::string& writeDir,
                       const std::string& extraSuffix) {
  char path[256];
  const char* suffix;
  std::uint64_t x;
  std::uint64_t y;
  std::uint64_t btree = index_item.btree_high;
  btree <<= 16;
  btree |= (index_item.btree_low & 0xffff);
  GetXY(index_item.level, btree, &x, &y);
  if ((buffer[6] == 'J') && (buffer[7] == 'F')
      && (buffer[8] == 'I') && (buffer[9] == 'F')) {
    suffix = "jpg";
  } else if ((buffer[1] == 'P') && (buffer[2] == 'N')
             && (buffer[3] == 'G')) {
    suffix = "png";
  } else {
    suffix = "unk";
  }
  snprintf(path, sizeof(path), "%s/%d/%d/%lu/%lu%s.%s",
           writeDir.c_str(), index_item.channel, index_item.level, x, y, extraSuffix.c_str(), suffix);
  if (skipSmall && buffer.size() <= 153) {
    std::cout << "Skipping small: " << path << std::endl;
  } else {
    khEnsureParentDir(path);
    khWriteSimpleFile(path, &buffer[0], buffer.size());
  }
}

bool getPackageFileLocs(GlcUnpacker* const unpacker,
                    const std::string& index_file,
                    const std::string& data_file,
                    bool is_composite,
                    int layer_idx,
                    PackageFileLoc& index_file_loc,
                    PackageFileLoc& data_file_loc) {
  // For glcs, we need to look inside each layer for the indexes.
  if (is_composite) {
    if (unpacker->Is2d()) {
      std::cout << "2d glc" << std::endl;
    }
    if (unpacker->Is3d()) {
      std::cout << "3d glc" << std::endl;
    }

    int layer_index = unpacker->LayerIndex(layer_idx);
    if (!unpacker->FindLayerFile(
           index_file.c_str(), layer_index, &index_file_loc)) {
       std::cout << "Unable to find layer index: " << index_file << std::endl;
       return false;
    }

    if (!unpacker->FindLayerFile(
           data_file.c_str(), layer_index, &data_file_loc)) {
       std::cout << "Unable to find layer data: " << data_file << std::endl;
       return false;
    }

  // For glms and glbs, just use the corresponding data index and divide
  // by its size (sizeof(IndexItem)).
  } else {
    if (!unpacker->FindFile(index_file.c_str(), &index_file_loc)) {
        std::cout << "Unable to find index: " << index_file << std::endl;
      return false;
    }
    if (!unpacker->FindFile(data_file.c_str(), &data_file_loc)) {
        std::cout << "Unable to find data: " << data_file << std::endl;
      return false;
    }
  }

  return true;
}

void printTerrainPacket(LittleEndianReadBuffer& buffer, std::ostringstream& s) {

  geterrain::Mesh m;

  int skip = 0;
  int count = 0;
  while ((buffer.CurrPos() < buffer.size()) || skip > 0) {
    count++;

    if (count != 1) {
      s << std::endl << std::endl;
    }

    s << "mesh " << count << " of 20:" << std::endl;

    if (skip > 0) {
      skip--;
      s << "Skipped because of previous empty mesh header." << std::endl;
    } else {
      m.Pull(buffer);
      if (m.source_size() > 0) {
        m.PrintMesh(s);
      } else {
        skip = 3;
        s << "EMPTY MESH HEADER -- Skipping next "
          << skip << " mesh entries." << std::endl;
      }
    }
  }

  s << std::endl;
}

void printVectorPacketBitFlags(const ushort bitFlags, std::ostringstream& s) {
  s << "      bitFlags: 0x" << std::hex << bitFlags << std::dec << " (";
  
  if ((bitFlags & 0x01) == 0x01) {
    s << "Relative";
  } else if ((bitFlags & 0x02) == 0x02) {
    s << "Absolute";
  } else {
    s << "Clamp to ground";
  }
  if ((bitFlags & 0x04) == 0x04) {
    s << ", Extrude";
  }
  s << ")" << std::endl;
}

void printVectorPacket(const LittleEndianReadBuffer& buffer, std::ostringstream& s) {

  const std::map<std::uint32_t, std::string> vTypeNames = {
    {TYPE_STREETPACKET, "TYPE_STREETPACKET"},
    {TYPE_SITEPACKET, "TYPE_SITEPACKET"},
    {TYPE_DRAWABLEPACKET, "TYPE_DRAWABLEPACKET"},
    {TYPE_POLYLINEPACKET, "TYPE_POLYLINEPACKET"},
    {TYPE_AREAPACKET, "TYPE_AREAPACKET"},
    {TYPE_STREETPACKET_UTF8, "TYPE_STREETPACKET_UTF8"},
    {TYPE_SITEPACKET_UTF8, "TYPE_SITEPACKET_UTF8"},
    {TYPE_LANDMARK, "TYPE_LANDMARK"},
    {TYPE_POLYGONPACKET, "TYPE_POLYGONPACKET"}
  };

  etDrawablePacket drawablePacket;
  drawablePacket.load((char*)buffer.data(), (int)buffer.size());

  const char *pPacketDataBuffer = buffer.data() + drawablePacket.packetHeader.dataBufferOffset;

  const int floatPrecision = 12;

  const auto typeNameIter = vTypeNames.find(drawablePacket.packetHeader.dataTypeID);
  std::string vectorDataType = "UNKNOWN";
  if (typeNameIter != vTypeNames.end()) {
    vectorDataType = typeNameIter->second;
  }

  s << "etDrawablePacket.packetHeader fields:" << std::endl;
  s << "  dataBufferOffset = " << drawablePacket.packetHeader.dataBufferOffset << std::endl;
  s << "  dataBufferSize = " << drawablePacket.packetHeader.dataBufferSize << std::endl;
  s << "  dataInstanceSize = " << drawablePacket.packetHeader.dataInstanceSize << std::endl;
  s << "  dataTypeID = " << drawablePacket.packetHeader.dataTypeID << " -- " << vectorDataType << std::endl;
  s << "  magicID = " << drawablePacket.packetHeader.magicID 
    << " (0x" << std::hex << drawablePacket.packetHeader.magicID << std::dec << ")" << std::endl;
  s << "  metaBufferSize = " << drawablePacket.packetHeader.metaBufferSize << std::endl;
  s << "  numInstances = " << drawablePacket.packetHeader.numInstances << std::endl;
  s << "  version = " << drawablePacket.packetHeader.version << std::endl;
  s << std::endl;

  for(std::uint32_t idx = 0; idx < drawablePacket.packetHeader.numInstances; idx++){
    const etDataPacket *pChildPacket = drawablePacket.getPtr(idx);
    const char *pChildPacketDataBuffer = pPacketDataBuffer +
                                pChildPacket->packetHeader.dataBufferOffset +
                                pChildPacket->dataBuffer_OFFSET;

    s << "ChildDataPacket " << idx << ":" << std::endl;
    s << "  packetHeader.numInstances = " << pChildPacket->packetHeader.numInstances << std::endl;

    for(std::uint32_t instance_idx = 0; instance_idx < pChildPacket->packetHeader.numInstances; instance_idx++){

      s << "    Instance " << instance_idx << ":" << std::endl;

      const auto typeNameIter = vTypeNames.find(pChildPacket->packetHeader.dataTypeID);
      std::string vectorDataType = "unknown";
      if (typeNameIter != vTypeNames.end()) {
        vectorDataType = typeNameIter->second;
      }

      s << "      packetHeader.dataTypeID: " << vectorDataType << std::endl;

      const char* pVectorPacketDataLocation = pPacketDataBuffer 
            + pChildPacket->packetHeader.dataBufferOffset 
            + pChildPacket->dataInstances_OFFSET 
            + pChildPacket->packetHeader.dataInstanceSize*instance_idx;

      if (pChildPacket->packetHeader.dataTypeID == TYPE_STREETPACKET_UTF8) {
        const etStreetPacketData *pVectorPacketData =
          reinterpret_cast<const etStreetPacketData*>(pVectorPacketDataLocation);

        const char *name_str = 
          pChildPacketDataBuffer + pVectorPacketData->name_OFFSET + sizeof(etPattern);

        s << "      name_str: " << name_str << std::endl;
        s << "      style: " << pVectorPacketData->style << std::endl;
        s << "      numPt: " << pVectorPacketData->numPt << std::endl;
        printVectorPacketBitFlags(pVectorPacketData->bitFlags, s);

        const etVec3d* pPoints = reinterpret_cast<const etVec3d*>
          (pChildPacketDataBuffer + pVectorPacketData->localPt_OFFSET);

        s << "      points: (longitude * 180, latitude * 180, altitude (raw))" << std::endl;
        s << std::setprecision(floatPrecision);

        for (int a = 0; a < pVectorPacketData->numPt; a++, pPoints++) {
          s << "        " << pPoints->elem[0] * 180.0
            << ", " << pPoints->elem[1] * 180.0
            << ", " << pPoints->elem[2] 
            << std::endl;
        }
      } else if (pChildPacket->packetHeader.dataTypeID == TYPE_POLYLINEPACKET) {
        const etPolyLinePacketData *pVectorPacketData =
          reinterpret_cast<const etPolyLinePacketData*>(pVectorPacketDataLocation);

        const char *name_str = 
          pChildPacketDataBuffer + pVectorPacketData->name_OFFSET + sizeof(etPattern);

        s << "      name_str: " << name_str << std::endl;
        s << "      style: " << pVectorPacketData->style << std::endl;
        s << "      numPt: " << pVectorPacketData->numPt << std::endl;
        printVectorPacketBitFlags(pVectorPacketData->bitFlags, s);

        const etVec3d* pPoints = reinterpret_cast<const etVec3d*>
          (pChildPacketDataBuffer + pVectorPacketData->localPt_OFFSET);

        s << "      points: (longitude * 180, latitude * 180, altitude (raw))" << std::endl;
        s << std::setprecision(floatPrecision);

        for (int a = 0; a < pVectorPacketData->numPt; a++, pPoints++) {
          s << "        " << pPoints->elem[0] * 180.0
            << ", " << pPoints->elem[1] * 180.0
            << ", " << pPoints->elem[2] 
            << std::endl;
        }
      } else if (pChildPacket->packetHeader.dataTypeID == TYPE_POLYGONPACKET){
        const etPolygonPacketData *pVectorPacketData =
          reinterpret_cast<const etPolygonPacketData*>(pVectorPacketDataLocation);

        const char *name_str = 
          pChildPacketDataBuffer + pVectorPacketData->name_OFFSET + sizeof(etPattern);

        s << "      name_str: " << name_str << std::endl;
        s << "      style: " << pVectorPacketData->style << std::endl;
        s << "      numPt: " << pVectorPacketData->numPt << std::endl;
        s << "      numEdgeFlags: " << pVectorPacketData->numEdgeFlags << std::endl;
        printVectorPacketBitFlags(pVectorPacketData->bitFlags, s);

        const etVec3d* pPoints = reinterpret_cast<const etVec3d*>
          (pChildPacketDataBuffer + pVectorPacketData->localPt_OFFSET);
        const bool* pEdgeFlags = reinterpret_cast<const bool*>
          (pChildPacketDataBuffer + pVectorPacketData->edgeFlags_OFFSET);

        s << "      points: (longitude * 180, latitude * 180, altitude (raw), edgeFlag)" << std::endl;
        s << std::setprecision(floatPrecision);

        if (pVectorPacketData->numPt == pVectorPacketData->numEdgeFlags) {
          for (int a = 0; a < pVectorPacketData->numPt; a++, pPoints++, pEdgeFlags++) {
            s << "        " << pPoints->elem[0] * 180.0
              << ", " << pPoints->elem[1] * 180.0
              << ", " << pPoints->elem[2]
              << ", " << ((*pEdgeFlags) ? "true" : "false") 
              << std::endl;
          }
        } else {
          s << "NOT ENUMERATING BECAUSE NUMBER OF POINTS != NUMBER OF EDGE FLAGS" << std::endl;
        }
      } else if (pChildPacket->packetHeader.dataTypeID == TYPE_LANDMARK) {
        const etLandmarkPacketData *pVectorPacketData =
          reinterpret_cast<const etLandmarkPacketData*>(pVectorPacketDataLocation);

        const char *name_str =
          pChildPacketDataBuffer + pVectorPacketData->name_OFFSET + sizeof(etPattern);
        const char *desc_str =
          pChildPacketDataBuffer + pVectorPacketData->description_OFFSET;

        s << "      name_str: " << name_str << std::endl;
        s << "      desc_str: " << desc_str << std::endl;
        s << "      style: " << pVectorPacketData->style << std::endl;
        s << "      numPt: " << pVectorPacketData->numPt << std::endl;
        printVectorPacketBitFlags(pVectorPacketData->bitFlags, s);

        const etVec3d *vecPoint = 
          reinterpret_cast<const etVec3d*>(
            pChildPacketDataBuffer + pVectorPacketData->localPt_OFFSET);

        s << "      point:" << std::endl;
        s << std::setprecision(floatPrecision);
        s << "        longitude * 180: " << vecPoint->elem[0] * 180.0 << std::endl;
        s << "        latitude * 180: " << vecPoint->elem[1] * 180.0 << std::endl;
        s << "        altitude (raw): " << vecPoint->elem[2] << std::endl;
      } else {
        s << "      NEED TO HANDLE THIS TYPE" << std::endl;
      }

      s << std::endl;
    }
  }
}

void extractAllPackets(GlcUnpacker* const unpacker,
                    PortableGlcReader* const reader,
                    std::uint64_t start_idx,
                    std::uint64_t end_idx,
                    int layer_idx,
                    bool is_composite,
                    const std::string& index_file,
                    const std::string& data_file) {
  PackageFileLoc index_file_loc;
  PackageFileLoc data_file_loc;

  if (!getPackageFileLocs(unpacker, index_file, data_file, is_composite, layer_idx,
                          index_file_loc, data_file_loc)) {
    return;
  }

  // Sanity check.
  if (index_file_loc.Size() % sizeof(IndexItem) != 0) {
    std::cout << "Index is damaged." << std::endl;
    return;
  }

  std::uint64_t number_of_packets = index_file_loc.Size() / sizeof(IndexItem);
  std::cout << "Extracting " << (end_idx - start_idx)
            << " of " << number_of_packets
            << " packets" << std::endl;

  if (number_of_packets  < end_idx) {
    std::cout << "Insufficient packets. " <<
        "Resetting end packet to: " << number_of_packets << std::endl;
    end_idx = number_of_packets;
  }

  std::uint64_t offset = index_file_loc.Offset() + sizeof(IndexItem) * start_idx;
  std::string buffer;
  std::uint64_t max_size = 200000;
  buffer.resize(max_size);
  // Main extraction loop.
  // Reads sequential index entries and saves packets as files
  // to disk. Directories are arranged in z, x, y order and the
  // name of the file corresponds to the channel.
  for (std::uint64_t i = start_idx; i < end_idx; ++i) {
    IndexItem index_item;
    // std::cout << "index offset: " << offset << std::endl;
    if (reader->ReadData(&index_item, offset, sizeof(IndexItem))) {
      std::uint64_t data_offset = data_file_loc.Offset() + index_item.offset;
      buffer.resize(MIN(max_size, index_item.packet_size));
      if (index_item.packet_size >= max_size) {
        std::cout << "Data item is too big: " << i
                  << " size: " << index_item.packet_size << std::endl;
      } else if (reader->ReadData(
          &buffer[0], data_offset, index_item.packet_size)) {
        if (unpacker->Is2d()) {
          writePacketToFile(index_item, buffer, true, "maptiles", "");
        }
        else if (index_item.packet_type == kImagePacket) {
          etEncoder::DecodeWithDefaultKey(&buffer[0], index_item.packet_size);
          geEarthImageryPacket protoPacket;
          if (!protoPacket.ParseFromString(buffer)) {
            std::cout << "Error parsing protobuf imagery packet" << std::endl;
          }

          if (protoPacket.HasImageData()) {
            writePacketToFile(index_item, protoPacket.ImageData(), false, "globetiles", "_img");
          }
          if (protoPacket.HasImageAlpha()) {
            writePacketToFile(index_item, protoPacket.ImageAlpha(), false, "globetiles", "_alpha");
          }
        }
        else if (index_item.packet_type == kQtpPacket) {
          LittleEndianReadBuffer decompressed;
          etEncoder::DecodeWithDefaultKey(&buffer[0],
                                          buffer.size());
          if (KhPktDecompress(buffer.data(),
                              buffer.size(),
                              &decompressed)) {
            qtpacket::KhQuadTreePacket16 theMetadata;
            decompressed >> theMetadata;
            writePacketToFile(index_item, theMetadata.ToString(index_item.level == 0,true), false, "globetiles", "_meta");
          }
        }
        else if (index_item.packet_type == kTerrainPacket) {
          LittleEndianReadBuffer decompressed;
          etEncoder::DecodeWithDefaultKey(&buffer[0], buffer.size());
          if (KhPktDecompress(buffer.data(), buffer.size(), &decompressed)) {
            std::ostringstream s;
            printTerrainPacket(decompressed, s);
            writePacketToFile(index_item, s.str(), false, "globetiles", "_terrain");
          }
        }
        else if (index_item.packet_type == kDbRootPacket) {
          writePacketToFile(index_item, buffer, false, "globetiles", "_dbroot");
        }
        else if (index_item.packet_type == kVectorPacket) {
          LittleEndianReadBuffer decompressed;
          etEncoder::DecodeWithDefaultKey(&buffer[0], buffer.size());
          if (KhPktDecompress(buffer.data(), buffer.size(), &decompressed) &&
             (decompressed.size() >= sizeof(std::uint32_t)*2)) {
            std::ostringstream s;
            printVectorPacket(decompressed, s);
            writePacketToFile(index_item, s.str(), false, "globetiles", "_vector");
          }
        }
        else {
          std::cout << "Found unhandled packet type of " << int(index_item.packet_type) << std::endl;
        }
      }  else {
        std::cout << "Unable to read data item." << std::endl;
      }
    } else {
      std::cout << "Unable to read index item." << std::endl;
    }
    offset += sizeof(IndexItem);
  }
}

// Extract tiles for all channels from glm or 2d glc.
// Begin with the start_idx-th entry in the index and continue
// until the last index entry before end_idx-th entry.
// All channels are extracted from the glm, which may be
// the file itself or may be a layer within a glc.
// Tiles are saved to directory tree:
//    maptiles/<z>/<x>/<y>/<channel>.<sfx>
// Where sfx indicates the tile type (jpg or png).
void ExtractPackets(GlcUnpacker* const unpacker,
                    PortableGlcReader* const reader,
                    const std::string& suffix,
                    bool is_composite,
                    int layer_idx,
                    std::uint64_t start_idx,
                    std::uint64_t end_idx) {
  if (unpacker->Is3d()) {
    std::cout << "Extracting packets for a globe file" << std::endl;
    extractAllPackets(unpacker, reader, start_idx, end_idx, layer_idx, is_composite, "data/index", "data/pbundle_0000");
    extractAllPackets(unpacker, reader, start_idx, end_idx, layer_idx, is_composite, "qtp/index", "qtp/pbundle_0000");
    return;
  }
  else {
    extractAllPackets(unpacker, reader, start_idx, end_idx, layer_idx, is_composite, "mapdata/index", "mapdata/pbundle_0000");
  }
}


void NumberOfPackets(
    GlcUnpacker* const unpacker, const std::string& suffix, bool is_composite) {
  PackageFileLoc file_loc;
  // For glcs, we need to look inside each layer for the indexes.
  if (is_composite) {
    // Determine if the glc is 2d, 3d, or both
    bool is_2d = false;
    bool is_3d = false;
    for (int i = 0; i < unpacker->IndexSize(); ++i) {
      std::string package_file = unpacker->IndexFile(i);
      std::string layer_suffix = package_file.substr(package_file.size() - 3);
      if (layer_suffix == "glb") {
        is_3d = true;
      } else if (layer_suffix == "glm") {
        is_2d = true;
      }
    }
    if (is_2d) {
      std::cout << "2d glc" << std::endl;
    }
    if (is_3d) {
      std::cout << "3d glc" << std::endl;
    }

    // For each layer, add the number of packets in it.
    std::cout << "Number of layers: "
        << unpacker->NumberOfLayers() << std::endl;
    int number_of_packets = 0;
    for (int i = 0; i < unpacker->NumberOfLayers(); ++i) {
      int layer_index = unpacker->LayerIndex(i);
      if (is_3d &&
          unpacker->FindLayerFile("data/index", layer_index, &file_loc)) {
        if (file_loc.Size() % sizeof(IndexItem) != 0) {
          std::cout << "Index is damaged." << std::endl;
        } else {
          int number_of_layer_packets = file_loc.Size() / sizeof(IndexItem);
          std::cout << "layer " << i << ": " << number_of_layer_packets <<
                       " packets" << std::endl;
          number_of_packets += number_of_layer_packets;
        }
      }
      if (is_2d &&
          unpacker->FindLayerFile("mapdata/index", layer_index, &file_loc)) {
        if (file_loc.Size() % sizeof(IndexItem) != 0) {
          std::cout << "Index is damaged." << std::endl;
        } else {
          int number_of_layer_packets = file_loc.Size() / sizeof(IndexItem);
          std::cout << "layer " << i << ": " << number_of_layer_packets <<
                       " packets" << std::endl;
          number_of_packets += number_of_layer_packets;
        }
      }
    }
    std::cout << number_of_packets << " packets" << std::endl;

  // For glms and glbs, just use the corresponding data index and divide
  // by its size (sizeof(IndexItem)).
  } else {
    std::string index_file;
    if (suffix == "glm") {
      index_file = "mapdata/index";
    } else {
      index_file = "data/index";
    }
    if (unpacker->FindFile(index_file.c_str(), &file_loc)) {
      if (file_loc.Size() % sizeof(IndexItem) != 0) {
        std::cout << "Index is damaged." << std::endl;
      } else {
        std::cout << (file_loc.Size() / sizeof(IndexItem))
            << " packets" << std::endl;
      }
    } else {
      std::cout << "Unable to find: " << index_file << std::endl;
    }
  }
}

void ListFiles(GlcUnpacker* const unpacker) {
  std::cout << "Index has " << unpacker->IndexSize() << " files." << std::endl;
  for (int i = 0; i < unpacker->IndexSize(); ++i) {
    const char* package_file = unpacker->IndexFile(i);
    PackageFileLoc file_loc;
    if (unpacker->FindFile(package_file, &file_loc)) {
       std::cout << i << ": " << package_file << std::endl;
       std::cout << "  offset: " << file_loc.Offset() << std::endl;
       std::cout << "    size: " << file_loc.Size() << std::endl;
    } else {
      std::cout << "Unable to find: " << package_file << std::endl;
    }
  }
}

// Extracts package file specified by (offset, size) from glx_file and
// writes to out_file.
void ExtractFile(const std::string& out_file,
                 const std::string& glx_file,
                 std::uint64_t offset,
                 std::uint64_t size) {
  if (out_file.empty()) {
    notify(NFY_WARN, "No file specified for output.");
    return;
  }

  std::ifstream fp_in(glx_file.c_str(), std::ios::in | std::ios::binary);
  std::ofstream fp_out(out_file.c_str(), std::ios::out | std::ios::binary);

  // Copy data from package to file.
  fp_in.seekg(offset, std::ios::beg);
  for (std::uint64_t i = 0; i < size; ++i) {
    fp_out.put(fp_in.get());
  }

  fp_in.close();
  fp_out.close();
}

// Extracts all package files from glx_file and writes to dest_dir.
void ExtractAllFiles(
    GlcUnpacker* const unpacker,
    const std::string& glx_file,
    const std::string& dest_dir) {
  for (int i = 0; i < unpacker->IndexSize(); ++i) {
    const char* package_file = unpacker->IndexFile(i);
    PackageFileLoc file_loc;
    if (unpacker->FindFile(package_file, &file_loc)) {
       std::string dest_file = khComposePath(dest_dir, package_file);
       std::cout << "Extracting " << package_file
                 << " to " << dest_file << std::endl;

       // Make parent directories as needed.
       if (!khEnsureParentDir(dest_file, 0750)) {
         notify(NFY_WARN, "Couldn't create parent directories for: %s",
                dest_file.c_str());
         return;
       }

       ExtractFile(dest_file, glx_file, file_loc.Offset(), file_loc.Size());
    } else {
      std::cout << "Unable to find: " << package_file << std::endl;
    }
  }
}

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "E.g. geglobefiletool \\\n"
          "        --glx /tmp/my_globe.glb \\\n"
          "        --list_files \n"
          "\nusage: %s\n"
          "   --glx <glb_file_path>: The glb, glm or glc file being\n"
          "                          analyzed.\n"
          "\n"
          " Tool for checking a globe file.\n"
          "\n"
          "\nOptions:\n"
          "   --list_files:    List all of the files in the glx.\n"
          "   --id:            Get id that identifies the glx file.\n"
          "   --is_gee:        Does glx appear to be a valid globe or map.\n"
          "   --check_crc:     Check crc of the glx.\n"
          "   --extract_file <relative_file_path>: File to be extracted from\n"
          "                    the glx.\n"
          "   --extract_all_files: Extract all files from the glx.\n"
          "   --number_of_packets: Output number of data packets in the glx.\n"
          "   --extract_packet <quadtree_address>: Extract packet at given\n"
          "                    quadtree address (e.g. '310').\n"
          "   --packet_type <type_string>: Type of packet to extract\n"
          "                    ('dbroot', 'qtp', 'img', 'ter', or 'vec').\n"
          "   --packet_channel <channel_int>: Channel of packet to extract.\n"
          "   --output <dest_{file,dir}_path>: Where extracted file(s) should be\n"
          "                     written.\n"
          "   --extract_packets Extract all packets from a portable file.\n"
          "                     Can be used with start_idx and end_idx\n"
          "                     parameters, and the layer_idx parameter\n"
          "                     if it's a glc.\n"
          "   --start_idx       If extracting all packets, the index item\n"
          "                     to begin at. Default: 0\n"
          "   --end_idx         If extracting all packets, the index item\n"
          "                     to end before (non-incusive). Default: MAXINT\n"
          "   --layer_idx       If extracting all packets from a 2d glc, the\n"
          "                     layer to extract. Default: 0\n",
          progn.c_str());
  exit(1);
}

}   // unnamed namespace

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    bool list_files = false;
    bool number_of_packets = false;
    bool id = false;
    bool check_crc = false;
    bool is_gee = false;
    bool clip_file = false;
    bool extract_all_files = false;
    bool extract_packets = false;
    std::string glx_file;
    std::string extract_file;
    std::string extract_packet;
    std::string packet_type;
    std::string packet_channel;
    std::string output;
    std::string layer_idx_str;
    std::string start_idx_str;
    std::string end_idx_str;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("list_files", list_files);
    options.flagOpt("number_of_packets", number_of_packets);
    options.flagOpt("extract_all_files", extract_all_files);
    options.flagOpt("check_crc", check_crc);
    options.flagOpt("id", id);
    options.flagOpt("is_gee", is_gee);
    options.flagOpt("clip_file", clip_file);
    options.flagOpt("extract_packets", extract_packets);
    options.opt("glx", glx_file);
    options.opt("extract_file", extract_file);
    options.opt("extract_packet", extract_packet);
    options.opt("packet_type", packet_type);
    options.opt("packet_channel", packet_channel);
    options.opt("output", output);
    options.opt("layer_idx_str", layer_idx_str);
    options.opt("start_idx_str", start_idx_str);
    options.opt("end_idx_str", end_idx_str);
    options.setRequired("glx");
    // TODO: Allow layer to be set via a flag.
    int layer = 0;

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }


    assert(!glx_file.empty());
    bool is_composite;
    std::string suffix = glx_file.substr(glx_file.size() - 3);
    is_composite = (suffix == "glc");
    PortableGlcReader reader(glx_file.c_str());
    GlcUnpacker unpacker(reader, is_composite, false);

    // List all of the packed files.
    if (list_files) {
      ListFiles(&unpacker);
    }

    // Output number of data packets.
    if (number_of_packets) {
      NumberOfPackets(&unpacker, suffix, is_composite);
    }

    // Show whether it is a valid glx.
    if (is_gee) {
      std::cout << "IsGee: " << unpacker.IsGee() << std::endl;
    }

    // Check crc.
    if (check_crc) {
      std::uint32_t read_crc = Package::ReadCrc(reader);
      std::uint32_t calculated_crc = Package::CalculateCrc(reader);
      std::cout.flags(std::ios::right | std::ios::hex | std::ios::showbase);
      std::cout << "\nFile crc: " << read_crc<< std::endl;
      std::cout << "Calculated crc :" << calculated_crc << std::endl;
      if (read_crc == calculated_crc) {
        std::cout << "Good crc!" << std::endl;
      } else {
        std::cout << "Bad crc." << std::endl;
      }
    }

    // Get id.
    if (id) {
      std::cout << "Id: " << unpacker.Id() << std::endl;
    }

    // Extract a packed file.
    if (!extract_file.empty()) {
      PackageFileLoc file_loc;
      if (unpacker.FindFile(extract_file.c_str(), &file_loc)) {
        ExtractFile(output, glx_file, file_loc.Offset(), file_loc.Size());
      }
    }

    // Extract all packed files.
    if (extract_all_files) {
      ExtractAllFiles(&unpacker, glx_file, output);
    }

    // Extract all packed files.
    if (extract_packets) {
      int layer_idx = 1000;
      if (!layer_idx_str.empty()) {
        sscanf(layer_idx_str.c_str(), "%d", &layer_idx);
      }
      std::uint64_t start_idx = 0;
      if (!start_idx_str.empty()) {
        sscanf(start_idx_str.c_str(), "%lu", &start_idx);
      }
      std::uint64_t end_idx = 0x3fffffffffffffff;
      if (!end_idx_str.empty()) {
        sscanf(end_idx_str.c_str(), "%lu", &end_idx);
      }
      ExtractPackets(&unpacker,
                     &reader,
                     suffix,
                     is_composite,
                     layer_idx,
                     start_idx,
                     end_idx);
    }

    // Extract a packet.
    if (!extract_packet.empty()) {
      int channel = 0;
      int type = kImagePacket;
      if (packet_type == "img") {
        type = kImagePacket;
        channel = 0;
      } else if (packet_type == "ter") {
        type = kTerrainPacket;
        channel = 1;
      } else if (packet_type == "vec") {
        type = kVectorPacket;
      } else if (packet_type == "qtp") {
        type = kQtpPacket;
        channel = 0;
      } else if (packet_type == "dbroot") {
        type = kDbRootPacket;
        channel = 0;
        extract_packet = "0";
      } else {
        notify(NFY_FATAL, "Unknown type (must be img, ter, qtp or dbroot).");
      }

      if (!packet_channel.empty()) {
        sscanf(packet_channel.c_str(), "%d", &channel);
      }

      // Find the packet.
      PackageFileLoc file_loc;
      if ((type == kImagePacket)
          || (type == kTerrainPacket)
          || (type == kVectorPacket)) {
        if (suffix == "glm") {
          std::cout << "Find 2d map packet: " << std::endl;
          if (unpacker.FindMapDataPacket(
              extract_packet.c_str(), type, channel, layer, &file_loc)) {
            std::cout << "Data packet: " << std::endl;
            std::cout << "  offset: " << file_loc.Offset() << std::endl;
            std::cout << "    size: " << file_loc.Size() << std::endl;
          } else {
            std::cout << "Unable to find packet." << std::endl;
            exit(1);
          }
        } else {
          std::cout << "Find 3d globe packet: " << std::endl;
          if (unpacker.FindDataPacket(
              extract_packet.c_str(), type, channel, layer, &file_loc)) {
            std::cout << "Data packet: " << std::endl;
            std::cout << "  offset: " << file_loc.Offset() << std::endl;
            std::cout << "    size: " << file_loc.Size() << std::endl;
          } else {
            std::cout << "Unable to find packet." << std::endl;
            exit(1);
          }
        }
      } else {
        if (unpacker.FindQtpPacket(
            extract_packet.c_str(), type, channel, layer, &file_loc)) {
          std::cout << "Data packet: " << std::endl;
          std::cout << "  offset: " << file_loc.Offset() << std::endl;
          std::cout << "    size: " << file_loc.Size() << std::endl;
        } else {
          std::cout << "Unable to find packet." << std::endl;
          exit(1);
        }
      }

      ExtractFile(output, glx_file, file_loc.Offset(), file_loc.Size());
    }
  } catch(const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
