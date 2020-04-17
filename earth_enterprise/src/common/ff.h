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

#ifndef _ff_h_
#define _ff_h_

#include <cstdint>

// ****************************************************************************
// ***  flat file header structure
// ****************************************************************************
class FFRecHeader {
  std::uint32_t  len_,           // length of the compressed data in bytes
    level_,         // quadnode level 
    x_, y_,         // tile address
    vers_;          // tile version
  std::uint32_t  padding[3];     // pad to 32 byte boundary

 public:
  FFRecHeader(std::uint32_t len__, std::uint32_t level__, std::uint32_t x__, std::uint32_t y__,
              std::uint32_t vers__ = 0)
      : len_(len__), level_(level__), x_(x__), y_(y__), vers_(vers__) {
    // zero fill padding to facilitate binary diffs of flat files
    padding[0] = 0;
    padding[1] = 0;
    padding[2] = 0;
  }
  inline std::uint32_t  len()   const { return len_; }
  inline std::uint32_t  level() const { return level_; }
  inline std::uint32_t  x()     const { return x_; }
  inline std::uint32_t  y()     const { return y_; }
  inline std::uint32_t  vers()  const { return vers_; }
  inline static std::uint32_t  paddedLen(std::uint32_t l) { return (l + 31) & ~31; }
  inline std::uint32_t  paddedLen(void) const { return paddedLen(len_); }
  void BigEndianToHost(void);
  // just another name for code clarity
  inline void HostToBigEndian(void) { BigEndianToHost(); }
};
static_assert(sizeof(FFRecHeader) == 32, "Bad FF Rec Header Size");


// ****************************************************************************
// ***  flat file read routine
// ****************************************************************************
typedef bool (*ffReadCallbackFunc)(const FFRecHeader *head, void *data,
                                   void *userData);
int ff_read(const char *_ffpath, ffReadCallbackFunc callback,
            void* userData = 0);


// ****************************************************************************
// ***  Misc flat file routines
// ****************************************************************************
// returns approx number of entries
 std::uint64_t ff_estimate_numentries(const char *ffpath);





#endif // !_ff_h_
