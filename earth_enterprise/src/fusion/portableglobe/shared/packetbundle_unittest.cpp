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

// Unit Tests for portable packet bundles.

#include <limits.h>
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_finder.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"

namespace fusion_portableglobe {

// Test class for portable packet bundles.
class PacketBundleTest : public testing::Test {
 protected:
  std::string test_directory;
  std::string data_directory;
  std::string delta_directory;
  std::string qtp_directory;
  std::string glb_file;

  virtual void SetUp() {
    test_directory = khTmpDirPath() + "/packet_bundle_test";
    data_directory = test_directory + "/data";
    qtp_directory = test_directory + "/qtp";
    delta_directory = test_directory + "/delta";
    glb_file = test_directory + "/test.glb";
    khEnsureParentDir(data_directory + "/index");
    khEnsureParentDir(delta_directory + "/index");
    khEnsureParentDir(qtp_directory + "/index");
  }

  virtual void TearDown() {
    khPruneDir(test_directory);
  }

  /**
   * Helper method converts blist representation to string quadtree path.
   * This is here partially because the interface initially supported
   * a blist and level argument rather than a quadtree path, so the tests
   * were first written using blists. Also, it is still here because the
   * blist is easier to use to create a big ordered sequence of quadtree
   * paths for testing.
   */
  void ConvertToQtPath(std::uint64_t blist,
                       int level,
                       std::string* qtpath) {
    *qtpath = "0";
    for (int i = 0; i < level; ++i) {
      std::uint64_t next = (blist >> (62 - i * 2)) & 0x3;
      switch (next) {
        case 0:
          *qtpath += "0";
          break;

        case 1:
          *qtpath += "1";
          break;

        case 2:
          *qtpath += "2";
          break;

        case 3:
          *qtpath += "3";
          break;
       }
    }
  }

