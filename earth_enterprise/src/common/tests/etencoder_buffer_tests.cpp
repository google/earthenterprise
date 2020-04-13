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

// This test checks for buffer overflows in the khEncrypt algorithm. It does
// not include any asserts; instead, it must be compiled with Address Sanitizer
// so that it will fail in the case of an overflow.

#include <cstring>
#include <memory>

// Intentionally include the .cc file instead of the .h file to force the .cc
// file to be compiled with Address Sanitizer.
#include "common/etencoder.cc"

using namespace std;

unique_ptr<uint8_t[]> GetByteArray(uint32_t length) {
  unique_ptr<uint8_t[]> data(new uint8_t[length]);
  memset(data.get(), 0, length);
  return data;
}

int main() {
  const uint32_t KEY_LENGTHS = 100;
  const uint32_t DATA_LENGTHS = 100;
  for(uint32_t klen = 0; klen < KEY_LENGTHS; ++klen) {
    auto key = GetByteArray(klen);
    for(uint32_t dlen = 0; dlen < DATA_LENGTHS; ++dlen) {
      auto data = GetByteArray(dlen);
      try {
        etEncoder::Encode(data.get(), dlen, key.get(), klen);
      }
      catch (...) {
        // Ignore the exception - this means that Encode detected and handled
        // the error, so the test passes.
      }
      
    }
  }
  return 0;
}
