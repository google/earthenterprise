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


/******************************************************************************
File:        ffioWriter.h
******************************************************************************/
#ifndef __ffioWriter_h
#define __ffioWriter_h

#include <string>
#include "ffioIndex.h"
#include "ffioPresenceMask.h"

namespace ffio {
class Writer {
 public:
  // Set to 0 to disable splitting the ff's
  //   - but only do that after the khdb index format has been
  // enhanced to deal w/ 64bit file offsets
  static const std::uint64_t DefaultSplitSize = 1000 * 1024 * 1024;

  ~Writer(void) { }
  Writer(Type type,
         const std::string &outdir_,
         std::uint64_t splitSize_ = DefaultSplitSize);

  // Will throw if unsuccessful
  void WritePacket(char *buf, std::uint32_t buflen, std::uint32_t bufsize,
                   const khTileAddr &addr);

  inline std::uint32_t TotalTilesWritten(void) const {
    return totalTilesWritten;
  }

 protected:
  std::string outdir;
  std::uint64_t splitSize;

  std::uint64_t totalOffset;  // offset into (logical) single flatfile
  std::uint64_t fileOffset;       // offset into current flatfile
  int nextfilenum;         // which subfile to open next
  khWriteFileCloser fileHandle; // currently open file
  std::uint32_t totalTilesWritten;

  // Will throw if unsuccessful
  void OpenNextFile(void);
};


// protected inheritance - I'm really a Writer, but only my derived
// classes and I know it. I cannot be used like I am a plain ol' Writer.
class GridIndexedWriter : protected Writer {
 public:
  GridIndexedWriter(Type type,
                    const std::string &outdir_,
                    const khInsetCoverage &coverage,
                    std::uint64_t splitSize_ = DefaultSplitSize,
                    void* littleEndianTypeData = 0);

  // Will throw if unsuccessful
  void WritePacket(char *buf, std::uint32_t buflen, std::uint32_t bufsize,
                   const khTileAddr &addr);

  // define this here, since my users can't see my base class
  inline std::uint32_t TotalTilesWritten(void) const {
    return totalTilesWritten;
  }

  inline bool GetPresence(const khTileAddr& addr) const {
    return presence.GetPresence(addr);
  }
  inline const khPresenceMask& GetPresenceMask(void) const {
    return presence.GetPresenceMask();
  }
 protected:
  inline void SetLittleEndianTypeData(void* typeData) {
    index.SetLittleEndianTypeData(typeData);
  }
 private:
  IndexWriter index;
  khPresenceMaskWriter presence;
};
} // namespace ffio



#endif /* __ffioWriter_h */
