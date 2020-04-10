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



#include <iostream>
#include <string>
#include <vector>

#include "common/khConstants.h"
#include "common/khGuard.h"
#include "protobuf/quadtreeset.pb.h"
#include "common/qtpacket/quadtree_utils.h"
#include "common/qtpacket/quadset_gather.h"
#include "common/qtpacket/quadtreepacket.h"
#include "common/qtpacket/quadtreepacketitem.h"
#include "common/qtpacket/quadtree_processing.h"
#include "common/khGetopt.h"
#include <keyhole/jpeg_comment_date.h>
#include "common/UnitTest.h"

using namespace qtpacket;

static const int kTestImageryVersion = 10;
static const int kTestImageryVersion1 = kTestImageryVersion + 1;
static const int kTestImageryVersion2 = kTestImageryVersion + 2;
static const int kTestImageryVersion3 = kTestImageryVersion + 3;
static const int kTestImageryProvider = 1011;
static const int kTestTerrainVersion = 20;
static const int kTestTerrainVersion1 = kTestTerrainVersion + 1;
static const int kTestTerrainProvider = 1013;
static const int kTestVectorVersion = 40;
static const int kTestVectorVersion1 = kTestVectorVersion + 1;
static const int kTestQTPacketVersion = 30;
static const int kTestVectorChannel = 20;


//
// Data to import into the index
//

struct IndexRow {
  std::string quadset_path;
  std::int32_t  subindex;
  std::int32_t  layer_type;
  std::int32_t  version;
  std::int32_t  provider;
  bool   deleted;
  // Indicates which children are present in a 5-to-1 packed terrain
  // node (see magrathean.protodevel)
  std::int32_t  terrain_presence_flags;
};

//
// The test data is intended to catch these things:
//
// - Data far down in the tree causes cache nodes to be set correctly
//   higher up in the tree, including creating entire quadsets if
//   necessary.
//
// - The root node is intentionally not completely full.
//
// - Mix of terrain, imagery, and vector data.
//
// - TODO:
//   There are vector nodes with old versions, which are intended to
//   be ignored by the quadtree builder.
//
// - TODO:
//   There's a delete marker that should cause data to be ignored.
//
// - One quadset has a terrain node with children, to test the 5-to-1
//   packing of the terrain in the index.
//
//
// The test data looks like this:
//
//                    /\        quadset ""                             /
//                   /IV\                                              /
//                  /.  .\                                             /
//                 /. X  .\                                            /
//                /.      T\                                           /
//                ----------                                           /
//               /\                                                    /
//              /. \        quadset "000"                              /
//             /.   \                                                  /
//            /.     \                                                 /
//           /.       \                                                /
//          /.         \                                               /
//          ------------                                               /
//          /\*                                                        /
//         /..\          quadset "0000000"                             /
//        /  ..\                                                       /
//       /   V .\                                                      /
//      /       .\   images at these nodes:                            /
//     /       III\  subindex 336: level=11, row=2032, col=2
//     ------------  subindex 337: level=11, row=2033, col=0
//                   subindex 339: level=11, row=2032, col=1
//
//   * = position of quadset "0000003"
//               /\                                                    /
//              /. \        quadset "0000003"                          /
//             /.   \                                                  /
//            /.  D  \                                                 /
//           /.       \                                                /
//          / IT     X \  I: subindex 22: level=11, row=2031, col=0
//          ------------  T: subindex 23: level=11, row=2031, col=1
//          /\                                                         /
//         /. \        quadset "00000030000"                            /
//        /.   \                                                       /
//       /  T   \     terrain with two children packed in              /
//      /  T T   \                                                     /
//     /          \                                                    /
//     ------------
//
// I = imagery
// T = terrain
// V = vector
// X = vector with an old version number -- should be ignored (TODO)
// D = node with data in version n, and a delete marker in version n+1 -- ignore
//     (TODO)
// . = a node that should be generated

static const std::int32_t kNodeTypeImage = QuadtreePacketItem::kLayerImagery;
static const std::int32_t kNodeTypeTerrain = QuadtreePacketItem::kLayerTerrain;
static const std::int32_t kNodeTypeVector =
  QuadtreePacketItem::kLayerVectorMin + kTestVectorChannel;

