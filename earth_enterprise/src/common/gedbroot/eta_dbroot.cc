// Copyright 2017 Google Inc.
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
// Check whether a file is a dbroot (binary or ascii).
// If binary, decode its contents into ascii.

#include "common/gedbroot/eta_dbroot.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "common/khSimpleException.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/etencoder.h"
#include "common/packetcompress.h"

namespace {
const std::uint32_t kOldMagic = 0x4e876494;

// Struct from old ETA DBroot code.
struct etDbaseRootHeader {
  std::uint32_t rootMagic;
  unsigned char qtTypeVersion;
  // Padding char added by compiler.
  std::uint16_t qtDataVersion;
};
}  // anonymous namespace

bool EtaDbroot::IsEtaDbroot(const std::string &content,
                            const ExpectType type) {
  if (type == kExpectBinary || type == kExpectEither) {
    // Check whether it is a binary dbroot.
    if (content.size() < sizeof(kOldMagic)) {
      return false;
    }
    std::string eta_magic_buffer = content.substr(0, sizeof(kOldMagic));
    if (kOldMagic == *reinterpret_cast<std::uint32_t*>(&eta_magic_buffer[0])) {
      return true;
    }
  }

    // Check whether it is an ascii dbroot.
  if (type == kExpectAscii || type == kExpectEither) {
    return content.find("<etStruct>") != std::string::npos;
  }

  return false;
}

bool EtaDbroot::DecodeBinary(const std::string &binary,
                             std::string *text, std::uint16_t *epoch) {
  if (!IsEtaDbroot(binary, kExpectBinary)) {
    return false;
  }

  // A valid binary dbroot has to consist of at least one valid key.
  size_t data_offset = sizeof(etDbaseRootHeader) +
      etEncoder::kDefaultKey.size();
  if (binary.size() < data_offset) {
    throw khSimpleException("Truncated dbroot: ") << binary.size() << " < " <<
        data_offset;
  }

  // Read the encoding key
  // It's always been the same, but let's sanity check it.
  std::string key_buffer;
  key_buffer.append(binary, sizeof(etDbaseRootHeader),
                    etEncoder::kDefaultKey.size());
  if (key_buffer != etEncoder::kDefaultKey) {
    throw khSimpleException("Corrupted dbroot: Key has unexpected value.");
  }

  // Read the header and see if it is a valid binary dbroot file.
  std::string header_buffer;
  header_buffer.append(binary, 0, sizeof(etDbaseRootHeader));
  // Decrypt the entire header
  etEncoder::DecodeWithDefaultKey(&header_buffer[0], header_buffer.size());
  etDbaseRootHeader header;
  memcpy(&header, header_buffer.data(), sizeof(etDbaseRootHeader));
  // now restore the originally unencrypted magic from the file
  // That's just the way the file format works.
  memcpy(&header.rootMagic, binary.data(), sizeof(header.rootMagic));
  if (header.rootMagic != kOldMagic) {
    throw khSimpleException("Corrupted dbroot: Bad magic.");
  }
  *epoch = header.qtDataVersion;

  // If there is no data then this is just an empty dbroot which is valid.
  size_t data_size = binary.size() - data_offset;
  if (data_size > 0) {
    // Decrypt the post portion of the dbroot.
    std::string decodeBuf;
    decodeBuf.append(binary, data_offset, data_size);
    etEncoder::DecodeWithDefaultKey(&decodeBuf[0], data_size);

    // Try to decompress the data portion. If it fails then we assume that the
    // dbroot is uncompressed.
    *text = "";
    LittleEndianReadBuffer uncompressed_buffer;
    if (KhPktDecompress(&decodeBuf[0], data_size,
                        &uncompressed_buffer)) {
      text->append(uncompressed_buffer);
    } else {
      text->append(decodeBuf, 0, data_size);
    }
    return true;
  }
  return false;
}