  /**
   * Write and read known packets for a given bundle size.
   */
  void BasicRWTest(std::uint64_t max_file_size, bool multi_file) {
    std::uint64_t blist = 0x0000000000000000;
    std::uint16_t first_byte;
    std::uint64_t num_packets = 12000;
    std::uint32_t num_packet_blocks = 25;
    std::uint32_t packet_size = num_packet_blocks * 256;

    std::string qtpath;
    {
      PacketBundleWriter writer(test_directory, max_file_size);
      std::string data;
      for (std::uint64_t j = 0; j < num_packet_blocks; ++j) {
        for (std::uint32_t i = 0; i < 256; ++i) {
          data += static_cast<char>(i);
        }
      }

      for (std::uint64_t i = 0; i < num_packets; ++i) {
        char *ptr = const_cast<char *>(data.c_str());
        first_byte = 0xff & i;
        *ptr = static_cast<char>(first_byte);
        ConvertToQtPath(blist, 24, &qtpath);
        writer.AppendPacket(qtpath, kImagePacket, 0, data);
        blist += 0x0000000000010000;
      }
    }

    std::string index_file = test_directory + "/index";
    std::ifstream source(index_file.c_str());
    std::uint64_t index_size = khGetFileSizeOrThrow(index_file);
    std::uint64_t index_offset = 0;
    PacketBundleFinder finder(&source, index_offset, index_size);

    // Try last packet.
    blist -= 0x0000000000010000;
    PacketBundleReader reader(test_directory);
    std::string data;
    ConvertToQtPath(blist, 24, &qtpath);
    EXPECT_TRUE(reader.ReadPacket(qtpath, kImagePacket, 0, &data));
    for (std::uint32_t i = 0; i < data.size(); ++i) {
      if (i == 0) {
        EXPECT_EQ(first_byte,
                  static_cast<std::uint16_t>(data[i] & 0xff));
      } else {
        EXPECT_EQ(static_cast<std::uint16_t>(i & 0xff),
                  static_cast<std::uint16_t>(data[i] & 0xff));
      }
    }

    // Use find to get offset and size to read packet.
    IndexItem index_item;
    std::uint64_t last_offset;
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_TRUE(finder.FindPacketInIndex(&index_item));
    if (!multi_file) {
      last_offset = (num_packets - 1) * packet_size;
      EXPECT_EQ(last_offset, index_item.offset);
    }
    EXPECT_EQ(data.size(), index_item.packet_size);

    // Try first packet.
    ConvertToQtPath(0x0000000000000000, 24, &qtpath);
    EXPECT_TRUE(reader.ReadPacket(qtpath,
                                  kImagePacket,
                                  0,
                                  &data));
    for (std::uint32_t i = 0; i < data.size(); ++i) {
      if (i == 0) {
        EXPECT_EQ(0x00, static_cast<std::uint16_t>(data[i] & 0xff));
      } else {
        EXPECT_EQ(static_cast<std::uint16_t>(i & 0xff),
                  static_cast<std::uint16_t>(data[i] & 0xff));
      }
    }

    // Use find to get offset and size to read packet.
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_TRUE(finder.FindPacketInIndex(&index_item));
    EXPECT_EQ(qtpath, index_item.QuadtreeAddress());
    EXPECT_EQ(static_cast<std::uint64_t>(0), index_item.offset);
    EXPECT_EQ(data.size(), index_item.packet_size);

    // Try a few other packets ...
    ConvertToQtPath(0x0000000000070000, 24, &qtpath);
    EXPECT_TRUE(reader.ReadPacket(qtpath,
                                  kImagePacket,
                                  0,
                                  &data));
    for (std::uint32_t i = 0; i < data.size(); ++i) {
      if (i == 0) {
        EXPECT_EQ(0x07, static_cast<std::uint16_t>(data[i] & 0xff));
      } else {
        EXPECT_EQ(static_cast<std::uint16_t>(i & 0xff),
                  static_cast<std::uint16_t>(data[i] & 0xff));
      }
    }

    // Use find to get index entry.
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_TRUE(finder.FindPacketInIndex(&index_item));
    EXPECT_EQ(qtpath, index_item.QuadtreeAddress());
    EXPECT_EQ(data.size(), index_item.packet_size);
    // Make sure offset is a reasonable value.
    if (!multi_file) {
      EXPECT_TRUE(index_item.offset < last_offset);
      // Should be on a packet boundary.
      std::uint64_t packet_num = index_item.offset / packet_size;
      EXPECT_EQ(packet_num * packet_size, index_item.offset);
    }

    ConvertToQtPath(0x0000000001870000, 24, &qtpath);
    EXPECT_TRUE(reader.ReadPacket(qtpath,
                                  kImagePacket,
                                  0,
                                  &data));
    for (std::uint32_t i = 0; i < data.size(); ++i) {
      if (i == 0) {
        EXPECT_EQ(0x87, static_cast<std::uint16_t>(data[i] & 0xff));
      } else {
        EXPECT_EQ(static_cast<std::uint16_t>(i & 0xff),
                  static_cast<std::uint16_t>(data[i] & 0xff));
      }
    }

    // Use find to get index entry.
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_TRUE(finder.FindPacketInIndex(&index_item));
    EXPECT_EQ(qtpath, index_item.QuadtreeAddress());
    EXPECT_EQ(data.size(), index_item.packet_size);
    // Make sure offset is a reasonable value.
    if (!multi_file) {
      EXPECT_TRUE(index_item.offset < last_offset);
      // Should be on a packet boundary.
      std::uint64_t packet_num = index_item.offset / packet_size;
      EXPECT_EQ(packet_num * packet_size, index_item.offset);
    }

    ConvertToQtPath(0x000000000A910000, 24, &qtpath);
    EXPECT_TRUE(reader.ReadPacket(qtpath,
                                  kImagePacket,
                                  0,
                                  &data));
    for (std::uint32_t i = 0; i < data.size(); ++i) {
      if (i == 0) {
        EXPECT_EQ(0x91, static_cast<std::uint16_t>(data[i] & 0xff));
      } else {
        EXPECT_EQ(static_cast<std::uint16_t>(i & 0xff),
                  static_cast<std::uint16_t>(data[i] & 0xff));
      }
    }

    // Use find to get index entry.
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_TRUE(finder.FindPacketInIndex(&index_item));
    EXPECT_EQ(data.size(), index_item.packet_size);
    // Make sure offset is a reasonable value.
    if (!multi_file) {
      EXPECT_TRUE(index_item.offset < last_offset);
      // Should be on a packet boundary.
      std::uint64_t packet_num = index_item.offset / packet_size;
      EXPECT_EQ(packet_num * packet_size, index_item.offset);
    }

    ConvertToQtPath(0x000000000A910000, 23, &qtpath);
    EXPECT_FALSE(reader.ReadPacket(qtpath,
                                   kImagePacket,
                                   0,
                                   &data));
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_FALSE(finder.FindPacketInIndex(&index_item));

    EXPECT_FALSE(reader.ReadPacket(qtpath,
                                   kTerrainPacket,
                                   0,
                                   &data));

    ConvertToQtPath(0x800000000A910000, 23, &qtpath);
    EXPECT_FALSE(reader.ReadPacket(qtpath,
                                   kImagePacket,
                                   0,
                                   &data));
    index_item.Fill(qtpath, kImagePacket, 0);
    EXPECT_FALSE(finder.FindPacketInIndex(&index_item));
  }