class TestQuadtreeBuilderIndexSource
    : public MergeSource<QuadsetItem<QuadtreePacketItem> > {
 public:
  TestQuadtreeBuilderIndexSource(const std::string &name)
      : MergeSource<QuadsetItem<QuadtreePacketItem> >(name),
        next_(0) {
    // Build sorted container of IndexRow nodes
    for (size_t i = 0; i < kDataCount; ++i) {
      const IndexRow &item = kIndexData[i];
      std::uint64_t quadset_num =
        QuadtreeNumbering::TraversalPathToGlobalNodeNumber(item.quadset_path);
      QuadtreePath item_path =
        QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(quadset_num,
                                                             item.subindex);
      index_data_.insert(std::make_pair(item_path, item));
    }

    Advance();
  }
  virtual const MergeType &Current() const { return *current_; }
  virtual bool Advance() {
    if (!index_data_.empty()) {
      {                                 // protect scope of reference vars
        QuadtreePath item_path = index_data_.begin()->first;
        const IndexRow &item = index_data_.begin()->second;
        std::uint64_t quadset_num =
          QuadtreeNumbering::TraversalPathToGlobalNodeNumber(item.quadset_path);

        current_ = TransferOwnership(
            new MergeType(item_path,
                          QuadtreePacketItem(item.layer_type,
                                             item.version,
                                             item.provider,
                                             kUnknownDate)));
        notify(NFY_DEBUG, "Source generated (%llu,%d) \"%s\"",
               static_cast<unsigned long long>(quadset_num),
               item.subindex,
               item_path.AsString().c_str());
        assert(!(current_path_ > item_path));
        current_path_ = item_path;
      }

      index_data_.erase(index_data_.begin());
      return true;
    } else {
      current_.clear();
      return false;
    }
  }
  virtual void Close() {}
 private:
  khDeleteGuard<QuadsetItem<QuadtreePacketItem> > current_;
  QuadtreePath current_path_;
  std::multimap<QuadtreePath,IndexRow> index_data_;
  size_t next_;
  static const size_t kDataCount = 17;
  static const IndexRow kIndexData[kDataCount];
};

// Table of index data
const IndexRow TestQuadtreeBuilderIndexSource::kIndexData[kDataCount] = {
  { "", 0,          kNodeTypeImage,   kTestImageryVersion,
      kTestImageryProvider },
  { "", 0,          kNodeTypeVector,  kTestVectorVersion },
  { "", 27,         kNodeTypeVector,  kTestVectorVersion1 },
  { "", 84,         kNodeTypeTerrain, kTestTerrainVersion,
      kTestTerrainProvider },
  { "0000000", 89,  kNodeTypeVector,  kTestVectorVersion },
  { "0000000", 336, kNodeTypeImage,   kTestImageryVersion,
      kTestImageryProvider },
  { "0000000", 337, kNodeTypeImage,   kTestImageryVersion,
      kTestImageryProvider },
  { "0000000", 339, kNodeTypeImage,   kTestImageryVersion,
      kTestImageryProvider },
  { "0000000", 339, kNodeTypeTerrain, kTestTerrainVersion1,
      kTestTerrainProvider },
  { "0000003", 22,  kNodeTypeImage,   kTestImageryVersion,
      kTestImageryProvider },
  { "0000003", 23,  kNodeTypeTerrain, kTestTerrainVersion,
      kTestTerrainProvider },
  { "0000003", 24,  kNodeTypeImage,   kTestImageryVersion1 },
  { "0000003", 25,  kNodeTypeImage,   kTestImageryVersion2 },
  { "0000003", 337, kNodeTypeVector,  kTestVectorVersion1 },
  { "0000003", 111, kNodeTypeImage,   kTestImageryVersion3 },
  { "0000003", 111, kNodeTypeImage,   kTestImageryVersion,
      kTestImageryProvider,
    true },
  { "00000030000", 86, kNodeTypeTerrain, kTestTerrainVersion,
    kTestTerrainProvider, false, 0x05 << 1},
};

//
// Expected results in the quadtree bigtable
//
struct LayerTypeInfo {
  bool has_data;
  std::int32_t version;
  std::int32_t provider;
};

struct QuadtreePacketInfo {
  int  subindex;
  int  version;
  LayerTypeInfo imagery_info;
  LayerTypeInfo terrain_info;
  LayerTypeInfo vector_info;
  bool cache_node;
  int  child_flags;
};

struct ExpectedQuadset {
  std::string quadset_path;
  int    num_nodes;
  QuadtreePacketInfo nodes[10];
};

static const size_t kNumImageryVersions = 3;
static const LayerTypeInfo kTestImageryInfo[kNumImageryVersions] = {
  { true, kTestImageryVersion,  kTestImageryProvider },
  { true, kTestImageryVersion1, kTestImageryProvider },
  { true, kTestImageryVersion2, kTestImageryProvider } };
