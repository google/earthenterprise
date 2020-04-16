/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

//
//
//
// Quadtree packet generation and merging.
// Fusion/server write those packets, client reads them.
// In 99.99% of the cases, both set of machines (client/server)
// run on x86 architecture cpus, which means little endian.
// The entire installed client base (1.0, 1.7, 2.2...) does not
// detect the endianness of the packets, hence for backwards
// compatibility, we must write multi-byte data fields as little endian.
// Byte swapping happens in serialization -
// see util/serialization/serializationutils-inl.h for details.
//
#ifndef COMMON_QTPACKET_QUADTREEPACKET_H__
#define COMMON_QTPACKET_QUADTREEPACKET_H__

#include <stdlib.h>
#include <string.h>
#include <set>
#include <string>
#include <vector>
#include <keyhole/jpeg_comment_date.h>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khGuard.h"
#include "common/khEndian.h"
#include "common/quadtreepath.h"
#include "protobuf/quadtreeset.pb.h"

namespace qtpacket {

struct KffDataProviderCRL;
class QuadtreeNumbering;
class Collector;
class CollectorFormat2;

// Data header.
// This is the header for all packet types.
// We only ship quadtree packets to client though.
// Other packets are directly read from disk and
// shipped over the network to client.
class KhDataHeader {
 public:
  std::uint32_t magic_id;
  std::uint32_t data_type_id;
  std::uint32_t version;
  std::int32_t num_instances;
  std::int32_t data_instance_size;
  std::int32_t data_buffer_offset;
  std::int32_t data_buffer_size;
  std::int32_t meta_buffer_size;

  void Push(EndianWriteBuffer &buf,
            std::int32_t databuffersize) const;
  void Pull(EndianReadBuffer &buf);
};

// Return structure for list of entries referred to from a quadtree packet.
// Allows assignment and copying to allow inclusion in STL container.

class KhQuadtreeDataReference {
 public:
  KhQuadtreeDataReference()
        : version_(0),
          channel_(0),
          provider_(0),
          jpeg_date_() {
  }
  KhQuadtreeDataReference(const QuadtreePath &qt_path,
                          std::uint16_t version,
                          std::uint16_t channel,
                          std::uint16_t provider)
      : qt_path_(qt_path),
        version_(version),
        channel_(channel),
        provider_(provider),
        jpeg_date_() {
  }
  KhQuadtreeDataReference(const QuadtreePath &qt_path,
                          std::uint16_t version,
                          std::uint16_t channel,
                          std::uint16_t provider,
                          const keyhole::JpegCommentDate& jpeg_date)
      : qt_path_(qt_path),
        version_(version),
        channel_(channel),
        provider_(provider),
        jpeg_date_(jpeg_date) {
  }
  inline QuadtreePath qt_path() const { return qt_path_; }
  inline std::uint16_t version() const { return version_; }
  inline std::uint16_t channel() const { return channel_; }
  inline std::uint16_t provider() const { return provider_; }
  inline const keyhole::JpegCommentDate& jpeg_date() const {
    return jpeg_date_;
  }
  inline bool IsHistoricalImagery() const {
    return channel_ == keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY &&
      !jpeg_date_.IsCompletelyUnknown();
  }

  inline void set_qt_path(const QuadtreePath &qt_path) { qt_path_ = qt_path; }
  inline void set_version(std::uint16_t version) { version_ = version; }
  inline void set_channel(std::uint16_t channel) { channel_ = channel; }
  inline void set_provider(std::uint16_t provider) { provider_ = provider; }

  inline bool operator==(const KhQuadtreeDataReference &other) const {
    return qt_path_ == other.qt_path_
        && channel_ == other.channel_
        && version_ == other.version_
        && provider_ == other.provider_;
  }
  inline bool operator!=(const KhQuadtreeDataReference &other) const {
    return !(*this == other);
  }
  inline bool operator>(const KhQuadtreeDataReference &other) const {
    return qt_path_ > other.qt_path_;
  }

  void Push(EndianWriteBuffer &buffer) const; // serializer
  void Pull(EndianReadBuffer &input);   // de-serializer

