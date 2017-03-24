/*
 * Copyright 2017 Google Inc.
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
  static const uint64 DefaultSplitSize = 1000 * 1024 * 1024;

  ~Writer(void) { }
  Writer(Type type,
         const std::string &outdir_,
         uint64 splitSize_ = DefaultSplitSize);

  // Will throw if unsuccessful
  void WritePacket(char *buf, uint32 buflen, uint32 bufsize,
                   const khTileAddr &addr);

  inline uint32 TotalTilesWritten(void) const {
    return totalTilesWritten;
  }

 protected:
  std::string outdir;
  uint64 splitSize;

  uint64 totalOffset;  // offset into (logical) single flatfile
  uint64 fileOffset;       // offset into current flatfile
  int nextfilenum;         // which subfile to open next
  khWriteFileCloser fileHandle; // currently open file
  uint32 totalTilesWritten;

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
                    uint64 splitSize_ = DefaultSplitSize,
                    void* littleEndianTypeData = 0);

  // Will throw if unsuccessful
  void WritePacket(char *buf, uint32 buflen, uint32 bufsize,
                   const khTileAddr &addr);

  // define this here, since my users can't see my base class
  inline uint32 TotalTilesWritten(void) const {
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