static const size_t kNumTerrainVersions = 2;
static const LayerTypeInfo kTestTerrainInfo[kNumTerrainVersions] = {
  { true, kTestTerrainVersion,  kTestTerrainProvider },
  { true, kTestTerrainVersion1, kTestTerrainProvider } };
static const size_t kNumVectorVersions = 2;
static const LayerTypeInfo kTestVectorInfo[kNumVectorVersions] = {
  { true, kTestVectorVersion,  0 },
  { true, kTestVectorVersion1, 0 } };
static const LayerTypeInfo kEmptyLayerInfo = {false, 0, 0 };


// Expected results - must be in POSTorder by quadset root path
static const size_t kNumExpectedQuadsets = 5;
static const ExpectedQuadset expected_quadsets[kNumExpectedQuadsets] = { {
  "0000000", 6, { { 0,   kTestQTPacketVersion, kEmptyLayerInfo,
      kEmptyLayerInfo,     kEmptyLayerInfo,    false, 0x0a },
                  { 86,  kTestQTPacketVersion, kEmptyLayerInfo,
                      kEmptyLayerInfo,     kEmptyLayerInfo,    false, 0x04 },
                  { 89,  kTestQTPacketVersion, kEmptyLayerInfo,
                      kEmptyLayerInfo,     kTestVectorInfo[0], false, 0x00 },
                  { 336, kTestQTPacketVersion, kTestImageryInfo[0],
                      kEmptyLayerInfo,     kEmptyLayerInfo,    false, 0x00 },
                  { 337, kTestQTPacketVersion, kTestImageryInfo[0],
                      kEmptyLayerInfo,     kEmptyLayerInfo,    false, 0x00 },
                  { 339, kTestQTPacketVersion, kTestImageryInfo[0],
                      kTestTerrainInfo[1], kEmptyLayerInfo,    false, 0x00 },
  }, }, {
  // Children should NOT appear because of 5->1 packing
  "00000030000", 1, { { 86,  kTestQTPacketVersion, kEmptyLayerInfo,
      kTestTerrainInfo[0],  kEmptyLayerInfo, false,  0x00 },
  }, }, {
  "0000003", 2, { { 22,  kTestQTPacketVersion, kTestImageryInfo[0],
      kEmptyLayerInfo,      kEmptyLayerInfo, true,  0x00 },
                  { 23,  kTestQTPacketVersion, kEmptyLayerInfo,
                      kTestTerrainInfo[0],  kEmptyLayerInfo, false, 0x00 },
  }, }, {
  "000", 6, { { 0,  kTestQTPacketVersion, kEmptyLayerInfo, kEmptyLayerInfo,
      kEmptyLayerInfo, false, 0x01 },
              { 1,  kTestQTPacketVersion, kEmptyLayerInfo, kEmptyLayerInfo,
                  kEmptyLayerInfo, false, 0x01 },
              { 2,  kTestQTPacketVersion, kEmptyLayerInfo, kEmptyLayerInfo,
                  kEmptyLayerInfo, false, 0x01 },
              { 6,  kTestQTPacketVersion, kEmptyLayerInfo, kEmptyLayerInfo,
                  kEmptyLayerInfo, false, 0x09 },
              { 22, kTestQTPacketVersion, kEmptyLayerInfo, kEmptyLayerInfo,
                  kEmptyLayerInfo, true,  0x00 },
              { 25, kTestQTPacketVersion, kEmptyLayerInfo, kEmptyLayerInfo,
                  kEmptyLayerInfo, true,  0x00 },
  }, }, {
  "", 7, { { 0,  kTestQTPacketVersion, kTestImageryInfo[0], kEmptyLayerInfo,
      kTestVectorInfo[0],  false, 0x09 },
           { 4,  kTestQTPacketVersion, kEmptyLayerInfo,     kEmptyLayerInfo,
               kEmptyLayerInfo,     false, 0x08 },
           { 20, kTestQTPacketVersion, kEmptyLayerInfo,     kEmptyLayerInfo,
               kEmptyLayerInfo,     false, 0x08 },
           { 84, kTestQTPacketVersion, kEmptyLayerInfo,     kTestTerrainInfo[0],
               kEmptyLayerInfo,     false, 0x00 },
           { 1,  kTestQTPacketVersion, kEmptyLayerInfo,     kEmptyLayerInfo,
               kEmptyLayerInfo,     false, 0x03 },
           { 5,  kTestQTPacketVersion, kEmptyLayerInfo,     kEmptyLayerInfo,
               kEmptyLayerInfo,     false, 0x01 },
           { 21, kTestQTPacketVersion, kEmptyLayerInfo,     kEmptyLayerInfo,
               kEmptyLayerInfo,     true,  0x00 },
  }, },
};



