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

// Unit Tests for globe files.

#include <limits.h>
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "fusion/portableglobe/shared/file_package.h"
#include "fusion/portableglobe/shared/file_packer.h"
#include "fusion/portableglobe/shared/file_unpacker.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

// Test classes for globe file package.
class FilePackerTest : public testing::Test {
 protected:
  std::string test_directory;
  std::string data_packet;
  std::string packet1_directory;
  std::string packet1_bundle;
  std::string packet1_index;
  std::string packet2_directory;
  std::string packet2_bundle;
  std::string packet2_index;
  std::string files_directory;
  std::string data_file1;
  std::string data_file2;
  std::string data_file3;
  std::string file1_data;
  std::string file2_data;
  std::string file3_data;
  std::string earth_directory;
  std::string info_file;
  std::string default_info;
  std::string standalone_file;

  std::string test_addendum_directory;
  std::string files_addendum_directory;
  std::string data_addendum_file2;
  std::string data_addendum_file4;
  std::string file2_addendum_data;
  std::string file4_addendum_data;
  std::string addendum_file;

  std::string concatenated_file;

  virtual void SetUp() {
    test_directory = khTmpDirPath() + "/file_packer_test";

    packet1_directory = test_directory + "/data";
    packet1_bundle = packet1_directory + "/pbundle_0000";
    packet1_index = packet1_directory + "/index";
    khEnsureParentDir(packet1_index);

    packet2_directory = test_directory + "/qtp";
    packet2_bundle = packet2_directory + "/pbundle_0000";
    packet2_index = packet2_directory + "/index";
    khEnsureParentDir(packet2_index);

    WritePacketFiles(packet1_directory, 10000);
    WritePacketFiles(packet2_directory, 200);

    files_directory = test_directory + "/files";
    data_file1 = files_directory + "/data_file1";
    data_file2 = files_directory + "/data_file2";
    data_file3 = files_directory + "/data_file3";
    khEnsureParentDir(data_file1);


    WriteDataFile(data_file1, "Data file 1 data\n", 200, &file1_data);
    WriteDataFile(data_file2, "Data file 2 data\n", 300, &file2_data);
    WriteDataFile(data_file3, "Data file 3 data\n", 100, &file3_data);

    earth_directory = test_directory + "/earth";
    info_file = earth_directory + "/info.txt";
    khEnsureParentDir(info_file);
    default_info = "Portable Globe\nCopyright 2012\n2012-02-24 01:26:07 GMT\n";
    WriteFile(info_file, default_info);

    test_addendum_directory = khTmpDirPath() + "/file_packer_addendum_test";
    files_addendum_directory = test_addendum_directory + "/files";
    data_addendum_file2 = files_addendum_directory + "/data_file2";
    data_addendum_file4 = files_addendum_directory + "/data_file4";
    khEnsureParentDir(data_addendum_file2);

    WriteDataFile(data_addendum_file2,
                  "Addendum data file 2 data\n",
                  200, &file2_addendum_data);
    WriteDataFile(data_addendum_file4,
                  "Addendum data file 4 data\n",
                  400, &file4_addendum_data);

    standalone_file = test_directory + "/standalone.glc";
    addendum_file = test_addendum_directory + "/addendum.glc";
    concatenated_file = test_addendum_directory + "/concatenated.glc";
    BuildPackage();
  }

