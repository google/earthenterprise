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


#ifndef kbf_H
#define kbf_H

#include <cstdint>
#include <string.h>
#include <khGuard.h>
#include <khEndian.h>

#define KBF_READ 0
#define KBF_WRITE 1

#define MAGIC_1 123
#define MAGIC_2 213
#define MAGIC_3 132

struct kbfFileHeader {
  std::uint16_t channel;
  std::uint16_t version;
  std::uint32_t size;

  unsigned char level;
  unsigned char magicId[3];
  unsigned char branchlist[32];

  void init() {
    memset(this, 0, sizeof(kbfFileHeader));

    magicId[0] = MAGIC_1;
    magicId[1] = MAGIC_2;
    magicId[2] = MAGIC_3;
  };

  kbfFileHeader() {
    init();
  }

  kbfFileHeader(std::uint16_t chan, std::uint16_t ver,
                unsigned char lev, unsigned char *blist, std::uint32_t buffsz) {
    init();
    channel = chan;
    version = ver;
    size = buffsz;
    level = lev;
    memcpy(branchlist, blist, 32);
  }

  bool checkMagic() {
    return ((magicId[0] == MAGIC_1) &&
            (magicId[1] == MAGIC_2) &&
            (magicId[2] == MAGIC_3));
  };

  void byteSwap() {
    swapb(channel);
    swapb(version);
    swapb(size);
  };
};

class kbfFile {
 public:
  kbfFile();
  ~kbfFile();

  bool open(const char *filename, int mode);
  int eof();
  void close();
  bool read(kbfFileHeader& hdr, char* buff, std::uint32_t buffsz, std::int32_t& fpos);
 private:
  void resetposition();
  FILE* fp;
};

#endif