class QuadtreeBuilderUnitTest : public UnitTest<QuadtreeBuilderUnitTest> {
 public:
  QuadtreeBuilderUnitTest() : BaseClass("QuadtreeBuilderUnitTest") {
    REGISTER(TestRefSerializer);
    REGISTER(TestInsertChannel);
    REGISTER(TestQuadtreeBuilder1);
    REGISTER(TestQuadtreeBuilder2);
  }

  bool TestRefSerializer() {
    // Test serialization of KhQuadtreeDataReference
    KhQuadtreeDataReference src_ref(QuadtreePath("03123"), 3, 102, 11);
    LittleEndianWriteBuffer write_buffer;
    write_buffer << src_ref;
    if (!EXPECT_EQ(write_buffer.size(), KhQuadtreeDataReference::kSerialSize))
      return false;
    LittleEndianReadBuffer read_buffer(write_buffer.data(),
                                       write_buffer.size());
    KhQuadtreeDataReference dst_ref;
    read_buffer >> dst_ref;
    if (!EXPECT(src_ref == dst_ref))
      return false;

    return true;
  }

  bool TestInsertChannel() {
    // Test channel insertion.  Source channels are ordered to test
    // inserts at front, middle, and back.
    const size_t kCount = 10;
    static const std::uint16_t channel_src[kCount] = {
      150, 155, 160, 140, 135, 170, 157, 158, 180, 130
    };
    static const std::uint16_t version_src[kCount] = {
      10150, 10155, 10160, 10140, 10135, 10170, 10157, 10158, 10180, 10130
    };

    static const std::uint16_t channel_expect[kCount] = {
      130, 135, 140, 150, 155, 157, 158, 160, 170, 180
    };
    static const std::uint16_t version_expect[kCount] = {
      10130, 10135, 10140, 10150, 10155, 10157, 10158, 10160, 10170, 10180
    };

    KhQuadTreeQuantum16 node;

    // Insert test data into node
    for (size_t i = 0; i < kCount; ++i) {
      qtpacket::InsertChannel(channel_src[i], version_src[i], &node);
    }

    // Compare node to expected data
    if (!EXPECT_EQ(node.num_channels(), kCount))
      return false;

    bool result = true;
    for (size_t i = 0; i < kCount; ++i) {
      result = result && EXPECT_EQ(channel_expect[i], node.channel_type[i]);
      result = result && EXPECT_EQ(version_expect[i], node.channel_version[i]);
    }
    return result;
  }

  QuadsetGather<QuadtreePacketItem>* CreateGatherer(const std::string& name) {
    // Set up test source and merge
    khDeleteGuard<Merge<QuadsetItem<QuadtreePacketItem> > > index_merge(
        TransferOwnership(
            new Merge<QuadsetItem<QuadtreePacketItem> >("TestIndexMerge")));
    index_merge->AddSource(
        TransferOwnership(
            new TestQuadtreeBuilderIndexSource("TestIndexSource")));
    index_merge->Start();
    return new QuadsetGather<QuadtreePacketItem>(name,
                                             TransferOwnership(index_merge));
  }

  bool TestQuadtreeBuilder1() {
    // Test the builder for Format1 of QuadtreePacket
    QuadsetGather<QuadtreePacketItem>* gatherer1 =
      CreateGatherer("Test Gatherer 1");
    bool result = TestQuadtreePacketsFormat1(*gatherer1);
    delete gatherer1;
    return result;
  }

  bool TestQuadtreeBuilder2() {
    // Test the builder for Format2 (protocol buffer) of QuadtreePacket
    QuadsetGather<QuadtreePacketItem>* gatherer2 =
      CreateGatherer("Test Gatherer 2");
    bool result = TestQuadtreePacketsFormat2(gatherer2);
    delete gatherer2;
    return result;
  }