  static const size_t kSerialSize = sizeof(QuadtreePath) + 3 * sizeof(std::uint16_t);
 private:
  QuadtreePath qt_path_;                // path to the data
  std::uint16_t version_;
  std::uint16_t channel_;                      // zero for non-vector
  std::uint16_t provider_;
  keyhole::JpegCommentDate jpeg_date_;
};

struct KhQuadtreeDataReferenceGroup {
  KhQuadtreeDataReferenceGroup(std::vector<KhQuadtreeDataReference> *qtp_refs_,
      std::vector<KhQuadtreeDataReference> *qtp2_refs_,
      std::vector<KhQuadtreeDataReference> *img_refs_,
      std::vector<KhQuadtreeDataReference> *ter_refs_,
      std::vector<KhQuadtreeDataReference> *vec_refs_)
      : qtp_refs(qtp_refs_),
        qtp2_refs(qtp_refs_),
        img_refs(img_refs_),
        ter_refs(ter_refs_),
        vec_refs(vec_refs_) {
  }
  ~KhQuadtreeDataReferenceGroup() {}

  void Reset() {
    qtp_refs->clear();
    qtp2_refs->clear();
    img_refs->clear();
    ter_refs->clear();
    vec_refs->clear();
  }

  // Note: these vectors are not owned by this structure
  std::vector<KhQuadtreeDataReference> *qtp_refs;
  std::vector<KhQuadtreeDataReference> *qtp2_refs;
  std::vector<KhQuadtreeDataReference> *img_refs;
  std::vector<KhQuadtreeDataReference> *ter_refs;
  std::vector<KhQuadtreeDataReference> *vec_refs;
};

// KhDataPacket - base class for all packet types, including quadtree packets.
// You cannot use KhDataPacket directly - only a derived class.
// Provides storage for data instances.
// Derived classes implement serialization.
// We don't store a buffer pointer when saving since the buffer is provided
// by an external user. Saving to a temp buffer and growing it as needed
// only to realize that it won't fit in destination buffer later is also
// not very useful.
template <class InstanceType>
class KhDataPacket {
 public:
  // Save header and databuffer to a buffer and advances buffer pointer.
  // No size validation here. Caller needs to do it.
  void SaveHeader(EndianWriteBuffer &buffer, std::int32_t databuffersize) const;

  // accessors
  const KhDataHeader& packet_header() const { return packet_header_; }
  const InstanceType* data_instances() const { return data_instances_; }
  const InstanceType* GetDataInstanceP(int index) const {
    return &data_instances_[index];
  }

 protected:
  // Default constructor initializes fields to 0's.
  // Derived class should call InitHeader to allocate space
  // for data instances.
  KhDataPacket();
  ~KhDataPacket() { Clear(); }

  // Clear Data Packet - delete allocated buffers.
  void Clear();

  // Initialize Data Packet - specify how many instances we're going to store
  // which will allocate necessary space using new[].
  void InitHeader(int dataTypeID, int version, int numInstances);

  // Getter/Setter for data instances
  InstanceType* GetDataInstanceP(int index) {
    return &data_instances_[index];
  }

  // Calculate Size of Data Packet
  int get_save_size() const {
    return sizeof(KhDataHeader) +
      sizeof(InstanceType) * packet_header_.num_instances +
      packet_header_.data_buffer_size;
  }

 private:
  // Our data header
  KhDataHeader packet_header_;

  // TODO: support meta header
  // void* metaHeader;

  // buffer for data instances
  // We new[] these, thus it uses default ctor.
  // You probably need to call Init() on each instance
  InstanceType* data_instances_;
};

// Helper structure to store data information in quadnodes.
struct KhQuadTreeBTG {
  // This is actually a bitfield.
  std::uint8_t children;

  KhQuadTreeBTG() { children = 0; }

  void Clear() { children = 0; }

  // Those are bit for children nodes.
  int  GetBit(int bit) const { return (children & bytemaskBTG[bit]) != 0; }
  void SetBit(int bit)   { children |= bytemaskBTG[bit]; }
  void ClearBit(int bit) { children &= ~bytemaskBTG[bit]; }

  // CacheNodeBit indicates a node on last level.
  // client does not process children info for these,
  // since we don't actually have info for the children.
  // As a result, no need to set any of the children bits for
  // cache nodes, since client will simply disregard them.
  int  GetCacheNodeBit() const { return (children & bytemaskBTG[4]) != 0; }
  void SetCacheNodeBit()   { children |= bytemaskBTG[4]; }
  void ClearCacheNodeBit() { children &= ~bytemaskBTG[4]; }

  // Set if this node contains vector data.
  int  GetDrawableBit() const { return (children & bytemaskBTG[5]) != 0; }
  void SetDrawableBit()   { children |= bytemaskBTG[5]; }
  void ClearDrawableBit() { children &= ~bytemaskBTG[5]; }

  // Set if this node contains image data.
  int  GetImageBit() const { return (children & bytemaskBTG[6]) != 0; }
  void SetImageBit()   { children |= bytemaskBTG[6]; }
  void ClearImageBit() { children &= ~bytemaskBTG[6]; }