  virtual void TearDown() {
    khPruneDir(test_directory);
    khPruneDir(test_addendum_directory);
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
   * Write a packet file and its index.
   */
  void WritePacketFiles(std::string packet_directory, int num_packets) {
    std::uint64_t blist = 0x0000000000000000;
    int num_packet_blocks = 25;
    // int header_size = 8;
    // int packet_size = num_packet_blocks * 256 + header_size;

    std::string qtpath;
    {
      PacketBundleWriter writer(packet_directory, 0x80000000);
      data_packet = "";
      for (int j = 0; j < num_packet_blocks; ++j) {
        for (int i = 0; i < 256; ++i) {
          data_packet += static_cast<char>(i);
        }
      }

      for (int i = 0; i < num_packets; ++i) {
        SetFirstByte(i);
        ConvertToQtPath(blist, 24, &qtpath);
        writer.AppendPacket(qtpath, kImagePacket, 0, data_packet);
        blist += 0x0000000000010000;
      }
    }
  }

  /**
   * Write a packet file and its index.
   */
  void WriteDataFile(std::string path,
                     std::string data,
                     int num_repeats,
                     std::string* file_content) {
    *file_content = "";
    for (int i = 0; i < num_repeats; ++i) {
      *file_content += data;
    }
    WriteFile(path, *file_content);
  }

  /**
   * Write given content to file.
   */
  void WriteFile(const std::string& path, const std::string& file_content) {
    std::ofstream fp(path.c_str());
    fp.write(file_content.c_str(), file_content.size());
    fp.close();
  }

  /**
   * Build the package file.
   */
  void BuildPackage() {
    // Build standalone packed file.
    size_t prefix_len = test_directory.size();
    {
      fusion_portableglobe::FilePacker packer(standalone_file,
                                              packet1_bundle,
                                              prefix_len);
      packer.AddFile(packet1_index, prefix_len);
      packer.AddAllFiles(packet2_directory, prefix_len);
      packer.AddAllFiles(files_directory, prefix_len);
      packer.AddAllFiles(earth_directory, prefix_len);

      // Need destructor to execute before standalone file
      // can be used for addendum file.
    }

    // Build addendum packed file for the standalone file.
    fusion_portableglobe::FilePacker addendum_packer(addendum_file,
                                                     standalone_file);
    prefix_len = test_addendum_directory.size();
    addendum_packer.AddAllFiles(files_addendum_directory, prefix_len);
  }

  /**
   * Set first byte.
   */
  void SetFirstByte(int value) {
    std::uint16_t first_byte;
    char *ptr = const_cast<char *>(data_packet.c_str());
    first_byte = 0xff & value;
    *ptr = static_cast<char>(first_byte);
  }

  /**
   * Test that the current file or package pointed to by the unpacker is what
   * is in data.
   */
  void TestContent(FileUnpacker* unpacker,
                   std::string data,
                   std::string packed_file) {
    std::ifstream fp(packed_file.c_str(), std::ios::in | std::ios::binary);
    std::string file_data;
    file_data.resize(unpacker->Size());
    fp.seekg(unpacker->Offset());
    fp.read(&file_data[0], unpacker->Size());
    fp.close();

    EXPECT_EQ(data, file_data);
  }
};

// Tests package index.
TEST_F(FilePackerTest, TestPackageIndex) {
  FileUnpacker unpacker(standalone_file.c_str());
  std::map<std::string, PackageFileLoc> index;
  index = unpacker.Index();

  // 1 info file + 4 files and 2 pairs of packet data file and index
  EXPECT_EQ(9, static_cast<int>(index.size()));

  std::map<std::string, PackageFileLoc>::iterator it;
  it = index.find("files/data_file1");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(data_file1), it->second.Size());

  it = index.find("files/data_file2");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(data_file2), it->second.Size());

  it = index.find("files/data_file3");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(data_file3), it->second.Size());

  it = index.find("data/pbundle_0000");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(packet1_bundle), it->second.Size());

  it = index.find("data/index");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(packet1_index), it->second.Size());

  it = index.find("qtp/pbundle_0000");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(packet2_bundle), it->second.Size());

  it = index.find("qtp/pbundle_0000.crc");
  EXPECT_TRUE(it != index.end());

  it = index.find("qtp/index");
  EXPECT_TRUE(it != index.end());
  EXPECT_EQ(khGetFileSizeOrThrow(packet2_index), it->second.Size());
}

