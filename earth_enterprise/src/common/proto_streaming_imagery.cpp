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


#include "common/proto_streaming_imagery.h"

#include <third_party/rsa_md5/crc32.h>

#include "google/protobuf/descriptor.h"
#include "common/khTileAddr.h"

// geEarthImageryPacket
geEarthImageryPacket::geEarthImageryPacket(void) {
}

geEarthImageryPacket::~geEarthImageryPacket(void) {
}

size_t geEarthImageryPacket::GetMetadataSize() const {
  // message_metadata_size = (field_count * field_metadata_size);
  // field_metadata_size = varint_key_field(1 byte) +
  //     length_field (3 bytes for tile size 255*255*3) + reserved(1 byte);
  const size_t kFieldMetadataSize = 5;
  size_t metadata_size = packet_.GetDescriptor()->field_count() *
      kFieldMetadataSize;
  return metadata_size;
}

// geProtobufPacketBuilder
geProtobufPacketBuilder::geProtobufPacketBuilder(size_t _data_size)
    : packet_buf_(),
      packet_() {
  // Allocate memory for packet_buf:
  // packet_size = metadata_size + data_size.
  size_t packet_buf_size = packet_.GetMetadataSize() + _data_size;
  packet_buf_.reserve(packet_buf_size);
}

geProtobufPacketBuilder::~geProtobufPacketBuilder() {
}

void geProtobufPacketBuilder::Build(char **packet_data, unsigned int *packet_size) {
  // Serialize EarthImagery packet.
  bool serialized = packet_.SerializeToArray(
      &packet_buf_[0], packet_buf_.capacity());

  // Note: GetCachedSize() is faster then ByteSize(). We use it here because
  // ByteSize() was called in SerializeToArray().
  size_t serialized_size = packet_.GetCachedSize();

  // packet_buf should have at least kCRC32Size bytes available on the end.
  std::uint32_t pack_size = serialized_size + kCRC32Size;

  if ((!serialized) || (pack_size > packet_buf_.capacity())) {
    // Note: can't be reached but just to be sure.
    // Reallocate larger buffer and serialize.
    // new_buffer_size = pack_size + reserve(1/4 pack_size);
    packet_buf_.reserve(pack_size + (pack_size >> 2));
    packet_.SerializeToArray(&packet_buf_[0], packet_buf_.capacity());
  }

  // Clear EarthImagery packet fields.
  packet_.Clear();

  // Fill in returned values.
  *packet_size = pack_size;
  *packet_data = &(packet_buf_[0]);
}