 private:
  void BuildExpectedRefs(const ExpectedQuadset *expected,
                         const std::uint64_t quadset_num,
                         KhQuadtreeDataReferenceGroup *ref_group) {
    // TODO: fill in correct versions, providers, channels
    for (int i = 0; i < expected->num_nodes; ++i) {
      QuadtreePath qt_path(
          qtpacket::QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(
              quadset_num, expected->nodes[i].subindex));
      if (expected->nodes[i].cache_node) {
        ref_group->qtp_refs->push_back(
            KhQuadtreeDataReference(qt_path, expected->nodes[i].version,
                                    0, 0));
      }
      const LayerTypeInfo& imagery_info = expected->nodes[i].imagery_info;
      const LayerTypeInfo& terrain_info = expected->nodes[i].terrain_info;
      const LayerTypeInfo& vector_info  = expected->nodes[i].vector_info;
      if (imagery_info.has_data) {
        ref_group->img_refs->push_back(
            KhQuadtreeDataReference(qt_path, imagery_info.version,
                                    keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY,
                                    imagery_info.provider));
      }
      if (terrain_info.has_data) {
        ref_group->ter_refs->push_back(
            KhQuadtreeDataReference(qt_path, terrain_info.version,
                                    keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN,
                                    terrain_info.provider));
      }
      if (vector_info.has_data) {
        std::uint16_t channel_id = 0;  // TODO: start recording this.
        ref_group->vec_refs->push_back(
            KhQuadtreeDataReference(qt_path, vector_info.version,
                                    channel_id, vector_info.provider));
      }
    }
  }

  // Compare the refs for the specified ref type (one of I, T, V)
  // expect_8bit_providers allows us to note whether we're using format1 which
  // uses 8bit providers or format2 which uses 32bit.
  int CompareRefs(const std::vector<KhQuadtreeDataReference> &expected_refs,
                  const std::vector<KhQuadtreeDataReference> &refs,
                  char ref_type,
                  bool expect_8bit_providers) {
    int errors = 0;
    if (!EXPECT(expected_refs.size() <= refs.size())) ++errors;
    for (size_t j = 0; j < std::min(expected_refs.size(), refs.size()); ++j) {
      if (!EXPECT_EQ(expected_refs[j].qt_path().AsString(),
                     refs[j].qt_path().AsString())) ++errors;
      if (!EXPECT_EQ(expected_refs[j].version(), refs[j].version())) ++errors;
      if (ref_type != 'V') {
        // TODO: start recording expected channels for vectors
        if (!EXPECT_EQ(expected_refs[j].channel(), refs[j].channel())) ++errors;
      }
      // Note: that format1 only supports 8 bit providers...must truncate
      // expected to match.
      std::uint16_t expected_provider = expected_refs[j].provider();
      if (expect_8bit_providers) {
        expected_provider &= 0xff;
      }
      if (!EXPECT_EQ(expected_provider, refs[j].provider())) ++errors;
    }
    if (errors) {
      for (size_t j = 0; j < expected_refs.size(); ++j ) {
        std::uint16_t expected_provider = expected_refs[j].provider();
        if (expect_8bit_providers) {
          expected_provider &= 0xff;
        }
        notify(NFY_VERBOSE, "    %c %s ver %u chan %u prov %u (expected)",
               ref_type,
               expected_refs[j].qt_path().AsString().c_str(),
               expected_refs[j].version(),
               expected_refs[j].channel(),
               expected_provider);
      }
      for (size_t j = 0; j < refs.size(); ++j ) {
        notify(NFY_VERBOSE, "    %c %s ver %u chan %u prov %u (actual)",
               ref_type,
               refs[j].qt_path().AsString().c_str(),
               refs[j].version(),
               refs[j].channel(),
               refs[j].provider());
      }
      errors++;
    }
    return errors;
  }

