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
// A wrapper around zlib compress api.
// Retrofitted back to Fusion by Mike Goss.
//
#ifndef COMMON_PACKETCOMPRESS_H__
#define COMMON_PACKETCOMPRESS_H__

//#include "common/khTypes.h"
#include <cstdint>

extern const size_t kPacketCompressHdrSize;
// Magic ids
extern const std::uint32_t kPktMagic;
extern const std::uint32_t kPktMagicSwap;

class EndianReadBuffer;

// Compress pkt buffer to *data.
// Store output buffer size in datalen/
// Memory is allocated with new[], so use delete[]
// once you're done with the output buffer.
// Return true if compression was successful, false otherwise
// DEPRECATED(jdjohnso) use KhPktCompressWithBuffer instead
bool KhPktCompress(const void* pkt, size_t pktlen,  char** OUTPUT,
                   size_t* OUTDATALEN, bool allowLargeBlocks);

// Get the compress buffer size for a given buffer size
inline size_t KhPktGetCompressBufferSize(size_t buffersize) {
  return static_cast<size_t>((buffersize * 1.001 + .5) + 20);
}

// A decompress function which places the result in an EndianReadBuffer.
bool KhPktDecompress(const void* buf, size_t buflen,
                     EndianReadBuffer *data);

// A compress function which takes a user buffer to avoid allocation.
// User buffer (*outbuffer) must be large enough.
// If compressed output would be larger, source buffer will be copied
// to OUTPUT and *OUTDATALEN will be bufLen.
// Return true if compression was successful, false otherwise
bool KhPktCompressWithBuffer(const void* buf, size_t bufLen,
                             char* OUTPUT, size_t* OUTDATALEN);

#endif  // COMMON_PACKETCOMPRESS_H__
