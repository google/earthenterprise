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

//
// Decode a qt packet that has been dumped into a file. This is an alternative
// to the geqtpdump which requires a server. This is a little easier in
// that you can use your browser to get through required steps to get
// to the desired qt packet.

#include <fstream>
#include <iostream>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/khConstants.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "common/khGetopt.h"
#include "common/khStringUtils.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/packetcompress.h"
#include "common/qtpacket/quadtreepacket.h"
#include "common/etencoder.h"

/**
 * Reads specified amount of data from the given from file in the specified
 * location.
 * @param file Path to file being read.
 * @param data String where data will be placed.
 * @param start Starting position in file to be read from.
 * @param size Number of bytes to read from the file.
 */
void GetData(
    const std::string& file, std::string* data, size_t start, size_t size) {
  std::ifstream fp(file.c_str(), std::ifstream::binary);
  data->resize(size);

  fp.seekg(start);
  fp.read(&(*data)[0], size);
  fp.close();
}

/**
 * Does a hex dump of data from a data buffer.
 * @param data Data buffer containing data to be shown.
 * @param start Starting position in buffer of data to show.
 * @param size Number of bytes to show.
 */
void ShowData(const std::string& data, size_t start, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    char str[256];
    if ((i & 0x0f) == 0) {
      snprintf(str, sizeof(str), "%04x: ", static_cast<unsigned int>(i));
      std::cout << std::endl << str;
    }

    snprintf(str, sizeof(str), " %02x",
             static_cast<int>(data[start + i]) & 255);
    std::cout << str;
  }

  std::cout << std::endl;
}

/**
 * Returns the file size for the given file.
 * @param file_name Name of file being checked.
 * @return size of the file in bytes.
 */
int FileSize(std::string file_name) {
  std::ifstream fp(file_name.c_str(), std::ifstream::binary);
  if (fp) {
    fp.seekg(0L, std::ios::end);
    int length = fp.tellg();
    fp.close();
    return length;
  }

  return 0;
}

/**
 * Output the proto quadtree packet info as human-readable text.
 * @param qtpacket_file Name of file containing the quadtree packet.
 * @param mask Mask data for decrypting the packet.
 * @param is_root Whether the packet is the root quadtree packet.
 */
void DecodeProtoQtPacket(const std::string& qtpacket_file, bool is_root) {
  std::string qtpacket;
  int qtpacket_size = FileSize(qtpacket_file);
  GetData(qtpacket_file, &qtpacket, 0, qtpacket_size);

  std::cout << "Proto QtPacket:";
  ShowData(qtpacket, 0, qtpacket_size);

  // Decrypt qt packet.
  std::cout << "QtPacket decrypted:";
  etEncoder::DecodeWithDefaultKey(&qtpacket[0], qtpacket_size);
  ShowData(qtpacket, 0, qtpacket_size);

  // Uncompress qt packet.
  LittleEndianReadBuffer buffer;
  std::cout << std::endl;
  std::cout << "QtPacket size:" << qtpacket.size() << std::endl;
  if (!KhPktDecompress(qtpacket.data(), qtpacket.size(), &buffer)) {
    std::cout << "Unable to decompress." << std::endl;
    exit(0);
  }

  // Show qt packet contents.
  std::cout << "Try to create proto packet ..." << std::endl;
  qtpacket::KhQuadTreePacketProtoBuf packet;
  buffer >> packet;
  std::cout << "Created proto packet." << std::endl;
  std::cout << packet.ToString(is_root, true) << std::endl;
}

/**
 * Output the quadtree packet info as human-readable text.
 * @param qtpacket_file Name of file containing the quadtree packet.
 * @param mask Mask data for decrypting the packet.
 * @param is_root Whether the packet is the root quadtree packet.
 */
void DecodeQtPacket(const std::string& qtpacket_file, bool is_root) {
  std::string qtpacket;
  int qtpacket_size = FileSize(qtpacket_file);
  GetData(qtpacket_file, &qtpacket, 0, qtpacket_size);

  std::cout << "QtPacket:";
  ShowData(qtpacket, 0, qtpacket_size);

  // Decrypt qt packet.
  std::cout << "QtPacket decrypted:";
  etEncoder::DecodeWithDefaultKey(&qtpacket[0], qtpacket_size);
  ShowData(qtpacket, 0, qtpacket_size);

  // Uncompress qt packet.
  LittleEndianReadBuffer buffer;
  std::cout << std::endl;
  std::cout << "QtPacket size:" << qtpacket.size() << std::endl;
  if (!KhPktDecompress(qtpacket.data(), qtpacket.size(), &buffer)) {
    std::cout << "Unable to decompress." << std::endl;
    exit(0);
  }

  // Show qt packet contents.
  qtpacket::KhQuadTreePacket16 packet;
  buffer >> packet;

  std::cout << "  QtPacket info:" << std::endl;
  std::cout << "  subnode index (s#)" << std::endl;
  std::cout << "  qt path (\"[0123]*\")" << std::endl;
  std::cout << "  imagery version (iv)" << std::endl;
  std::cout << "  imagery provider (ip)" << std::endl;
  std::cout << "  terrain version (tv)" << std::endl;
  std::cout << "  terrain provider (tp)" << std::endl;
  std::cout << "  cnode version (c)" << std::endl;
  std::cout << "  children (hex and decoded:)" << std::endl;
  std::cout << "    I - image bit set" << std::endl;
  std::cout << "    T - terrain bit set" << std::endl;
  std::cout << "    V - vector (drawable) bit set" << std::endl;
  std::cout << "    C - cache node bit set" << std::endl;
  std::cout << "    0?1?2?3? children nodes" << std::endl;
  std::cout << "  vector layer data" << std::endl;

  std::cout << std::endl;
  std::cout << "QtPacket (" << buffer.size() << " bytes):" << std::endl;
  std::cout << packet.ToString(is_root, true) << std::endl;
}

// Shows legal arguments for the program.
void usage(const std::string &progn) {
  fprintf(stderr,
          "\n"
          "usage: %s [--root] [--proto] <qt_packet_file>\n"
          "\n"
          "  qt_packet_file: quadtree packet grabbed from server.\n"
          "    E.g. using "
          "http://server.name.com/default_ge/flatfile?q2-0-q.1"
          "\n"
          "Optional:\n"
          "  --root: indicates that the qtpacket is the root packet.\n"
          "    I.e. it is one level shallower than all other packets.\n"
          "  --proto: indicates that the qtpacket is a protocol buffer.\n"
          "\n",
          progn.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  try {
    // Process commandline options
    int argn;
    std::string progname = argv[0];
    bool is_root = false;
    bool is_proto = false;
    bool help = false;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("root", is_root);
    options.flagOpt("proto", is_proto);

    if (!options.processAll(argc, argv, argn)
        || help) {
      usage(progname);
      return 1;
    }

    // Expect two file names after the optional flags.
    if (argc - argn != 1) {
      usage(progname);
    }

    // Check that we ostensibly have a qtpacket file.
    std::string qtpacket_file = argv[argn];
    if (!qtpacket_file.empty() && !khIsFileReadable(qtpacket_file)) {
      notify(NFY_FATAL, "Cannot read qtpacket file: %s",
             qtpacket_file.c_str());
    }

    // Get qt packet.
    if (is_proto) {
      DecodeProtoQtPacket(qtpacket_file, is_root);
    } else {
      DecodeQtPacket(qtpacket_file, is_root);
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Unknown error");
  }
}