// Tests package files.
TEST_F(FilePackerTest, TestPackageFiles) {
  FileUnpacker unpacker(standalone_file.c_str());
  std::map<std::string, PackageFileLoc> index;

  // Check that content is the expected content.
  EXPECT_TRUE(unpacker.FindFile("files/data_file1"));
  TestContent(&unpacker, file1_data, standalone_file);

  EXPECT_TRUE(unpacker.FindFile("files/data_file2"));
  TestContent(&unpacker, file2_data, standalone_file);

  EXPECT_TRUE(unpacker.FindFile("files/data_file3"));
  TestContent(&unpacker, file3_data, standalone_file);
}


// Tests earth/info.txt.
TEST_F(FilePackerTest, TestPackageInfoFile) {
  FileUnpacker unpacker(standalone_file.c_str());

  // Check that content is the expected content.
  EXPECT_EQ(default_info, unpacker.Info());
  std::string id = unpacker.Id();
  EXPECT_EQ(id, "20120224012607-f0ab829d-3e7fd92");

  // Rebuild with empty info file.
  WriteFile(info_file, "");
  BuildPackage();
  FileUnpacker unpacker2(standalone_file.c_str());
  id = unpacker2.Id();
  EXPECT_EQ(id, "noinfo-3e7fd5c");

  // Rebuild with empty info file.
  // Randomly change one character so that date and length
  // don't change, but csum does.
  std::string new_info = default_info;
  new_info[37] += 1;
  WriteFile(info_file, new_info);
  BuildPackage();
  FileUnpacker unpacker3(standalone_file.c_str());
  id = unpacker3.Id();
  // Just to make sure.
  EXPECT_NE(default_info, unpacker3.Info());
  EXPECT_EQ(id, "20120224012607-f0ab839d-3e7fd92");
}

// Tests addendum files.
TEST_F(FilePackerTest, TestAddendumFiles) {
  // We won't even be able to find the index until we have
  // concatenated the addendum onto the standalone file.
  char str[256];
  // Expect users to concatenate files with standard cat.
  snprintf(str, sizeof(str), "cat %s %s > %s",
           standalone_file.c_str(),
           addendum_file.c_str(),
           concatenated_file.c_str());
  system(str);

  FileUnpacker unpacker(concatenated_file.c_str());

  // Check that content is the expected content.
  EXPECT_TRUE(unpacker.FindFile("files/data_file1"));
  TestContent(&unpacker, file1_data, concatenated_file);

  EXPECT_TRUE(unpacker.FindFile("files/data_file2"));
  TestContent(&unpacker, file2_addendum_data, concatenated_file);

  EXPECT_TRUE(unpacker.FindFile("files/data_file3"));
  TestContent(&unpacker, file3_data, concatenated_file);

  EXPECT_TRUE(unpacker.FindFile("files/data_file4"));
  TestContent(&unpacker, file4_addendum_data, concatenated_file);
}

// Tests package packets.
TEST_F(FilePackerTest, TestPackagePackets) {
  FileUnpacker unpacker(standalone_file.c_str());
  std::map<std::string, PackageFileLoc> index;

  // Try a packet for data and qtp.
  std::string qtpath;
  ConvertToQtPath(0x0000000000070000, 24, &qtpath);
  EXPECT_TRUE(unpacker.FindDataPacket(qtpath.c_str(), kImagePacket, 0));
  SetFirstByte(7);
  TestContent(&unpacker, data_packet, standalone_file);

  EXPECT_TRUE(unpacker.FindQtpPacket(qtpath.c_str(), kImagePacket, 0));
  SetFirstByte(7);
  TestContent(&unpacker, data_packet, standalone_file);

  // Try another one.
  ConvertToQtPath(0x0000000003CF0000, 24, &qtpath);
  EXPECT_TRUE(unpacker.FindDataPacket(qtpath.c_str(), kImagePacket, 0));
  SetFirstByte(0xCF);
  TestContent(&unpacker, data_packet, standalone_file);

  // Only 200 packets written for qtp.
  EXPECT_FALSE(unpacker.FindQtpPacket(qtpath.c_str(), kImagePacket, 0));
}

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