  bool TestQuadtreePacketsFormat1(QuadsetGather<QuadtreePacketItem> &gather) {
    int errors = 0;
    // Check each expected quadset
    for (size_t i = 0; i < kNumExpectedQuadsets; ++i) {
      const ExpectedQuadset *expected = &expected_quadsets[i];

      const std::uint64_t quadset_num
        = QuadtreeNumbering::TraversalPathToGlobalNodeNumber(
            expected->quadset_path);

      notify(NFY_DEBUG, "Expect quadset %llu, path \"%s\"",
             static_cast<unsigned long long>(quadset_num),
             expected->quadset_path.c_str());

      CHECK_EQ(quadset_num, gather.Current().quadset_num());

      // Build expected packet
      KhQuadTreePacket16 packet;
      ComputeQuadtreePacketFormat1(gather.Current(), kTestQTPacketVersion,
                                   packet);
printf("ToString\n");

      notify(NFY_VERBOSE, " Packet: %s",
             packet.ToString(quadset_num == 0, false /* don't care */).c_str());

      // Build expected references to packet

      std::vector<KhQuadtreeDataReference> expected_qtp_refs;
      std::vector<KhQuadtreeDataReference> expected_qtp2_refs;
      std::vector<KhQuadtreeDataReference> expected_img_refs;
      std::vector<KhQuadtreeDataReference> expected_ter_refs;
      std::vector<KhQuadtreeDataReference> expected_vec_refs;
      KhQuadtreeDataReferenceGroup expected_ref_group(
          &expected_qtp_refs, &expected_qtp2_refs, &expected_img_refs,
          &expected_ter_refs, &expected_vec_refs);
      BuildExpectedRefs(expected, quadset_num, &expected_ref_group);

      // Extract referred-to nodes
printf("GetDataReferences\n");

      std::vector<KhQuadtreeDataReference> qtp_refs;
      std::vector<KhQuadtreeDataReference> qtp2_refs;
      std::vector<KhQuadtreeDataReference> img_refs;
      std::vector<KhQuadtreeDataReference> ter_refs;
      std::vector<KhQuadtreeDataReference> vec_refs;
      KhQuadtreeDataReferenceGroup ref_group(
        &qtp_refs, &qtp2_refs, &img_refs, &ter_refs, &vec_refs);
      packet.GetDataReferences(&ref_group, expected->quadset_path,
                               // unspecified date
                               keyhole::kUnknownJpegCommentDate);
      notify(NFY_VERBOSE, " Referenced packets from %s:",
             expected->quadset_path.c_str());
      bool expect_8bit_providers = true;  // Format1 has 8 bit providers.
      errors += CompareRefs(expected_qtp_refs, qtp_refs, 'Q',
                            expect_8bit_providers);
      errors += CompareRefs(expected_img_refs, img_refs, 'I',
                            expect_8bit_providers);
      errors += CompareRefs(expected_ter_refs, ter_refs, 'T',
                            expect_8bit_providers);
      errors += CompareRefs(expected_vec_refs, vec_refs, 'V',
                            expect_8bit_providers);

      // Check nodes within packet
      for (int j = 0; j < expected->num_nodes; ++j) {
        const QuadtreePacketInfo *info = &expected->nodes[j];
        QuadtreePath node_path =
          QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(quadset_num,
                                                               info->subindex);
        const LayerTypeInfo& imagery_info = expected->nodes[j].imagery_info;
        const LayerTypeInfo& terrain_info = expected->nodes[j].terrain_info;
        const LayerTypeInfo& vector_info  = expected->nodes[j].vector_info;

        notify(NFY_DEBUG, "   Expect subindex %3d %c%c%c %c 0x%02x \"%s\"",
               info->subindex,
               imagery_info.has_data ? 'I' : ' ',
               terrain_info.has_data ? 'T' : ' ',
               vector_info.has_data ? 'V' : ' ',
               info->cache_node  ? 'C' : ' ',
               info->child_flags,
               node_path.AsString().c_str());

        // Get the info on this node in the quadset
        const KhQuadTreeQuantum16 *node =
          packet.FindNode(info->subindex, quadset_num == 0);

        if (!EXPECT(node)) ++errors;

        if (node) {
          // Check imagery, terrain, cache bits
          if (imagery_info.has_data) {
            if (!EXPECT_EQ(1, node->children.GetImageBit())) ++errors;
            if (!EXPECT_EQ(node->image_version, imagery_info.version)) ++errors;
            // NOTE: this format only supports 8bit for provider ids,
            // but our test id's are bigger (format2 supports 32bit
            if (!EXPECT_EQ(node->image_data_provider,
                           imagery_info.provider & 0xff))
                ++errors;
          } else {
            if (!EXPECT_EQ(0, node->children.GetImageBit()))
              ++errors;
          }

          if (terrain_info.has_data) {
            if (!EXPECT_EQ(1, node->children.GetTerrainBit())) ++errors;
            if (!EXPECT_EQ(node->terrain_version, terrain_info.version))
              ++errors;
            // NOTE: this format only supports 8bit for provider ids,
            // but our test id's are bigger (format2 supports 32bit
            if (!EXPECT_EQ(node->terrain_data_provider,
                           terrain_info.provider & 0xff))
              ++errors;
          } else {
            if (!EXPECT_EQ(0, node->children.GetTerrainBit())) ++errors;
          }

          if (vector_info.has_data) {
            if (!EXPECT_EQ(1, node->children.GetDrawableBit())) ++errors;
            if (!EXPECT_EQ(1u, node->num_channels())) ++errors;
            if (!EXPECT_EQ(node->channel_type[0], kTestVectorChannel)) ++errors;
            if (!EXPECT_EQ(node->channel_version[0], vector_info.version))
              ++errors;
          } else {
            if (!EXPECT_EQ(0, node->children.GetDrawableBit())) ++errors;
            if (!EXPECT_EQ(0u, node->num_channels())) ++errors;
          }

          if (info->cache_node) {
            if (!EXPECT_EQ(1, node->children.GetCacheNodeBit())) ++errors;
          } else {
            if (!EXPECT_EQ(0, node->children.GetCacheNodeBit())) ++errors;
          }

          // Check children flags
          if (!EXPECT_EQ(node->children.children & 0x0f, info->child_flags))
            ++errors;
        }
      }
      gather.Advance();
    }
    gather.Close();
    return errors == 0;
  }


