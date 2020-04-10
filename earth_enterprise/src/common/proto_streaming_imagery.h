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


// Helper classes to work with EarthImageryPacket from
// keyhole/common/proto/streaming_imagery.proto.

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_PROTO_STREAMING_IMAGERY_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_PROTO_STREAMING_IMAGERY_H_

#include <string>

//#include "common/khTypes.h"
#include <cstdint>
#include "protobuf/streaming_imagery.pb.h"


// Wrapper for keyhole::EarthImageryPacket.
class geEarthImageryPacket {
 public:
  geEarthImageryPacket(void);
  ~geEarthImageryPacket(void);

  // Computes message metadata size.
  size_t GetMetadataSize() const;

  // Computes the serialized size of the EarthImageryPacket message.
  inline int ByteSize() const {
    return packet_.ByteSize();
  }

  // Gets message cached size.
  inline int GetCachedSize() const {
    return packet_.GetCachedSize();
  }

  // Parses packet from string.
  // @param buf buffer that contains serialised EarthImageryPacket;
  inline bool ParseFromString(const std::string& buf) {
    return packet_.ParseFromString(buf);
  }

  // Serializes proto message to string.
  inline bool SerializeToString(std::string *output) const {
    return packet_.SerializeToString(output);
  }

  // Serializes proto message to array.
  inline bool SerializeToArray(void* data, int size) const {
    return packet_.SerializeToArray(data, size);
  }

  // Clears all fields.
  inline void Clear() {
    return packet_.Clear();
  }

  // The image data accessors/setters.
  // Check if packet has type of image data.
  inline bool HasImageType() const {
    return packet_.has_image_type();
  }

  // Gets type of image data.
  inline ::keyhole::EarthImageryPacket_Codec ImageType() const {
    assert(HasImageType());
    return packet_.image_type();
  }

  // Sets type of image data.
  inline void SetImageType(
      ::keyhole::EarthImageryPacket_Codec value) {
    packet_.set_image_type(value);
  }

  // Clears type of image data.
  inline void ClearImageType() {
    packet_.clear_image_type();
  }

  // Check if packet has image data.
  inline bool HasImageData() const {
    return packet_.has_image_data();
  }

  // Gets image data.
  inline const std::string& ImageData() const {
    assert(HasImageData());
    return packet_.image_data();
  }

  // Gets mutable image data.
  inline std::string* MutableImageData() {
    return packet_.mutable_image_data();
  }

  // Sets image data.
  inline void SetImageData(const void* value, size_t size) {
    packet_.set_image_data(value, size);
  }

  // Clears image data.
  inline void ClearImageData() {
    packet_.clear_image_data();
  }

  // The alpha data accessors/setters.
  // Check if packet has type of image data.
  inline bool HasAlphaType() const {
    return packet_.has_alpha_type();
  }

  // Gets type of alpha data.
  inline
  ::keyhole::EarthImageryPacket_SeparateAlphaType GetAlphaType() const {
    assert(HasAlphaType());
    return packet_.alpha_type();
  }

  // Sets type of alpha data.
  inline void SetAlphaType(
      ::keyhole::EarthImageryPacket_SeparateAlphaType value) {
    packet_.set_alpha_type(value);
  }

  // Clears type of alpha data.
  inline void ClearAlphaType() {
    packet_.clear_alpha_type();
  }

  // Check if packet has alpha data.
  inline bool HasImageAlpha() const {
    return packet_.has_image_alpha();
  }

  // Gets alpha data.
  inline const std::string& ImageAlpha() const {
    assert(HasImageAlpha());
    return packet_.image_alpha();
  }

  // Gets mutable alpha data.
  inline std::string* MutableImageAlpha() {
    return packet_.mutable_image_alpha();
  }

  // Sets alpha data.
  inline void SetImageAlpha(const void* value, size_t size) {
    packet_.set_image_alpha(value, size);
  }

  // Clears alpha data.
  inline void ClearImageAlpha() {
    packet_.clear_image_alpha();
  }

 private:
  keyhole::EarthImageryPacket packet_;
};

// Helper class to prepare protobuf packet for writing.
class geProtobufPacketBuilder {
 public:
  explicit geProtobufPacketBuilder(size_t _data_size);
  ~geProtobufPacketBuilder(void);

  // Sets data image.
  inline void SetImageData(const char *data, std::uint32_t size) {
    packet_.SetImageType(keyhole::EarthImageryPacket::JPEG);
    packet_.SetImageData(data, size);
  }

  // Sets alpha image.
  inline void SetImageAlpha(const char *data, std::uint32_t size) {
    packet_.SetAlphaType(keyhole::EarthImageryPacket::PNG);
    packet_.SetImageAlpha(data, size);
  }

  // Creates protobuf format imagery packet by serialization
  // EarthImageryPacket.
  // @param packet_data returned pointer to protobuf packet;
  // @param packet_size returned size of protobuf packet;
  void Build(char **packet_data, unsigned int *packet_size);

 private:
  std::string packet_buf_;
  geEarthImageryPacket packet_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_PROTO_STREAMING_IMAGERY_H_