  // Set if this node contains terrain data.
  int  GetTerrainBit() const { return (children & bytemaskBTG[7]) != 0; }
  void SetTerrainBit()   { children |= bytemaskBTG[7]; }
  void ClearTerrainBit() { children &= ~bytemaskBTG[7]; }

  static const std::uint8_t bytemaskBTG[];
};

class ChannelInfo;

// Indicates which data a quadnode holds,
// along with its children information.
struct KhQuadTreeQuantum16 {
  static const size_t kImageNeighborCount = 8;
  static const size_t kSerialSize = 32; // size when serialized

  KhQuadTreeBTG children;

  std::uint16_t cnode_version;  // cachenode version
  std::uint16_t image_version;
  std::uint16_t terrain_version;

  // channel information.
  // channel_type and channel_version will be empty
  // if we don't have any channels.
  std::vector<std::uint16_t> channel_type;
  std::vector<std::uint16_t> channel_version;
  inline size_t num_channels() const {
    assert(channel_type.size() == channel_version.size());
    return channel_type.size();
  }

  // image neighbors info.
  std::uint8_t image_neighbors[kImageNeighborCount];

  // Data provider info.
  // Terrain data provider does not seem to be used.
  std::uint8_t image_data_provider;
  std::uint8_t terrain_data_provider;

  // Make default ctor public - datapacket needs it to call vector ctor.
  // Use one of the Init() functions to intialize the structure
  // with proper number of channels.
  KhQuadTreeQuantum16() { }
  ~KhQuadTreeQuantum16() { Cleanup(); }

  // Serializer (must be little endian!)
  void Push(EndianWriteBuffer &buffer) const;

  // Frees any allocated data - channels
  void Cleanup();

  // Return a readable string describing the quadtree node (matches the output
  // of the KhQuadTreeNodeProtoBuf class).
  void ToString(std::ostringstream &str,
                const int node_index,
                const QuadtreePath &qt_path,
                const int subindex) const;

  // Get references to all packets referred to in the quantum
  // (including other quadtree packets) and append to refs.
  void GetDataReferences(KhQuadtreeDataReferenceGroup *references,
                         const QuadtreePath &qt_path) const;

  // Return true if a layer of the specified type exists in this node.
  bool HasLayerOfType(keyhole::QuadtreeLayer::LayerType layer_type) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(KhQuadTreeQuantum16);
};

struct KffCopyrightMap;

// 16-bit quadtree packet structure.
// Provides merging functionnality
// as well as packet generation.
class KhQuadTreePacket16 : public KhDataPacket<KhQuadTreeQuantum16> {
 public:
  // We make the copy ctor private - need to make this one public
  KhQuadTreePacket16() {}
  ~KhQuadTreePacket16() { Cleanup(); }

  // Initialize quadtree packet with that many instances.
  // Also initializes header
  void Init(int num_instances);

  // Cleanup quadtree packet - delete instances.
  // Simply clear data packet. It's going to delete all the
  // instances we allocated.
  void Cleanup() { Clear(); }

  // Attempt to fill this object by parsing the given input.
  // Throws exception if not successful.
  void Pull(EndianReadBuffer &input);

  // Return a readable string describing the packet (matches the output
  // of the KhQuadTreePacketProtoBuf class).
  // detailed_info is ignored for this implementation currently.
  std::string ToString(bool root_node, bool detailed_info) const;

  // Return a vector of packets referenced by this packet.  References
  // are appended to the supplied vector.  The path_prefix should
  // normally specify the path to the root of the quadset.
  // jpeg_date is ignored/not applicable for KhQuadTreePacket16, it is
  // used to filter historical imagery by date. But this quadtree packet type
  // does not support historical imagery, so the value is ignored.
  void GetDataReferences(KhQuadtreeDataReferenceGroup *references,
                         const QuadtreePath &path_prefix,
                         const keyhole::JpegCommentDate& jpeg_date);

  // Get pointer to nth node in packet
  KhQuadTreeQuantum16* GetPtr(int num) {
    return GetDataInstanceP(num);
  }
  const KhQuadTreeQuantum16* GetPtr(int num) const {
    return GetDataInstanceP(num);
  }

  // Find and return the node with the given node subindex, or NULL if it
  // doesn't appear in this packet.
  const KhQuadTreeQuantum16* FindNode(int subindex, bool root_node) const;

  // Save packet to a buffer (client expects little endian)
  void Push(EndianWriteBuffer &buffer) const;

