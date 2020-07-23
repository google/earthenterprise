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
//
// QuadtreePacket API implementation.
//
#include <stdlib.h>
#include <alloca.h>
#include <set>
#include <vector>
#include <iostream>
#include <sstream>
#include "common/khConstants.h"
#include "common/khEndian.h"
#include "common/qtpacket/quadtree_utils.h"
#include "common/qtpacket/quadtreepacket.h"
#include "common/qtpacket/quadtreepacketitem.h"
#include "common/qtpacket/kffdataprovidercrl.h"

namespace qtpacket {

// Moved these out of KhDataPacket class since they're independant
// of the template type.
static const std::uint32_t kKeyholeMagicId = 32301;

// Filler bytes used when serializing data packets.
static const std::uint8_t kByteFiller = 0;
static const std::uint16_t kWordFiller = 0;
// no need for a 4-byte filler yet. If we need an odd size
// such as 5 padding bytes, use a char array of 0's.

const std::uint8_t KhQuadTreeBTG::bytemaskBTG[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

// Used to do computations on traversal paths.  Quadtree packets are 5
// levels deep and have a strange numbering on the second level,
// except for the root, which has 4 levels and no strange numbering on
// the second level.
static QuadtreeNumbering tree_numbering(5, true);
static QuadtreeNumbering root_numbering(4, false);

// Define Collecting Parameter classes for traversals
class Collector {
 public:
  Collector() {}
  virtual ~Collector() {}
  virtual void Collect(const int node_index,
                       const QuadtreePath &qt_path,
                       const int subindex,
                       const KhQuadTreeQuantum16* node) = 0;
 private:
};

class ToStringCollector : public Collector {
 public:
  ToStringCollector() {}
  ~ToStringCollector() {}
  virtual void Collect(const int node_index,
                       const QuadtreePath &qt_path,
                       const int subindex,
                       const KhQuadTreeQuantum16* node) {
    node->ToString(ostr_, node_index, qt_path, subindex);
  }
  std::ostringstream &ostr() { return ostr_; }
 private:
  std::ostringstream ostr_;
  DISALLOW_COPY_AND_ASSIGN(ToStringCollector);
};

class DataReferenceCollector : public Collector {
 public:
  DataReferenceCollector(KhQuadtreeDataReferenceGroup *refs,
                         const QuadtreePath &path_prefix)
      : refs_(refs),
        path_prefix_(path_prefix) {
    assert(refs);
  }
  ~DataReferenceCollector() {}
  virtual void Collect(const int node_index,
                       const QuadtreePath &qt_path,
                       const int subindex,
                       const KhQuadTreeQuantum16* node) {
    node->GetDataReferences(refs_, path_prefix_ + qt_path);
  }
 private:
  KhQuadtreeDataReferenceGroup *refs_;  // not owned by this class
  const QuadtreePath path_prefix_;      // absolute path to this qt packet
  DISALLOW_COPY_AND_ASSIGN(DataReferenceCollector);
};

//////////////////////////////////////////////////
// KhQuadtreeDataReference
//
const size_t KhQuadtreeDataReference::kSerialSize;

void KhQuadtreeDataReference::Push(EndianWriteBuffer &buffer) const {
  buffer << qt_path_
         << version_
         << channel_
         << provider_;
}

void KhQuadtreeDataReference::Pull(EndianReadBuffer &input) {
  input >> qt_path_
        >> version_
        >> channel_
        >> provider_;
}

//////////////////////////////////////////////////
// KhDataheader
//
void KhDataHeader::Push(EndianWriteBuffer &buf,
                        std::int32_t databuffersize) const {
  assert(magic_id == kKeyholeMagicId);
  buf << magic_id
      << data_type_id
      << version
      << num_instances
      << data_instance_size
      << data_buffer_offset
      << databuffersize
      << meta_buffer_size;
}

void KhDataHeader::Pull(EndianReadBuffer &buf) {
  buf >> magic_id;
  if (magic_id != kKeyholeMagicId) {
    throw khSimpleException("KhDataheader::Pull: invalid magic_id: ")
      << magic_id;
  }
  buf >> data_type_id
      >> version
      >> num_instances
      >> data_instance_size
      >> data_buffer_offset
      >> data_buffer_size
      >> meta_buffer_size;
}


//////////////////////////////////////////////////
// KhDataPacket - header file uses <InstanceType>,
// using <T> here.

// Start with no instances
template <class T>
KhDataPacket<T>::KhDataPacket()
    :  // metaHeader(NULL),
      data_instances_(NULL) {
  // Clear out header
  memset(&packet_header_, 0, sizeof(packet_header_));
}

// Initialize data header and allocate instances
template <class T>
void KhDataPacket<T>::InitHeader(int dataTypeID, int version,
                              int numInstances) {
  packet_header_.magic_id = kKeyholeMagicId;
  packet_header_.data_type_id = dataTypeID;
  packet_header_.version = version;
  packet_header_.num_instances = numInstances;
  packet_header_.data_instance_size = T::kSerialSize;
  packet_header_.data_buffer_offset = 0;
  packet_header_.data_buffer_size = 0;
  packet_header_.meta_buffer_size = 0;

  // TODO: support meta header
  // metaHeader = metastruct;

  if (numInstances) {
    // Call default constructor.
    data_instances_ = new T[numInstances];
    // Save offset of data buffer in header
    // This data packet is going to be written to a buffer.
    packet_header_.data_buffer_offset =
      sizeof(packet_header_)
      + numInstances * packet_header_.data_instance_size;
  } else {
    data_instances_ = NULL;
  }
}

template <class T>
void KhDataPacket<T>::Clear() {
  // This is going to free memory allocated by data instances
  delete [] data_instances_;
  data_instances_ = NULL;
  // Reset the data header to 0's
  // This will make magic id and data type invalid,
  // and set both num_instances and data_buffer_size to 0.
  memset(&packet_header_, 0, sizeof(packet_header_));
}

// Save the datapacket header (client expects little endian)
template <class T>
void KhDataPacket<T>::SaveHeader(EndianWriteBuffer &buffer,
                                 std::int32_t databuffersize) const {
  packet_header_.Push(buffer, databuffersize);
}

// Instantiate the template that we use.
template class KhDataPacket<KhQuadTreeQuantum16>;

// Helper structure to store channel info + version
// Store entries into a vector.
// If number of vectors per quadtree packet grows dramatically,
// it could be worth using a hash_map.
class ChannelInfo {
  typedef std::pair<std::uint16_t, std::uint16_t> TypeVersion;
 public:
  // Start with 0 channels. More often than not,
  // we're building a quadnode for imagery or terrain db,
  // which has no channel. No need to waste memory.
  ChannelInfo() {}

  ~ChannelInfo() {}

  // Store a channel type and version.
  // Look for a channel with same type, and replace version if found.
  // Otherwise, simply create a new entry and add it to our list.
  void StoreVersion(std::uint16_t type, std::uint16_t version) {
    // cannot use find.
    for (std::vector<TypeVersion>::iterator p = channels_.begin();
         p != channels_.end();
         ++p) {
      if (p->first == type) {
        // found entry - patch version number
        p->second = version;
        return;
      }
    }
    // add a new entry if not found
    channels_.push_back(TypeVersion(type, version));
  }

  // Accessors
  int num_channels() const { return channels_.size(); }
  std::uint16_t channel_type(int i) const {
    return channels_[i].first;
  }
  std::uint16_t channel_version(int i) const {
    return channels_[i].second;
  }

  // Utility functions
  void clear() { channels_.clear(); }
  bool empty() const { return channels_.empty(); }

 private:
  // A vector should provide reasonably quick lookup,
  // as long as the number of entries is relatively small
  // I tried with a sparse_hash_table, it's actually a bit slower.
  std::vector<std::pair<std::uint16_t, std::uint16_t> > channels_;

  // Don't copy this structure.
  DISALLOW_COPY_AND_ASSIGN(ChannelInfo);
};


//
// KhQuadTreeQuantum16
//

// Free allocated buffers.
// Don't bother resetting the other data members.
// You need to call Init() on this structure if you want
// to use it again, it will correctly reset the data members.
void KhQuadTreeQuantum16::Cleanup() {
  channel_type.clear();
  channel_version.clear();
}

//
// 16-bit QuadTreePacket
//


// Save packet to a buffer (client understands little endian only!)
void KhQuadTreePacket16::Push(EndianWriteBuffer &buffer) const {
  // We cannot write the header just yet since it's incomplete.
  // savebuffer points to the instance buffer.
  EndianWriteBuffer::size_type savebuffer = sizeof(KhDataHeader);

  // Get a pointer into this buffer to the data section.
  EndianWriteBuffer::size_type databuffer = packet_header().data_buffer_offset;
  // keep track of where data buffer starts. We need offsets into it.
  EndianWriteBuffer::size_type databuffer_base = databuffer;

  // Keep track of data buffer usage
  EndianWriteBuffer::size_type databuffersize = 0;

  // Validate save buffer size against number of instances we need to write.
  assert(savebuffer + kQuadNodeSize * packet_header().num_instances
         <= databuffer);

  // data instances - quadtree quantum.
  // We need to convert pointers to offsets here.
  // We cannot simply write out the structure and replace pointers with offsets.
  // On a 64-bit machine, you're dead - the structure will be 8 bytes longer
  // and the client has no chance to understand it.
  // So do a bit more work and correctly serialize each quadnode.
  // This data is sent over the network and read by a windows client
  // without any form of byte swapping. It MUST be saved as little endian -
  // see explanations in quadtreepacket.h.
  // Ports of the client on other platforms (e.g Mac / PowerPC) will need
  // to deserialize from little endian.
  for (int i = 0; i < packet_header().num_instances; i++) {
    const KhQuadTreeQuantum16* quantum = GetPtr(i);

    std::int32_t type_offset = 0;
    std::int32_t version_offset = 0;

    // Skip this part if we don't have any channels.
    std::uint16_t num_channels = quantum->channel_type.size();
    if (num_channels != 0) {
      assert(num_channels == quantum->channel_version.size());

      buffer.Seek(databuffer);
      // We need to write the data buffer part first, since
      // we need the offsets in the data instance.
      // Validate data buffer size.
      databuffersize += 2 * (num_channels * sizeof(std::uint16_t));

      // Write channel types to data buffer - save the offset
      type_offset = buffer.CurrPos() - databuffer_base;
      for (size_t j = 0; j < num_channels; ++j) {
        buffer << quantum->channel_type[j];
      }

      // Write channel versions to data buffer - save the offset
      version_offset = buffer.CurrPos() - databuffer_base;
      for (size_t j = 0; j < num_channels; ++j) {
        buffer << quantum->channel_version[j];
      }
      databuffer = buffer.CurrPos();
    }

    buffer.Seek(savebuffer);
    // Now serialize the quadtree node - no endian for uint8.
    buffer << quantum->children.children;
    // One filler byte - next field is aligned on 2 bytes
    buffer << kByteFiller;
    // Write versions and number of channels
    buffer << quantum->cnode_version;
    buffer << quantum->image_version;
    buffer << quantum->terrain_version;
    buffer << num_channels;
    // Another 2 byte filler. The offsets have to be aligned on 4 bytes.
    // We're currently at 10. Endian does not matter, we're writing 0's.
    buffer << kWordFiller;
    // Write offsets instead of pointers.
    buffer << type_offset;
    buffer << version_offset;
    // Write image neighbors as a blob of data - 8 bytes
    buffer.rawwrite(quantum->image_neighbors, sizeof(quantum->image_neighbors));
    // Write data provider info - no endian for uint8.
    buffer << quantum->image_data_provider;
    buffer << quantum->terrain_data_provider;
    // Another 2 filler bytes to end the structure.
    // Next structure needs to be aligned on 4 bytes as well,
    // we only wrote 30 bytes this far.
    buffer << kWordFiller;

    // Make sure we wrote exactly what we said
    assert(buffer.CurrPos() - savebuffer == kQuadNodeSize);
    savebuffer = buffer.CurrPos();
  }
  // We better have filled the data instance portion of the buffer.
  assert(savebuffer == databuffer_base);
  // Update data buffer size in our header - it was the only missing field
  assert(databuffersize == databuffer - databuffer_base);
  //SetDataBufferSize(databuffersize);

  // Write header at beginning of buffer.
  // We have all the fields now.
  buffer.Seek(0);
  SaveHeader(buffer, databuffersize);
  assert(buffer.CurrPos() == sizeof(KhDataHeader));

  // Leave pointer at end of buffer
  buffer.Seek(buffer.size());
}

// initialize data header with number of instances
void KhQuadTreePacket16::Init(int num) {
  InitHeader(kTypeQuadTreePacket, kQuadTreePacketVersion, num);
};

// Count the number of non-empty nodes in a quadtree packet.
// We need to use a recursive function due to the ordering of
// quadnodes in each qtp. curlevel and curpos arrays keep track
// of which quadnodes have information for current node.
void KhQuadTreePacket16::CountNodesQTPR(int* nodenum, int num,
                                        int* curlevel, int* curpos,
                                        const KhQuadTreePacket16* qtpackets,
                                        int level) const {
  (*nodenum)++;
  int nodepos[num];

  memcpy(nodepos, curpos, sizeof(nodepos));

  // Traverse Children
  for (int i = 0; i < 4; i++) {
    // Check for children in any of the packets
    bool flag = false;
    for (int j = 0; j < num; j++) {
      // Check level first. Otherwise we'll do GetPtr()
      // on a potentially invalid index.
      if (curlevel[j] == level &&
          qtpackets[j].GetPtr(nodepos[j])->children.GetBit(i)) {
        curlevel[j]++;
        curpos[j]++;
        flag = true;
      }
    }
    if (flag)
      CountNodesQTPR(nodenum, num, curlevel, curpos, qtpackets, level+1);
  }

  for (int j = 0; j < num; j++)
    if (curlevel[j] >= level)
      curlevel[j] = level - 1;
}


void KhQuadTreePacket16::Pull(EndianReadBuffer &input) {
  Clear();
  size_t size = input.size();

  // Parse header
  if (size < sizeof(KhDataHeader)) {
    throw khSimpleException(
        "KhQuadTreePacket16::Pull: packet shorter than header size");
  }

  KhDataHeader header;
  header.Pull(input);

  // Enough space to read instances?
  if (size < sizeof(KhDataHeader) + kQuadNodeSize * header.num_instances) {
    throw khSimpleException("KhQuadTreePacket16::Pull: ")
      << "packet size (" << size
      << ") too short, should be " << kQuadNodeSize * header.num_instances;
  }

  Init(header.num_instances);

  for (int i = 0; i < header.num_instances; i++) {
    KhQuadTreeQuantum16* quantum = GetDataInstanceP(i);

    input >> quantum->children.children;

    std::uint8_t junk8;                        // filler for 2 byte alignment
    input >> junk8;

    input >> quantum->cnode_version;
    input >> quantum->image_version;
    input >> quantum->terrain_version;

    std::uint16_t num_channels;
    input >> num_channels;

    std::uint16_t junk16;                      // fill to 4 byte boundary
    input >> junk16;
    std::int32_t type_offset;
    input >> type_offset;
    std::int32_t version_offset;
    input >> version_offset;

    input.rawread(quantum->image_neighbors,
                  KhQuadTreeQuantum16::kImageNeighborCount);

    // Read data provider info
    input >> quantum->image_data_provider;
    input >> quantum->terrain_data_provider;
    input >> junk16;                    // 2 fill bytes at end

    assert(input.CurrPos() ==
           sizeof(KhDataHeader) + kQuadNodeSize * (i+1));

    // Read channels
    if (num_channels > 0) {
      khDeleteGuard<EndianReadBuffer>
        channel_type_buf(
            input.NewBuffer(
                input.data() + header.data_buffer_offset + type_offset,
                sizeof(std::uint16_t) * num_channels));
      quantum->channel_type.resize(num_channels);

      for (std::uint16_t j = 0; j < num_channels; ++j) {
        *channel_type_buf >> quantum->channel_type[j];
      }

      khDeleteGuard<EndianReadBuffer>
        channel_version_buf(
            input.NewBuffer(
                input.data() + header.data_buffer_offset + version_offset,
                sizeof(std::uint16_t) * num_channels));
      quantum->channel_version.resize(num_channels);

      for (std::uint16_t j = 0; j < num_channels; ++j) {
        *channel_version_buf >> quantum->channel_version[j];
      }
    } else {
      quantum->channel_type.clear();
      quantum->channel_version.clear();
    }
  }
}

std::string KhQuadTreePacket16::ToString(bool root_node,
                                         bool detailed_info) const {
  ToStringCollector collector;
  collector.ostr() << packet_header().num_instances << " nodes" << std::endl;
  int node_index = 0;
  Traverser(&collector,
            root_node ? root_numbering : tree_numbering,
            &node_index,
            QuadtreePath());
  return collector.ostr().str();
}

// Traverser - traverse the nodes of the quadtree packet with a
// Collecting Parameter
void KhQuadTreePacket16::Traverser(Collector *collector,
                                   const QuadtreeNumbering &numbering,
                                   int *node_indexp,
                                   const QuadtreePath &qt_path) const {
  if (*node_indexp >= packet_header().num_instances)
    return;

  const KhQuadTreeQuantum16* node = GetPtr(*node_indexp);
  int subindex =
    numbering.InorderToSubindex(numbering.TraversalPathToInorder(qt_path));
  collector->Collect(*node_indexp, qt_path, subindex, node);

  // Descend to children, using children bits in the packet
  for (int i = 0; i < 4; ++i) {
    if (node->children.GetBit(i)) {
      const QuadtreePath new_qt_path(qt_path.Child(i));
      *node_indexp += 1;
      Traverser(collector, numbering, node_indexp, new_qt_path);
    }
  }
}


// ToString() - convert node contents to printable string
void KhQuadTreeQuantum16::ToString(std::ostringstream &str,
                                   const int node_index,
                                   const QuadtreePath &qt_path,
                                   const int subindex) const {
  str << "  node " << node_index
      << "  s" << subindex
      << " \"" << qt_path.AsString() << "\""
      << "  iv = " << image_version
      << ", ip = " << (std::uint16_t)image_data_provider
      << ", tv = " << terrain_version
      << ", tp = " << (std::uint16_t)terrain_data_provider
      << ", c = "  << cnode_version
      << ", flags = 0x" << std::hex
      << static_cast<unsigned>(children.children) << std::dec
      << "(";

  if (children.GetImageBit())
    str << "I";
  if (children.GetTerrainBit())
    str << "T";
  if (children.GetDrawableBit())
    str << "V";
  if (children.GetCacheNodeBit())
    str << "C";
  for (int j = 0; j < 4; ++j) {
    if (children.GetBit(j))
      str << "0123"[j];
  }
  str << ")" << std::endl;

  std::uint16_t num_channels = channel_type.size();
  for (std::uint16_t j = 0; j < num_channels; ++j) {
    str << "    V" << j << ": layer = " << channel_type[j]
        << ", version = " << channel_version[j] << std::endl;
  }
}

const KhQuadTreeQuantum16* KhQuadTreePacket16::FindNode(int node_index,
                                                        bool root_node) const {
  int num = 0;
  QuadtreePath qt_path;
  return FindNodeImpl(node_index,
                      root_node ? root_numbering : tree_numbering,
                      &num,
                      qt_path);
}

const KhQuadTreeQuantum16* KhQuadTreePacket16::FindNodeImpl(
    int node_index,
    const QuadtreeNumbering &numbering,
    int* num,
    const QuadtreePath qt_path) const {
  if (*num >= packet_header().num_instances)
    return NULL;

  const KhQuadTreeQuantum16* node = GetPtr(*num);
  if (node_index == numbering.InorderToSubindex(
          numbering.TraversalPathToInorder(qt_path)))
    return node;

  // Descend to children, using children bits in the packet
  for (int i = 0; i < 4; ++i) {
    if (node->children.GetBit(i)) {
      const QuadtreePath new_qt_path(qt_path.Child(i));
      *num += 1;
      const KhQuadTreeQuantum16* child_node =
        FindNodeImpl(node_index, numbering, num, new_qt_path);
      if (child_node != NULL)
        return child_node;
    }
  }
  return NULL;
}

// Get references to all packets referred to in the quantum
// (including other quadtree packets) and append to result.

void KhQuadTreeQuantum16::GetDataReferences(
    KhQuadtreeDataReferenceGroup *references,
    const QuadtreePath &qt_path) const {
  if (references->qtp_refs && children.GetCacheNodeBit()) {
    references->qtp_refs->push_back(
        KhQuadtreeDataReference(qt_path, cnode_version, 0, 0));
  }
  if (references->img_refs && children.GetImageBit()) {
    references->img_refs->push_back(
        KhQuadtreeDataReference(qt_path,
                                image_version,
                                keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY,
                                image_data_provider));
  }
  if (references->ter_refs && children.GetTerrainBit()) {
    references->ter_refs->push_back(
        KhQuadtreeDataReference(qt_path,
                                terrain_version,
                                keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN,
                                terrain_data_provider));
  }
  if (references->vec_refs && children.GetDrawableBit()) {
    std::uint16_t num_channels = channel_type.size();
    for (std::uint16_t j = 0; j < num_channels; ++j) {
      references->vec_refs->push_back(
          KhQuadtreeDataReference(qt_path,
                                  channel_version[j],
                                  channel_type[j],
                                  0));
    }
  }
}

bool KhQuadTreeQuantum16::HasLayerOfType(
  keyhole::QuadtreeLayer::LayerType layer_type) const {
  if (children.GetImageBit() &&
      layer_type == keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY) {
    return true;
  }
  if (children.GetTerrainBit() &&
      layer_type == keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN) {
    return true;
  }
  if (children.GetDrawableBit() &&
      layer_type == keyhole::QuadtreeLayer::LAYER_TYPE_VECTOR) {
    return true;
  }
  return false;
}

void KhQuadTreePacket16::GetDataReferences(
    KhQuadtreeDataReferenceGroup *references,
    const QuadtreePath &path_prefix,
    const keyhole::JpegCommentDate& jpeg_date) {
  DataReferenceCollector collector(references, path_prefix);
  int node_index = 0;
  Traverser(&collector,
            (path_prefix.Level() == 0) ? root_numbering : tree_numbering,
            &node_index,
            QuadtreePath());
}

bool KhQuadTreePacket16::HasLayerOfType(
  keyhole::QuadtreeLayer::LayerType layer_type) const {
  if (layer_type == keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY) {
    return false;
  }

  int node_count = packet_header().num_instances;
  for (int i = 0; i < node_count; ++i) {
    const KhQuadTreeQuantum16* node = GetPtr(i);
    if (node->HasLayerOfType(layer_type)) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////
// KhQuadTreePacketProtoBuf
// Define Collecting Parameter classes for traversals for this version of
// protocol buffers.
// TODO: there should be interfaces for Traversable and Collector
class CollectorFormat2 {
 public:
  CollectorFormat2() {}
  virtual ~CollectorFormat2() {}
  virtual void Collect(const int node_index,
                       const QuadtreePath &qt_path,
                       const int subindex,
                       const KhQuadTreeNodeProtoBuf& node) = 0;
 private:
};

class DataReferenceCollectorFormat2 : public CollectorFormat2 {
 public:
  // The jpeg_date, if not empty, indicates that we only collect
  // historical imagery at that date or earlier.
  // If empty, we skip historical imagery. If it's "all", we'll return all
  // the historical imagery tiles.
  DataReferenceCollectorFormat2(KhQuadtreeDataReferenceGroup *references,
                                const QuadtreePath &path_prefix,
                                const keyhole::JpegCommentDate& jpeg_date)
      : refs_(references),
        path_prefix_(path_prefix),
        jpeg_date_(jpeg_date) {
    assert(references);
   }
  ~DataReferenceCollectorFormat2() {}
  virtual void Collect(const int node_index,
                       const QuadtreePath &qt_path,
                       const int subindex,
                       const KhQuadTreeNodeProtoBuf&  node) {
    QuadtreePath qt_path_full = path_prefix_ + qt_path;
    if (refs_->qtp2_refs && node.GetCacheNodeBit()) {
      refs_->qtp2_refs->push_back(
          KhQuadtreeDataReference(qt_path_full,
                                  node.proto_buf().cache_node_epoch(), 0, 0));
    }
    int num_layers = node.proto_buf().layer_size();
    // May want to skip non-historical imagery if the caller requests only a
    // specific date.
    bool get_only_historical_imagery =
      (!jpeg_date_.IsCompletelyUnknown()) && !jpeg_date_.MatchAllDates();

    for (int i = 0; i < num_layers; ++i) {
      const keyhole::QuadtreeLayer& layer = node.proto_buf().layer(i);
      switch (layer.type()) {
      case keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY:
        if (refs_->img_refs && !get_only_historical_imagery) {
          refs_->img_refs->push_back(
              KhQuadtreeDataReference(qt_path_full,
                                      layer.layer_epoch(),
                                      keyhole::QuadtreeLayer::
                                      LAYER_TYPE_IMAGERY,
                                      layer.provider()));
        }
        break;
      case keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN:
        if (refs_->ter_refs) {
          refs_->ter_refs->push_back(
              KhQuadtreeDataReference(qt_path_full,
                                      layer.layer_epoch(),
                                      keyhole::QuadtreeLayer::
                                      LAYER_TYPE_TERRAIN,
                                      layer.provider()));
        }
        break;
      case keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY:
        if (refs_->img_refs && !jpeg_date_.IsCompletelyUnknown()) {
          const keyhole::QuadtreeImageryDates& dates_layer =
            layer.dates_layer();

          int num_dates_layers = dates_layer.dated_tile_size();

          // Dated tiles are stored earliest first...we want the latest
          // tile that is <= our requested date.  Traverse in reverse order.
          for (int j = num_dates_layers-1; j >= 0; --j) {
             const keyhole::QuadtreeImageryDatedTile& dated_tile =
               dates_layer.dated_tile(j);
             // Convert to a JpegCommentDate format, then to hex
             // which is the format used by the crawler.
             keyhole::JpegCommentDate jpeg_date(dated_tile.date());
             if (!jpeg_date_.MatchAllDates()) {
               continue;  // Unless we're getting all tiles, skip these.
             }
             // Only collect the tile ref whose date is <= than the
             // requested date...unless "MatchAllDates" is requested.
             if (jpeg_date_.MatchAllDates() || jpeg_date < jpeg_date_) {
               refs_->img_refs->push_back(KhQuadtreeDataReference(qt_path_full,
                                           dated_tile.dated_tile_epoch(),
                                           keyhole::QuadtreeLayer::
                                             LAYER_TYPE_IMAGERY_HISTORY,
                                           dated_tile.provider(),
                                           jpeg_date));
               if (!jpeg_date_.MatchAllDates())
                 break;  // Only want the latest tile.
             }
           }
        }
        break;
      case keyhole::QuadtreeLayer::LAYER_TYPE_VECTOR:
        // Skip: Vectors are handled below by channels.
      default:
        break;
      }
    }
    // Create refs from the active channels.
    if (refs_->vec_refs) {
      int num_channels = node.proto_buf().channel_size();
      for (int j = 0; j < num_channels; ++j) {
        const keyhole::QuadtreeChannel& channel = node.proto_buf().channel(j);
        refs_->vec_refs->push_back(
            KhQuadtreeDataReference(qt_path_full,
                                    channel.channel_epoch(),
                                    channel.type(),
                                    0));
      }
    }
  }
 private:
  KhQuadtreeDataReferenceGroup *refs_;  // not owned by this class
  const QuadtreePath path_prefix_;      // absolute path to this qt packet
  const keyhole::JpegCommentDate jpeg_date_;  // only collect dates at this
                                              // or earlier.
  DISALLOW_COPY_AND_ASSIGN(DataReferenceCollectorFormat2);
};


void KhQuadTreePacketProtoBuf::Traverser(CollectorFormat2 *collector,
                                   const QuadtreeNumbering &numbering,
                                   int *node_index,
                                   const QuadtreePath &qt_prefix_path,
                                   const QuadtreePath &qt_path) const {
  if (*node_index >= proto_buffer_.sparsequadtreenode_size())
    return;

  khDeleteGuard<KhQuadTreeNodeProtoBuf> node(
    TransferOwnership(GetNode(*node_index)));
  if (node.operator->() == NULL)
    return;
  int subindex =
    numbering.InorderToSubindex(numbering.TraversalPathToInorder(qt_path));
  collector->Collect(*node_index, qt_path, subindex, *node);

  // Descend to children, using children bits in the packet
  for (int child_index = 0; child_index < 4; ++child_index) {
    // Child bits are in the 4 lowest bits of flags.
    if (node->GetChildBit(child_index)) {
      const QuadtreePath new_qt_path(qt_path.Child(child_index));
      *node_index += 1;
      Traverser(collector, numbering, node_index, qt_prefix_path, new_qt_path);
    }
  }
}

void KhQuadTreePacketProtoBuf::GetDataReferences(
    KhQuadtreeDataReferenceGroup *references,
    const QuadtreePath &path_prefix,
    const keyhole::JpegCommentDate& jpeg_date) {
  DataReferenceCollectorFormat2 collector(references, path_prefix, jpeg_date);
  int node_index = 0;
  Traverser(&collector,
            (path_prefix.Level() == 0) ? root_numbering : tree_numbering,
            &node_index,
            path_prefix,
            QuadtreePath());
}

KhQuadTreeNodeProtoBuf* KhQuadTreePacketProtoBuf::GetNode(int node_index)
const {
  if (node_index >= proto_buffer_.sparsequadtreenode_size() || node_index < 0)
    return NULL;
  const SparseQuadtreeNode& sparse_quadtree_node =
    proto_buffer_.sparsequadtreenode(node_index);
  return new KhQuadTreeNodeProtoBuf(sparse_quadtree_node.node());
}

KhQuadTreeNodeProtoBuf*
KhQuadTreePacketProtoBuf::FindNode(int subindex,
                                   bool root_node) const {
  int num = 0;
  QuadtreePath qt_path;
  return FindNodeImpl(subindex,
                      root_node ? root_numbering : tree_numbering,
                      &num,
                      qt_path);
}

KhQuadTreeNodeProtoBuf* KhQuadTreePacketProtoBuf::FindNodeImpl(
    int subindex,
    const QuadtreeNumbering &numbering,
    int* current_node_index,
    const QuadtreePath qt_path) const {
  if (*current_node_index >= proto_buffer_.sparsequadtreenode_size())
    return NULL;

  khDeleteGuard<KhQuadTreeNodeProtoBuf> node(
    TransferOwnership(GetNode(*current_node_index)));
  if (subindex == numbering.InorderToSubindex(
          numbering.TraversalPathToInorder(qt_path)))
    return node.take();  // Give ownership of node to the caller.

  // Descend to children, using children bits in the packet
  for (int i = 0; i < 4; ++i) {
    if (node->GetChildBit(i)) {
      const QuadtreePath new_qt_path(qt_path.Child(i));
      *current_node_index += 1;
      KhQuadTreeNodeProtoBuf* child_node =
        FindNodeImpl(subindex, numbering, current_node_index, new_qt_path);
      if (child_node != NULL)
        return child_node;
    }
  }
  return NULL;
}

// Save packet to a buffer (client understands little endian only!)
void KhQuadTreePacketProtoBuf::Push(EndianWriteBuffer& buffer) const {
  // Simply write the serialized packet. No headers for Protocol Buffer packets!
  std::string protocol_buffer_string;
  proto_buffer_.SerializeToString(&protocol_buffer_string);
  // serialize the string representation to the LittleEndianWriteBuffer
  // Note: << serializes to a 0-terminated string...no good for us!
  buffer.rawwrite(protocol_buffer_string.data(), protocol_buffer_string.size());
  // Leave pointer at end of buffer
  buffer.Seek(buffer.size());
}

void KhQuadTreePacketProtoBuf::Pull(EndianReadBuffer& input) {
  size_t size = input.size();
  if (size <= 0) {
    throw khSimpleException("KhQuadTreePacketProtoBuf::Pull: empty packet");
  }
  // Grab the packet. No headers for Protocol Buffer packets!
  std::string buffer;
  buffer.resize(size);

  input.rawread(static_cast<void*>(&buffer[0]), size);
  proto_buffer_.ParseFromString(buffer);
}


std::string KhQuadTreePacketProtoBuf::ToString(bool root_node,
                                               bool detailed_info) const {
  std::ostringstream stream;
  unsigned int node_count = proto_buffer_.sparsequadtreenode_size();
  if (detailed_info) {  // Separate the summary from details.
    stream <<
      "------------------------------------------------------------------"
       << std::endl << "Packet Contents Summary: " << std::endl;
  }
  stream << node_count << " nodes" << std::endl;

  const QuadtreeNumbering& numbering = root_node ?
    root_numbering : tree_numbering;
  std::set<std::string> imagery_dates;  // Collect the set of imagery dates
                                        // in this node if any.

  for (unsigned int node_index = 0; node_index < node_count; ++node_index) {
    const keyhole::QuadtreePacket_SparseQuadtreeNode& sparse_node =
      proto_buffer_.sparsequadtreenode(node_index);
    KhQuadTreeNodeProtoBuf* node = GetNode(node_index);
    QuadtreePath qt_path =
      numbering.SubindexToTraversalPath(sparse_node.index());
    node->ToString(&stream, node_index, qt_path, sparse_node.index());
    if (detailed_info) {
      node->AddImageryDates(&imagery_dates);
    }
  }
  if (detailed_info) {
    stream <<
      "------------------------------------------------------------------"
      << std::endl << "Protocol Buffer Contents:" << std::endl
      << proto_buffer_.DebugString() << std::endl;

    if (!imagery_dates.empty()) {
      stream
        << "------------------------------------------------------------------"
        << std::endl << "Dated Imagery Layers" << std::endl;

        std::set<std::string>::const_iterator iter = imagery_dates.begin();
        for (; iter != imagery_dates.end(); ++iter) {
          stream << "  " << *iter << std::endl;
        }
    }
  }
  return stream.str();
}

bool KhQuadTreePacketProtoBuf::HasLayerOfType(
  keyhole::QuadtreeLayer::LayerType layer_type) const {
  unsigned int node_count = proto_buffer_.sparsequadtreenode_size();
  for (unsigned int i = 0; i < node_count; ++i) {
    KhQuadTreeNodeProtoBuf* node = GetNode(i);
    if (node->HasLayerOfType(layer_type)) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////
// KhQuadTreeNodeProtoBuf
// Simple utilities to wrap keyhole::QuadtreeNode
bool KhQuadTreeNodeProtoBuf::GetImageBit() const {
  std::int32_t flags = node_.flags();
  return flags & (1 << keyhole::QuadtreeNode::NODE_FLAGS_IMAGE_BIT);
}
bool KhQuadTreeNodeProtoBuf::GetTerrainBit() const {
  std::int32_t flags = node_.flags();
  return flags & (1 << keyhole::QuadtreeNode::NODE_FLAGS_TERRAIN_BIT);
}
bool KhQuadTreeNodeProtoBuf::GetDrawableBit() const {
  std::int32_t flags = node_.flags();
  return flags & (1 << keyhole::QuadtreeNode::NODE_FLAGS_DRAWABLE_BIT);
}
bool KhQuadTreeNodeProtoBuf::GetCacheNodeBit() const {
  std::int32_t flags = node_.flags();
  return flags & (1 << keyhole::QuadtreeNode::NODE_FLAGS_CACHE_BIT);
}
bool KhQuadTreeNodeProtoBuf::GetChildBit(int i) const {
  std::int32_t flags = node_.flags();
  return flags & (1 << i);
}
std::int32_t KhQuadTreeNodeProtoBuf::GetChildFlags() const {
  return node_.flags() & 0xf;
}

const keyhole::QuadtreeLayer kEmptyQuadtreeLayer;

const keyhole::QuadtreeLayer& KhQuadTreeNodeProtoBuf::LayerOfType(
  keyhole::QuadtreeLayer::LayerType type) const {
  int num_layers = node_.layer_size();
  for (int i = 0; i < num_layers; ++i) {
    const keyhole::QuadtreeLayer& layer = node_.layer(i);
    if (layer.type() == type)
      return layer;
  }
  assert(false);
  return kEmptyQuadtreeLayer;
}

bool KhQuadTreeNodeProtoBuf::HasLayerOfType(
  keyhole::QuadtreeLayer::LayerType layer_type) const {
  int num_layers = node_.layer_size();
  for (int i = 0; i < num_layers; ++i) {
    const keyhole::QuadtreeLayer& layer = node_.layer(i);
    if (layer.type() == layer_type)
      return true;
  }
  return false;
}

void KhQuadTreeNodeProtoBuf::AddImageryDates(std::set<std::string>* dates)
const {
  int num_layers = node_.layer_size();
  for (int i = 0; i < num_layers; ++i) {
    const keyhole::QuadtreeLayer& layer = node_.layer(i);
    if (layer.type() == keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY) {
      const keyhole::QuadtreeImageryDates& date_buffer = layer.dates_layer();
      for (int j = 0; j < date_buffer.dated_tile_size(); ++j) {
        const keyhole::QuadtreeImageryDatedTile& dated_tile =
          date_buffer.dated_tile(j);
        keyhole::JpegCommentDate jpeg_date(dated_tile.date());
        std::string date_string;
        jpeg_date.AppendToString(&date_string);
        dates->insert(date_string);
      }
    }
  }
}

// ToString() - convert node contents to printable string
void KhQuadTreeNodeProtoBuf::ToString(std::ostringstream* str,
                                   const std::int32_t node_index,
                                   const QuadtreePath &qt_path,
                                   const std::int32_t subindex) const {
  const keyhole::QuadtreeNode& proto_buf_node = proto_buf();
  std::uint32_t image_version = 0;
  std::uint32_t image_data_provider = 0;
  std::uint32_t terrain_version = 0;
  std::uint32_t terrain_data_provider = 0;
  for (int i = 0; i < proto_buf_node.layer_size(); ++i) {
    const keyhole::QuadtreeLayer& layer = proto_buf_node.layer(i);
    if (layer.type() == keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY) {
      image_version = layer.layer_epoch();
      image_data_provider = layer.provider();
    } else if (layer.type() == keyhole::QuadtreeLayer::LAYER_TYPE_TERRAIN) {
      terrain_version = layer.layer_epoch();
      terrain_data_provider = layer.provider();
    }
  }

  *str << "  node " << node_index
      << "  s" << subindex
      << " \"" << qt_path.AsString() << "\""
      << "  iv = " << image_version
      << ", ip = " << image_data_provider
      << ", tv = " << terrain_version
      << ", tp = " << terrain_data_provider
      << ", c = "  << proto_buf_node.cache_node_epoch()
      << ", flags = 0x" << std::hex
      << proto_buf_node.flags() << std::dec
      << "(";

  if (GetImageBit())
    *str << "I";
  if (GetTerrainBit())
    *str << "T";
  if (GetDrawableBit())
    *str << "V";
  if (GetCacheNodeBit())
    *str << "C";
  for (int j = 0; j < 4; ++j) {
    if (GetChildBit(j))
      *str << "0123"[j];
  }
  *str << ")" << std::endl;

  // Print out the Vector layer info
  std::uint32_t num_channels = proto_buf_node.channel_size();
  for (std::uint32_t i = 0; i < num_channels; ++i) {
    const keyhole::QuadtreeChannel& channel = proto_buf_node.channel(i);
    *str << "    V" << i << ": layer = " << channel.type()
        << ", version = " << channel.channel_epoch() << std::endl;
  }
  // Print out the Dated History layers info.
  if (HasLayerOfType(keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY)) {
    for (int i = 0; i < proto_buf_node.layer_size(); ++i) {
      const keyhole::QuadtreeLayer& layer = proto_buf_node.layer(i);
      if (layer.type() == keyhole::QuadtreeLayer::LAYER_TYPE_IMAGERY_HISTORY) {
        const keyhole::QuadtreeImageryDates& dates_layer = layer.dates_layer();
        for (int j = 0; j < dates_layer.dated_tile_size(); ++j) {
          const keyhole::QuadtreeImageryDatedTile& dated_tile =
            dates_layer.dated_tile(j);
          keyhole::JpegCommentDate jpeg_date(dated_tile.date());
          std::string date_string;
          jpeg_date.AppendToString(&date_string);
          *str << "    Historical" << j << ": datedimagery = " << date_string
            << ", version = " << dated_tile.dated_tile_epoch()
            << ", provider = " << dated_tile.provider()
            <<  std::endl;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////
// KffCopyrightMap
int KffCopyrightMap::CheckBbox(const double bbox[4], int level,
                               const KffDataProviderCRL* dataProviderList,
                               int dataProviderListLen) const {
  int i;
  int pid = 0;

  DCHECK(dataProviderList);

  for (i = 0; i < dataProviderListLen; i++) {
    if (dataProviderList[i].level > level)
      return pid;

    // use this data provider iff its bbox is strictly
    // contained withing the provided bounding box
    if (dataProviderList[i].bbox[1] > bbox[0] &&
       dataProviderList[i].bbox[0] < bbox[1] &&
       dataProviderList[i].bbox[3] > bbox[2] &&
       dataProviderList[i].bbox[2] < bbox[3])
      pid = dataProviderList[i].provider_id;
  }

  return pid;
}

// Initialize copyright map. Compute bounding box
// in lat/long space based on position in quadtree,
// then fill in provider_id info based on intersection
// with provider info bounding box.
bool KffCopyrightMap::CreateMap(const QuadtreePath& pos,
                                const KffDataProviderCRL* dataProviderList,
                                int dataProviderListLen) {
  // Bail immediately if we don't have data provider info.
  if (!dataProviderList || !dataProviderListLen)
    return false;

  std::uint32_t i, j;
  double lowerleft[2];

  lowerleft[0] = -180.0;
  lowerleft[1] = -180.0;

  for (i = 0; i < pos.Level(); i++) {
    if (pos[i] == 0) {
    } else if (pos[i] == 1) {
      lowerleft[0] += 180.0 / (1 << i);
    } else if (pos[i] == 2) {
      double delta = 180.0 / (1 << i);
      lowerleft[0] += delta;
      lowerleft[1] += delta;
    } else if (pos[i] == 3) {
      lowerleft[1] += 180.0 / (1 << i);
    }
  }

  // Extent of the bounding box at top level.
  double size = 360.0 / (1 << i);
  double bbox[4];
  double bbox2[4];
  double bbox3[4];

  // Check - Level 0
  bbox[0] = lowerleft[0];
  bbox[1] = lowerleft[0] + size;
  bbox[2] = lowerleft[1];
  bbox[3] = lowerleft[1] + size;
  level00 = CheckBbox(bbox, pos.Level(),
                      dataProviderList, dataProviderListLen);

  // Check - Level 1
  bbox[0] = lowerleft[0];
  bbox[1] = lowerleft[0] + size * 0.5;
  bbox[2] = lowerleft[1];
  bbox[3] = lowerleft[1] + size * 0.5;
  level01[0] = CheckBbox(bbox, pos.Level() + 1,
                         dataProviderList, dataProviderListLen);

  bbox[0] = lowerleft[0] + size * 0.5;
  bbox[1] = lowerleft[0] + size;
  bbox[2] = lowerleft[1];
  bbox[3] = lowerleft[1] + size * 0.5;
  level01[1] = CheckBbox(bbox, pos.Level() + 1,
                         dataProviderList, dataProviderListLen);

  bbox[0] = lowerleft[0] + size * 0.5;
  bbox[1] = lowerleft[0] + size;
  bbox[2] = lowerleft[1] + size * 0.5;
  bbox[3] = lowerleft[1] + size;
  level01[2] = CheckBbox(bbox, pos.Level() + 1,
                         dataProviderList, dataProviderListLen);

  bbox[0] = lowerleft[0];
  bbox[1] = lowerleft[0] + size * 0.5;
  bbox[2] = lowerleft[1] + size * 0.5;
  bbox[3] = lowerleft[1] + size;
  level01[3] = CheckBbox(bbox, pos.Level() + 1,
                         dataProviderList, dataProviderListLen);

  // Check - Level 2
  for (i = 0; i < 4; i++) {
    if (i == 0) {
      bbox[0] = lowerleft[0];
      bbox[1] = lowerleft[0] + size * 0.5;
      bbox[2] = lowerleft[1];
      bbox[3] = lowerleft[1] + size * 0.5;
    } else if (i == 1) {
      bbox[0] = lowerleft[0] + size * 0.5;
      bbox[1] = lowerleft[0] + size;
      bbox[2] = lowerleft[1];
      bbox[3] = lowerleft[1] + size * 0.5;
    } else if (i == 2) {
      bbox[0] = lowerleft[0] + size * 0.5;
      bbox[1] = lowerleft[0] + size;
      bbox[2] = lowerleft[1] + size * 0.5;
      bbox[3] = lowerleft[1] + size;
    } else if (i == 3) {
      bbox[0] = lowerleft[0];
      bbox[1] = lowerleft[0] + size * 0.5;
      bbox[2] = lowerleft[1] + size * 0.5;
      bbox[3] = lowerleft[1] + size;
    }

    bbox2[0] = bbox[0];
    bbox2[1] = bbox[1] + size * 0.25;
    bbox2[2] = bbox[2];
    bbox2[3] = bbox[3] + size * 0.25;
    level02[i][0] = CheckBbox(bbox2, pos.Level() + 2,
                              dataProviderList, dataProviderListLen);

    bbox2[0] = bbox[0] + size * 0.25;
    bbox2[1] = bbox[1] + size * 0.5;
    bbox2[2] = bbox[2];
    bbox2[3] = bbox[3] + size * 0.25;
    level02[i][1] = CheckBbox(bbox2, pos.Level() + 2,
                              dataProviderList, dataProviderListLen);

    bbox2[0] = bbox[0] + size * 0.25;
    bbox2[1] = bbox[1] + size * 0.5;
    bbox2[2] = bbox[2] + size * 0.25;
    bbox2[3] = bbox[3] + size * 0.5;
    level02[i][2] = CheckBbox(bbox2, pos.Level() + 2,
                              dataProviderList, dataProviderListLen);

    bbox2[0] = bbox[0];
    bbox2[1] = bbox[1] + size * 0.25;
    bbox2[2] = bbox[2] + size * 0.25;
    bbox2[3] = bbox[3] + size * 0.5;
    level02[i][3] = CheckBbox(bbox2, pos.Level() + 2,
                              dataProviderList, dataProviderListLen);
  }

  // Check - Level 3
  for (i = 0; i < 4; i++) {
    if (i == 0) {
      bbox[0] = lowerleft[0];
      bbox[1] = lowerleft[0] + size * 0.5;
      bbox[2] = lowerleft[1];
      bbox[3] = lowerleft[1] + size * 0.5;
    } else if (i == 1) {
      bbox[0] = lowerleft[0] + size * 0.5;
      bbox[1] = lowerleft[0] + size;
      bbox[2] = lowerleft[1];
      bbox[3] = lowerleft[1] + size * 0.5;
    } else if (i == 2) {
      bbox[0] = lowerleft[0] + size * 0.5;
      bbox[1] = lowerleft[0] + size;
      bbox[2] = lowerleft[1] + size * 0.5;
      bbox[3] = lowerleft[1] + size;
    } else if (i == 3) {
      bbox[0] = lowerleft[0];
      bbox[1] = lowerleft[0] + size * 0.5;
      bbox[2] = lowerleft[1] + size * 0.5;
      bbox[3] = lowerleft[1] + size;
    }

    for (j = 0; j < 4; j++) {
      if (j == 0) {
        bbox2[0] = bbox[0];
        bbox2[1] = bbox[1] + size * 0.25;
        bbox2[2] = bbox[2];
        bbox2[3] = bbox[3] + size * 0.25;
      } else if (j == 1) {
        bbox2[0] = bbox[0] + size * 0.25;
        bbox2[1] = bbox[1] + size * 0.5;
        bbox2[2] = bbox[2];
        bbox2[3] = bbox[3] + size * 0.25;
      } else if (j == 2) {
        bbox2[0] = bbox[0] + size * 0.25;
        bbox2[1] = bbox[1] + size * 0.5;
        bbox2[2] = bbox[2] + size * 0.25;
        bbox2[3] = bbox[3] + size * 0.5;
      } else if (j == 3) {
        bbox2[0] = bbox[0];
        bbox2[1] = bbox[1] + size * 0.25;
        bbox2[2] = bbox[2] + size * 0.25;
        bbox2[3] = bbox[3] + size * 0.5;
      }

      bbox3[0] = bbox2[0];
      bbox3[1] = bbox2[1] + size * 0.125;
      bbox3[2] = bbox2[2];
      bbox3[3] = bbox2[3] + size * 0.125;
      level03[i][j][0] = CheckBbox(bbox3, pos.Level() + 3,
                                   dataProviderList, dataProviderListLen);

      bbox3[0] = bbox2[0] + size * 0.125;
      bbox3[1] = bbox2[1] + size * 0.25;
      bbox3[2] = bbox2[2];
      bbox3[3] = bbox2[3] + size * 0.125;
      level03[i][j][1] = CheckBbox(bbox3, pos.Level() + 3,
                                   dataProviderList, dataProviderListLen);

      bbox3[0] = bbox2[0] + size * 0.125;
      bbox3[1] = bbox2[1] + size * 0.25;
      bbox3[2] = bbox2[2] + size * 0.125;
      bbox3[3] = bbox2[3] + size * 0.25;
      level03[i][j][2] = CheckBbox(bbox3, pos.Level() + 3,
                                   dataProviderList, dataProviderListLen);

      bbox3[0] = bbox2[0];
      bbox3[1] = bbox2[1] + size * 0.125;
      bbox3[2] = bbox2[2] + size * 0.125;
      bbox3[3] = bbox2[3] + size * 0.25;
      level03[i][j][3] = CheckBbox(bbox3, pos.Level() + 3,
                                   dataProviderList, dataProviderListLen);
    }
  }

  return true;
}

} // namespace qtpacket
