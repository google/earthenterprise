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
// Test for compress module
// Retrofitted back to Fusion by Mike Goss.
//

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <khEndian.h>
#include <UnitTest.h>
#include "packetcompress.h"

class PacketCompressUnitTest : public UnitTest<PacketCompressUnitTest> {
 public:
  PacketCompressUnitTest() : BaseClass("PacketCompressUnitTest") {
    REGISTER(CompressTest1);
    REGISTER(CompressTest2);
    REGISTER(DecompressTest1);
    REGISTER(EndianBufferTest);
  }

  // Create a simple test string
  static const char kString[];

  // Simple compression test to verify we can compress a string
  bool CompressTest1() {
    size_t bufsize = strlen(kString);
    char* output;
    size_t outsize;
    TestAssert(KhPktCompress(kString, bufsize, &output, &outsize, true));
    TestAssert(outsize-kPacketCompressHdrSize < bufsize);
    // Try to compress it again - verify that it does not save space
    char* output_2;
    size_t outsize_2;
    TestAssert(KhPktCompress(output + kPacketCompressHdrSize,
                             outsize - kPacketCompressHdrSize,
                             &output_2,
                             &outsize_2,
                             true));
    TestAssert(outsize <= outsize_2);
    delete [] output;
    delete [] output_2;
    return true;
  }

  // Try to compress small buffers
  bool CompressTest2() {
    char buffer[] = "a";
    char* output;
    size_t outsize;
    TestAssert(KhPktCompress(buffer, 1, &output, &outsize, true));
    delete [] output;
    // Try with magic sequence. Should not compress much either.
    int int_buffer[2];
    int_buffer[0] = 0x7468dead;
    int_buffer[1] = 0;
    size_t bufsize = sizeof(int_buffer);
    TestAssert(KhPktCompress(int_buffer, bufsize, &output, &outsize, true));
    delete [] output;
    return true;
  }

  // Test compressing/decompressing small packets
  bool DecompressTest1() {
    char* output;
    size_t outputsize;
    TestAssert(KhPktCompress("a", 1, &output, &outputsize, true));
    // Try to decompress
    LittleEndianReadBuffer decompressed;
    TestAssert(KhPktDecompress(output, outputsize, &decompressed));
    delete [] output;
    return true;
  }

  // Check decompress using EndianReadBuffer
  bool EndianBufferTest() {
    size_t bufsize = strlen(kString);
    char* output;
    size_t outputsize;
    TestAssert(KhPktCompress(kString, bufsize, &output, &outputsize, true));
    // compressed size should be smaller
    TestAssert(outputsize-kPacketCompressHdrSize <= bufsize);
    std::uint32_t *hdr = reinterpret_cast<std::uint32_t*>(output);
    TestAssert(hdr[1] == bufsize);
    // Now decompress
    LittleEndianReadBuffer decompressed;
    TestAssert(KhPktDecompress(output, outputsize, &decompressed));
    TestAssert(decompressed.size() == bufsize);
    TestAssert(memcmp(decompressed.data(), kString, bufsize) == 0);

    delete [] output;
    return true;
  }
};

const char PacketCompressUnitTest::kString[] = 
  "A string to compress. This is plain text so compression "
  "should work very well. We are not trying to measure "
  "compression performance however, only verify that it "
  "actually reduces the size of the input.";

int main(int argc, char **argv) {
  PacketCompressUnitTest tests;
  return tests.Run();
}
