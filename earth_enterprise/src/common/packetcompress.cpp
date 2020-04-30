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
// A wrapper around zlib compress api.
// Retrofitted back to Fusion by Mike Goss.
//
#include <cstdint>
#include <zlib.h>
#include <khSimpleException.h>
#include <khEndian.h>
#include <packetcompress.h>

// Size of compression header
const size_t kPacketCompressHdrSize = 8;

// Maximum size of a 'small buffer'.
// There's a 128k restriction in the old clients (1.7),
// it would not be able to handle packets larger than that.
// For some packets though such as dbroot, it's actually ok
// to send more than 128k since newer clients support it
// (old clients will request dbRoot.v4).
static const size_t kLargeBlockSize = 1 << 17;  // 128k

// Magic ids
const std::uint32_t kPktMagic = 0x7468deadu;
const std::uint32_t kPktMagicSwap = 0xadde6874u;

namespace {

// Validate header and return decompress output buffer size

bool GetDecompressBufferSize(const void*buf, size_t buflen,
                             size_t *dbuf_size) {
  const std::uint32_t* lbuf = reinterpret_cast<const std::uint32_t*>(buf);

  // Check for header validity
  if (buflen < kPacketCompressHdrSize
      || (lbuf[0] != kPktMagic
          && lbuf[0] != kPktMagicSwap))
    return false;

  if (lbuf[0] == kPktMagicSwap) {
    size_t len = lbuf[1];
    size_t v = ((len >> 24) & 0x000000ffu) |
               ((len >>  8) & 0x0000ff00u) |
               ((len <<  8) & 0x00ff0000u) |
               ((len << 24) & 0xff000000u);
    *dbuf_size = v;
  } else {
    *dbuf_size = lbuf[1];
  }
  return true;
}

// Decompress to specified buffer (source includes packet header).
// Offset buffer by 8 bytes and buffer length by 8 to skip packet
// header

bool DecompressToBuffer(const void *source,
                        size_t source_size,
                        char *dest,
                        size_t dest_size) {
  uLongf ul_dest_size = dest_size;
  return
    Z_OK == uncompress(
        reinterpret_cast<Bytef*>(dest),
        &ul_dest_size,
        reinterpret_cast<const Bytef*>(source) + kPacketCompressHdrSize,
        source_size - kPacketCompressHdrSize);
}
}

// Decompress into a supplied EndianReadBuffer. Use of this version is
// preferred.

bool KhPktDecompress(const void* buf, size_t buflen,
                     EndianReadBuffer *data) {
  size_t tdatalen;
  if (!GetDecompressBufferSize(buf, buflen, &tdatalen))
    return false;

  data->resize(tdatalen);

  // Decompress into output buffer
  if (!DecompressToBuffer(buf, buflen, &((*data)[0]), data->size())) {
    data->resize(0);
  }
  data->Seek(0);
  return !data->empty();
}

// Compress using a pre-allocated buffer (*outbuffer).
// Outbuffer must be large enough to hold data size
bool KhPktCompressWithBuffer(const void* pkt, size_t pktlen,
                             char* outbuf, size_t* outbuflen) {
  // Guard against a blatant misuse of this API.
  assert(pktlen > 0);
  assert(outbuflen);
  assert(*outbuflen > kPacketCompressHdrSize);
  assert(*outbuflen > pktlen);
  assert(outbuf);
  // offset by 8 to account for header
  // we pass tbuf[8] to compress2
  uLongf tbuflen = (*outbuflen) - kPacketCompressHdrSize;
  Bytef* tbuf = reinterpret_cast<Bytef*>(outbuf);

  // compress does size check for us.
  // If tbuflen is too short, compress will fail
  if (compress(&tbuf[kPacketCompressHdrSize],
               &tbuflen,
               reinterpret_cast<const Bytef*>(pkt),
               pktlen) != Z_OK) {
    return false;
  }

  // add compressed packet header
  std::uint32_t* lbuf = reinterpret_cast<std::uint32_t*>(tbuf);
  lbuf[0] = kPktMagic;
  lbuf[1] = pktlen;
  tbuflen += kPacketCompressHdrSize;

  assert(tbuflen <= *outbuflen);
  *outbuflen = tbuflen;

  return true;
}

// Compress pkt into *buf (allocates memory), store buffersize in *buflen
// Does not compress buffer larger than 128k
// unless allowLargeBlocks is set to true
bool KhPktCompress(const void* pkt, size_t pktlen,
                   char** buf, size_t* buflen, bool allowLargeBlocks) {
  assert(buf);
  assert(buflen);
  // don't let the source get too big
  if (!allowLargeBlocks && pktlen > kLargeBlockSize)
    return false;

  // create a temporary buffer for the compression workspace
  size_t tbuflen = KhPktGetCompressBufferSize(pktlen);
  char* tbuf = new char[tbuflen];
  assert(tbuflen > pktlen);

  if (!KhPktCompressWithBuffer(pkt, pktlen, tbuf, &tbuflen)) {
    delete [] tbuf;
    return false;
  }

  // save data. User will need to delete this memory
  *buf = tbuf;
  *buflen = tbuflen;
  return true;
}