  // Return true if a layer of the specified type exists in this packet.
  bool HasLayerOfType(keyhole::QuadtreeLayer::LayerType layer_type) const;

 private:
  // Traverse node using a Collecting Parameter
  void Traverser(Collector *collector,
                 const QuadtreeNumbering &numbering,
                 int *node_indexp,
                 const QuadtreePath &qt_path) const;

  // Count nodes in packets
  void CountNodesQTPR(int* nodenum, int num, int* curlevel, int* curpos,
                      const KhQuadTreePacket16* qtpackets, int level) const;

  // Find and return the quadtree node with the given node index.
  // num points to the node that's currently under consideration.
  // path gives the quadtree traversal path to the current node.
  const KhQuadTreeQuantum16* FindNodeImpl(int node_index,
                                          const QuadtreeNumbering &numbering,
                                          int* num,
                                          const QuadtreePath qt_path) const;

  // id for this node type.
  static const int kTypeQuadTreePacket = 1;
  // size of a quadnode in the data buffer we ship.
  static const size_t kQuadNodeSize = KhQuadTreeQuantum16::kSerialSize;
  // version number
  static const int kQuadTreePacketVersion = 2;

  DISALLOW_COPY_AND_ASSIGN(KhQuadTreePacket16);
};

// KhQuadTreeNodeProtoBuf is a simple wrapper for a protocol buffer
// QuadtreeNode.
// This is only valid as long as the containing QuadtreePacket protocol buffer
// object remains in memory.
class KhQuadTreeNodeProtoBuf {
 public:
  // We make the copy ctor private - need to make this one public
  explicit KhQuadTreeNodeProtoBuf(const keyhole::QuadtreeNode& node) :
    node_(node) {}
  ~KhQuadTreeNodeProtoBuf() {}

  // Get the specified values out of the bit flags for the node.
  bool GetImageBit() const;
  bool GetTerrainBit() const;
  bool GetDrawableBit() const;
  bool GetCacheNodeBit() const;
  // Return true if the child bit i (in range 0...3) is set (indicating the
  // presence of a child quadtree node for the ith quadrant).
  // No range validation of "i" is performed.
  bool GetChildBit(int i) const;

  // Return the 4 bit flags representing the quadtree child occupancy bits.
  std::int32_t GetChildFlags() const;

  // Return a pointer to the first layer of the specified type from the
  // node's layer list. Asserts if none found.
  // This may not be so interesting when terrain and imagery supports multiple
  // layers.
  const keyhole::QuadtreeLayer& LayerOfType(
    keyhole::QuadtreeLayer::LayerType type) const;

  // Return a const reference to the underlying protocol buffer QuadtreeNode.
  const keyhole::QuadtreeNode& proto_buf() const { return node_; }

  // Return a readable string describing the quadtree node (matches the output
  // of the KhQuadTreeQuantum16 class).
  void ToString(std::ostringstream* str,
                const std::int32_t node_index,
                const QuadtreePath &qt_path,
                const std::int32_t subindex) const;

  // Return true if a layer of the specified type exists in this node.
  bool HasLayerOfType(keyhole::QuadtreeLayer::LayerType layer_type) const;

  // Add to the set any dates in DATED_IMAGERY layers in this node.
  void AddImageryDates(std::set<std::string>* dates) const;

 private:
  const keyhole::QuadtreeNode& node_;  // A const reference to the
                                       // protocol buffer node.
  DISALLOW_COPY_AND_ASSIGN(KhQuadTreeNodeProtoBuf);
};

// A simple wrapper for the QuadtreePacket (protocol buffer) class
// to provide some behaviors defined by KhQuadTreePacket16, mostly to simplify
// testing of the QuadtreePacket.
class KhQuadTreePacketProtoBuf {
 public:
  // We make the copy ctor private - need to make this one public
  KhQuadTreePacketProtoBuf() {}
  ~KhQuadTreePacketProtoBuf() {}

  // Return a vector of packets referenced by this packet.  References
  // are appended to the supplied vector.  The path_prefix should
  // normally specify the path to the root of the quadset.
  // If the jpeg_date is uninitialized, we skip all historical imagery,
  // otherwise, it will get the latest imagery <= the given date, unless
  // the jpeg_date is marked as MatchAllDates() in which case it will get
  // all imagery refs.
  void GetDataReferences(KhQuadtreeDataReferenceGroup *references,
                         const QuadtreePath &path_prefix,
                         const keyhole::JpegCommentDate& jpeg_date);