  bool TestQuadtreePacketsFormat2(QuadsetGather<QuadtreePacketItem>*
                                  gather) {
    int errors = 0;
    // Check each expected quadset
    for (size_t i = 0; i < kNumExpectedQuadsets; ++i) {
      const ExpectedQuadset *expected = &expected_quadsets[i];

      const std::uint64_t quadset_num
        = QuadtreeNumbering::TraversalPathToGlobalNodeNumber(
            expected->quadset_path);

      notify(NFY_DEBUG, "Expect quadset %lu, path \"%s\"",
             static_cast<unsigned long>(quadset_num),
             expected->quadset_path.c_str());
      const QuadsetGroup<QuadtreePacketItem>& quadset_group = gather->Current();
      CHECK_EQ(quadset_num, quadset_group.quadset_num());
      const QuadtreeNumbering &numbering =
        QuadtreeNumbering::Numbering(quadset_num);

      // Build expected packet
      qtpacket::KhQuadTreePacketProtoBuf packet;
      ComputeQuadtreePacketFormat2(quadset_group, kTestQTPacketVersion,
                                   &packet);
      const keyhole::QuadtreePacket& packet_protocol_buffer =
        packet.ConstProtocolBuffer();

      notify(NFY_VERBOSE, " Packet: %s",
             packet_protocol_buffer.DebugString().c_str());

      // Build expected references to packet

      std::vector<KhQuadtreeDataReference> expected_qtp_refs;
      std::vector<KhQuadtreeDataReference> expected_qtp2_refs;
      std::vector<KhQuadtreeDataReference> expected_img_refs;
      std::vector<KhQuadtreeDataReference> expected_ter_refs;
      std::vector<KhQuadtreeDataReference> expected_vec_refs;
      KhQuadtreeDataReferenceGroup expected_ref_group(
          &expected_qtp_refs, &expected_qtp2_refs, &expected_img_refs,
          &expected_ter_refs, &expected_vec_refs);
      BuildExpectedRefs(expected, quadset_num, &expected_ref_group);


      int subindex_actual = packet_protocol_buffer.sparsequadtreenode(0).index();
      int subindex_expected = numbering.InorderToSubindex(0);
      EXPECT_EQ(subindex_actual, subindex_expected);
      // Extract referred-to nodes

printf("GetDataReferences\n");

      std::vector<KhQuadtreeDataReference> qtp_refs;
      std::vector<KhQuadtreeDataReference> qtp2_refs;
      std::vector<KhQuadtreeDataReference> img_refs;
      std::vector<KhQuadtreeDataReference> ter_refs;
      std::vector<KhQuadtreeDataReference> vec_refs;
      KhQuadtreeDataReferenceGroup ref_group(
        &qtp_refs, &qtp2_refs, &img_refs, &ter_refs, &vec_refs);
      // We want this call to GetDataReferences to get all imagery packet refs.
      keyhole::JpegCommentDate jpeg_date_match_all;
      jpeg_date_match_all.SetMatchAllDates(true);
      packet.GetDataReferences(&ref_group,
                               expected->quadset_path,
                               jpeg_date_match_all);
      notify(NFY_VERBOSE, " Referenced packets from %s:",
             expected->quadset_path.c_str());
      bool expect_8bit_providers = false;  // Format2 has 32 bit providers.
      errors += CompareRefs(expected_qtp2_refs, qtp2_refs, 'Q',
                            expect_8bit_providers);
      errors += CompareRefs(expected_img_refs, img_refs, 'I',
                            expect_8bit_providers);
      errors += CompareRefs(expected_ter_refs, ter_refs, 'T',
                            expect_8bit_providers);
      errors += CompareRefs(expected_vec_refs, vec_refs, 'V',
                            expect_8bit_providers);

      // Check nodes within packet
      for (int j = 0; j < expected->num_nodes; ++j) {
        const QuadtreePacketInfo *info = &expected->nodes[j];
        QuadtreePath node_path =
          QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(quadset_num,
                                                               info->subindex);
        const LayerTypeInfo& imagery_info = expected->nodes[j].imagery_info;
        const LayerTypeInfo& terrain_info = expected->nodes[j].terrain_info;
        const LayerTypeInfo& vector_info  = expected->nodes[j].vector_info;

        notify(NFY_DEBUG, "   Expect subindex %3d %c%c%c %c 0x%02x \"%s\"",
               info->subindex,
               imagery_info.has_data ? 'I' : ' ',
               terrain_info.has_data ? 'T' : ' ',
               vector_info.has_data ? 'V' : ' ',
               info->cache_node  ? 'C' : ' ',
               info->child_flags,
               node_path.AsString().c_str());

        // Get the info on this node in the quadset
        khDeleteGuard<KhQuadTreeNodeProtoBuf> node(
          TransferOwnership(packet.FindNode(info->subindex, quadset_num == 0)));
        if (!EXPECT(node)) ++errors;

        if (node) {
          // Check imagery, terrain, cache bits
          if (imagery_info.has_data) {
            const keyhole::QuadtreeLayer& layer =
              node->LayerOfType(keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY);
            if (!EXPECT_EQ(1, node->GetImageBit())) ++errors;
            if (!EXPECT_EQ(layer.layer_epoch(), imagery_info.version)) ++errors;
            if (!EXPECT_EQ(layer.provider(), imagery_info.provider)) ++errors;
          } else {
            if (!EXPECT_EQ(0, node->GetImageBit()))
              ++errors;
          }

          if (terrain_info.has_data) {
            const keyhole::QuadtreeLayer& layer =
              node->LayerOfType(keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN);
            if (!EXPECT_EQ(1, node->GetTerrainBit())) ++errors;
            if (!EXPECT_EQ(layer.layer_epoch(), terrain_info.version)) ++errors;
            if (!EXPECT_EQ(layer.provider(), terrain_info.provider)) ++errors;
          } else {
            if (!EXPECT_EQ(0, node->GetTerrainBit())) ++errors;
          }

          if (vector_info.has_data) {
            if (!EXPECT_EQ(1, node->GetDrawableBit())) ++errors;
            if (!EXPECT_EQ(1, node->proto_buf().channel_size())) ++errors;
            const keyhole::QuadtreeChannel& channel =
              node->proto_buf().channel(0);
            if (!EXPECT_EQ(channel.type(), kTestVectorChannel)) ++errors;
            if (!EXPECT_EQ(channel.channel_epoch(), vector_info.version))
              ++errors;
          } else {
            if (!EXPECT_EQ(0, node->GetDrawableBit())) ++errors;
            if (!EXPECT_EQ(0, node->proto_buf().channel_size())) ++errors;
          }

          if (info->cache_node) {
            if (!EXPECT_EQ(1, node->GetCacheNodeBit())) ++errors;
          } else {
            if (!EXPECT_EQ(0, node->GetCacheNodeBit())) ++errors;
          }

          // Check children flags
          if (!EXPECT_EQ(node->GetChildFlags(), info->child_flags))
            ++errors;
        }
      }
      gather->Advance();
    }
    gather->Close();
    return errors == 0;
  }
};

int main(int argc, char *argv[]) {
  int argn;
  bool debug = false;
  bool verbose = false;
  bool help = false;
  khGetopt options;
  options.flagOpt("debug", debug);
  options.flagOpt("verbose", verbose);
  options.flagOpt("help", help);
  options.flagOpt("?", help);

  if (!options.processAll(argc, argv, argn)  ||  help  ||  argn < argc) {
    std::cerr << "usage: " << std::string(argv[0])
              << " [-debug] [-verbose] [-help]"
              << std::endl;
    exit(1);
  }

  if (verbose) {
    setNotifyLevel(NFY_VERBOSE);
  } else if (debug) {
    setNotifyLevel(NFY_DEBUG);
  }

  QuadtreeBuilderUnitTest q;
  return q.Run();
}