  /**
   * Write and read known packets for a given bundle size.
   */
  void DeltaRWTest(std::uint64_t max_file_size) {
    std::uint64_t blist = 0x0000000000000000;
    std::uint16_t first_byte;
    std::uint64_t num_packets = 12000;
    std::uint32_t num_packet_blocks = 25;

    std::string data;
    for (std::uint64_t j = 0; j < num_packet_blocks; ++j) {
      for (std::uint32_t i = 0; i < 256; ++i) {
        data += static_cast<char>(i);
      }
    }

    // Build bundles skipping a packet at regular intervals.
    std::string qtpath;
    int skip_interval = 100;
    int skip_count = skip_interval;
    {
      PacketBundleWriter writer(data_directory, max_file_size);

      for (std::uint64_t i = 0; i < num_packets; ++i) {
        char *ptr = const_cast<char *>(data.c_str());
        first_byte = 0xff & i;
        *ptr = static_cast<char>(first_byte);
        ConvertToQtPath(blist, 24, &qtpath);
        --skip_count;
        if (skip_count) {
          writer.AppendPacket(qtpath, kImagePacket, 0, data);
        } else {
          skip_count = skip_interval;
        }
        blist += 0x0000000000010000;
      }
    }

    // Create a glb file with the partial data.
    {
      // Create a dummy qtp index file.
      std::string qtp_index_file = qtp_directory + "/index";
      std::ofstream fp(qtp_index_file.c_str());
      fp.write("This is a dummy file.\n", 22);
      fp.close();
      std::uint32_t prefix_len = test_directory.size();
      fusion_portableglobe::FilePacker packer(glb_file,
                                              qtp_index_file,
                                              prefix_len);
      packer.AddAllFiles(data_directory, prefix_len);
    }

    // Reset the starting address.
    blist = 0x0000000000000000;

    // Now create a delta, which should contain only the
    // skipped packets.
    {
      PacketBundleWriter writer(
          delta_directory, glb_file, false, max_file_size);
      std::uint32_t write_cnt = 0;
      for (std::uint64_t i = 0; i < num_packets; ++i) {
        char *ptr = const_cast<char *>(data.c_str());
        first_byte = 0xff & i;
        *ptr = static_cast<char>(first_byte);
        ConvertToQtPath(blist, 24, &qtpath);
        if (writer.AppendPacket(qtpath, kImagePacket, 0, data)) {
          ++write_cnt;
        }
        blist += 0x0000000000010000;
      }
      EXPECT_EQ(num_packets / skip_interval, write_cnt);
    }

    // Now try to read the data back.
    PacketBundleReader data_reader(data_directory);
    PacketBundleReader delta_reader(delta_directory);
    blist = 0x0000000000000000;
    for (std::uint64_t i = 0; i < num_packets; ++i) {
      first_byte = 0xff & i;
      ConvertToQtPath(blist, 24, &qtpath);
      --skip_count;
      if (skip_count) {
        EXPECT_FALSE(delta_reader.ReadPacket(qtpath, kImagePacket, 0, &data));
        EXPECT_TRUE(data_reader.ReadPacket(qtpath, kImagePacket, 0, &data));
      } else {
        EXPECT_FALSE(data_reader.ReadPacket(qtpath, kImagePacket, 0, &data));
        EXPECT_TRUE(delta_reader.ReadPacket(qtpath, kImagePacket, 0, &data));
        skip_count = skip_interval;
      }
      EXPECT_EQ(first_byte, 0xff & data[0]);
      blist += 0x0000000000010000;
    }
  }
};

// Tests index item size.
TEST_F(PacketBundleTest, TestIndexItemSize) {
  EXPECT_EQ(static_cast<size_t>(24), sizeof(IndexItem));
}


// Tests index item operators.
TEST_F(PacketBundleTest, TestIndexItemOperators) {
  IndexItem item1;
  IndexItem item2;

  item1.btree_high = 0x80000000;
  item1.btree_low = 0x8000;
  item1.level = 0x80;
  item1.packet_type = 0x80;
  item1.file_id = 0x3784;
  item1.offset = 0x92939202;
  item1.packet_size = 0x9202;
  item1.channel = 5;

  item2.btree_high = 0x80000000;
  item2.btree_low = 0x8000;
  item2.level = 0x80;
  item2.packet_type = 0x80;
  item2.file_id = 0x1847;
  item2.offset = 0x847201;
  item2.packet_size = 0x3902;
  item2.channel = 5;

  EXPECT_TRUE(item1 == item2);
  EXPECT_FALSE(item1 < item2);

  item2.btree_high = 0x7fffffff;
  EXPECT_FALSE(item1 == item2);
  EXPECT_FALSE(item1 < item2);
  EXPECT_TRUE(item2 < item1);

  item2.btree_high = 0x80000000;
  item2.btree_low = 0x7fff;
  EXPECT_FALSE(item1 == item2);
  EXPECT_FALSE(item1 < item2);
  EXPECT_TRUE(item2 < item1);

  item2.btree_low = 0x8000;
  item2.level = 0x7f;
  EXPECT_FALSE(item1 == item2);
  EXPECT_FALSE(item1 < item2);
  EXPECT_TRUE(item2 < item1);

  item2.level = 0x80;
  item2.packet_type = 0x7f;
  EXPECT_FALSE(item1 == item2);
  EXPECT_FALSE(item1 < item2);
  EXPECT_TRUE(item2 < item1);

  item2.packet_type = 0x80;
  item2.channel = 4;
  EXPECT_FALSE(item1 == item2);
  EXPECT_FALSE(item1 < item2);
  EXPECT_TRUE(item2 < item1);
}

// Tests quadtree address converter.
TEST_F(PacketBundleTest, TestQuadtreeAddress) {
  IndexItem item;
  item.btree_high = 0x12345678;
  item.btree_low = 0x9abc;

  item.level = 0;
  EXPECT_EQ("0", item.QuadtreeAddress());
  item.level = 1;
  EXPECT_EQ("00", item.QuadtreeAddress());
  item.level = 2;
  EXPECT_EQ("001", item.QuadtreeAddress());
  item.level = 3;
  EXPECT_EQ("0010", item.QuadtreeAddress());
  item.level = 4;
  EXPECT_EQ("00102", item.QuadtreeAddress());
  item.level = 5;
  EXPECT_EQ("001020", item.QuadtreeAddress());
  item.level = 6;
  EXPECT_EQ("0010203", item.QuadtreeAddress());
  item.level = 7;
  EXPECT_EQ("00102031", item.QuadtreeAddress());
  item.level = 8;
  EXPECT_EQ("001020310", item.QuadtreeAddress());
  item.level = 9;
  EXPECT_EQ("0010203101", item.QuadtreeAddress());
  item.level = 10;
  EXPECT_EQ("00102031011", item.QuadtreeAddress());
  item.level = 11;
  EXPECT_EQ("001020310111", item.QuadtreeAddress());
  item.level = 12;
  EXPECT_EQ("0010203101112", item.QuadtreeAddress());
  item.level = 13;
  EXPECT_EQ("00102031011121", item.QuadtreeAddress());
  item.level = 14;
  EXPECT_EQ("001020310111213", item.QuadtreeAddress());
  item.level = 15;
  EXPECT_EQ("0010203101112132", item.QuadtreeAddress());
  item.level = 16;
  EXPECT_EQ("00102031011121320", item.QuadtreeAddress());
  item.level = 17;
  EXPECT_EQ("001020310111213202", item.QuadtreeAddress());
  item.level = 18;
  EXPECT_EQ("0010203101112132021", item.QuadtreeAddress());
  item.level = 19;
  EXPECT_EQ("00102031011121320212", item.QuadtreeAddress());
  item.level = 20;
  EXPECT_EQ("001020310111213202122", item.QuadtreeAddress());
  item.level = 21;
  EXPECT_EQ("0010203101112132021222", item.QuadtreeAddress());
  item.level = 22;
  EXPECT_EQ("00102031011121320212223", item.QuadtreeAddress());
  item.level = 23;
  EXPECT_EQ("001020310111213202122233", item.QuadtreeAddress());
  item.level = 24;
  EXPECT_EQ("0010203101112132021222330", item.QuadtreeAddress());
}

// Tests basic packetbundle read and write.
TEST_F(PacketBundleTest, TestPacketBundleRW) {
  // Expect a single bundle file.
  BasicRWTest(0x4000000000000000, false);
}

// Tests basic packetbundle read and write with multiple packet bundle files.
TEST_F(PacketBundleTest, TestMultiFilePacketBundleRW) {
  // Expect multiple bundle files.
  BasicRWTest(10000000, true);
}

// Tests expected struct size(s). If we cross an 8 byte boundary, then the
// structure size may differ on 32 bit machines.
TEST_F(PacketBundleTest, TestClassSize) {
  EXPECT_EQ(static_cast<std::uint32_t>(24), sizeof(IndexItem));
}


// Tests delta packetbundle read and write with multiple packet bundle files.
TEST_F(PacketBundleTest, TestMultiFileDeltaPacketBundleRW) {
  // TODO: Refactor to fix multibundle handling, even though
  // TODO: we don't use multiple bundles in this overflow
  // TODO: way. Use server (Mac and Win) code which has become
  // TODO: out of sync.
  DeltaRWTest(1000000000000);
}

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