  // Find and return the node with the given node subindex, or NULL if it
  // doesn't appear in this packet.
  // Caller is responsible for deleting the node.
  KhQuadTreeNodeProtoBuf* FindNode(int subindex,
                                  bool root_node) const;

  // Return a readable string describing the packet (matches the output
  // of the KhQuadTreePacket16 class).
  // set detailed_info to true to print out the full protobuf
  std::string ToString(bool root_node, bool detailed_info) const;

  // Attempt to fill this object by parsing the given input.
  // Throws exception if not successful.
  void Pull(EndianReadBuffer& input);

  // Save packet to a buffer (client expects little endian)
  void Push(EndianWriteBuffer& buffer) const;

  // Return true if a layer of the specified type exists in this packet.
  bool HasLayerOfType(keyhole::QuadtreeLayer::LayerType layer_type) const;

  // Allow direct access to the protocol buffer internal representation.
  keyhole::QuadtreePacket& ProtocolBuffer() { return proto_buffer_; }
  // Allow const access to the protocol buffer internal representation.
  const keyhole::QuadtreePacket& ConstProtocolBuffer() { return proto_buffer_; }

 private:
  // Traverse node using a Collecting Parameter
  void Traverser(CollectorFormat2 *collector,
                 const QuadtreeNumbering &numbering,
                 int *node_index,
                 const QuadtreePath &qt_prefix_path,
                 const QuadtreePath &qt_path) const;

  // Find and return the quadtree node with the given node subindex.
  // current_node_index points to the node that's currently under consideration
  // (it is a raw index within the packet.
  // path gives the quadtree traversal path to the current node.
  // Caller is responsible for deleting the node.
  KhQuadTreeNodeProtoBuf* FindNodeImpl(int subindex,
                                      const QuadtreeNumbering &numbering,
                                      int* current_node_index,
                                      const QuadtreePath qt_path) const;
  // Get the "node_index"th node from the protocol buffer
  //   where node_index is between 0 and N-1
  // where N is the number of nodes in the buffer).
  // Wrap the protocol buffer with a KhQuadTreeNodeProtoBuf.
  // NULL is returned if the index is invalid.
  // Caller is responsible for deleting the KhQuadTreeNodeProtoBuf.
  KhQuadTreeNodeProtoBuf* GetNode(int node_index) const;

  keyhole::QuadtreePacket proto_buffer_;
  DISALLOW_COPY_AND_ASSIGN(KhQuadTreePacketProtoBuf);
};

// Helpful typedefs used by Format2 Quadtree Packets
typedef keyhole::QuadtreePacket_SparseQuadtreeNode SparseQuadtreeNode;

class SerializedKhQuadTreePacket16 {
 public:
  SerializedKhQuadTreePacket16(KhQuadTreePacket16 &packet) {
    LittleEndianWriteBuffer write_buffer_;
    write_buffer_ << packet;
    buffer_ =
      TransferOwnership(new LittleEndianReadBuffer(write_buffer_.data(),
                                                   write_buffer_.size()));
  }
  ~SerializedKhQuadTreePacket16() {}
  void Pull(KhQuadTreePacket16 *packet) {
    buffer_->Seek(0);
    *buffer_ >> *packet;
  }

  // Create dynamically allocated copy with safe pointer
  khTransferGuard<SerializedKhQuadTreePacket16> Copy() const {
    return TransferOwnership(new SerializedKhQuadTreePacket16(*this));
  }
 private:
  khDeleteGuard<EndianReadBuffer> buffer_;

  // Private constructor for Copy
  SerializedKhQuadTreePacket16(
      const SerializedKhQuadTreePacket16 &other)
      :buffer_(other.buffer_->NewBuffer(buffer_->data(),
                                        buffer_->size())) {
  }

  void operator=(const SerializedKhQuadTreePacket16 &); // disable assignment
};

struct KffCopyrightMap {
  // Store info for all 4 levels in qtp.
  // We store the providerId for each node at each level.
  std::uint8_t level00;
  std::uint8_t level01[4];
  std::uint8_t level02[4][4];
  std::uint8_t level03[4][4][4];


  bool CreateMap(const QuadtreePath& pos,
                 const KffDataProviderCRL* dataProviderList,
                 int dataProviderListLen);

  void Clear() { memset(&level00, 0, sizeof(KffCopyrightMap)); }

 private:
  // Must provide a valid dataProviderList array here.
  int CheckBbox(const double bbox[4], int level,
                const KffDataProviderCRL* dataProviderList,
                int dataProviderListLen) const;
};

} // namespace qtpacket

#endif  // COMMON_QTPACKET_QUADTREEPACKET_H__
