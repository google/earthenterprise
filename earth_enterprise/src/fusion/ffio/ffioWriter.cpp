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


/******************************************************************************
File:        ffioWriter.cpp
******************************************************************************/
#include "ffioWriter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ff.h>
#include <khFileUtils.h>
#include <khException.h>
#include <notify.h>


// ****************************************************************************
// ***  ffio::Writer
// ****************************************************************************
void
ffio::Writer::OpenNextFile(void)
{
  char fnum[64];
  sprintf(fnum, "%02d", nextfilenum++);
  std::string fname = outdir + "/pack." + fnum;

  // make our life easier by letting all users read and write this file!
  ::umask(000);

  // will automatically close previous file (if any)
  fileHandle = ::open(fname.c_str(), O_WRONLY|O_CREAT|O_LARGEFILE, 0666);
  if (fileHandle.fd() < 0) {
    throw khErrnoException(kh::tr("Unable to create %1").arg(fname));
  }
}


ffio::Writer::Writer(Type type,
                     const std::string &outdir_,
                     uint64 splitSize_)
    : outdir(outdir_),
      splitSize(splitSize_),
      totalOffset(0),
      fileOffset(0),
      nextfilenum(0),
      fileHandle(),
      totalTilesWritten(0)
{
  if (!khMakeCleanDir(outdir)) {
    throw khException(kh::tr("Unable to create empty directory"));
  }

  OpenNextFile();
}


void
ffio::Writer::WritePacket(char *buf, uint32 buflen, uint32 bufsize,
                          const khTileAddr &addr)
{
  FFRecHeader hdr(buflen, addr.level, addr.col, addr.row);

  // pad buffer as dictated by the header
  size_t wlen = hdr.paddedLen();
  size_t reclen = (sizeof(hdr) + wlen);
  size_t fillNeeded = 0;
  if (wlen > buflen) {
    if (wlen > bufsize) {
      // we need to fill some zero for padding, but there isn't enough
      // room in the source buffer. Just set a flag and we'll weite
      // the padding separately
      fillNeeded = wlen - buflen;
    } else {
      // zero fill the padding in the existing buffer
      memset(&buf[buflen], 0, wlen - buflen);
    }
  }

  // increment our filesize counter
  // and open next subfile if this write will
  // push us over the edge
  totalOffset += reclen;
  fileOffset  += reclen;
  if (splitSize && (fileOffset > splitSize)) {
    fileOffset = reclen;
    OpenNextFile();
  }

  // the flat file header is in big endian, we fix the endianness in-place
  hdr.HostToBigEndian();
  if (!khWriteAll(fileHandle.fd(), &hdr, sizeof(hdr))) {
    throw khErrnoException
      (kh::tr("Failed to write ff tile header (lrc:%1,%2,%3) in %4")
       .arg(addr.level).arg(addr.row).arg(addr.col).arg(outdir));
  }

  size_t towrite = fillNeeded ? buflen : wlen;
  if (!khWriteAll(fileHandle.fd(), buf, towrite)) {
    throw khErrnoException
      (kh::tr
       ("Failed to write ff tile data (%1 bytes) (lrc:%2,%3,%4) in %5")
       .arg(towrite).arg(addr.level).arg(addr.row).arg(addr.col)
       .arg(outdir));
  }

  if (fillNeeded) {
    // make a zero filled buffer that is as big as any padding we will
    // ever need
    static std::vector<uchar> fillbuf(FFRecHeader::paddedLen(1));
    assert(fillNeeded < fillbuf.size());
    if (!khWriteAll(fileHandle.fd(), &fillbuf[0], fillNeeded)) {
      throw khErrnoException
        (kh::tr
         ("Failed to write padding (%1 bytes) (lrc:%2,%3,%4) in %5")
         .arg(fillNeeded).arg(addr.level).arg(addr.row).arg(addr.col)
         .arg(outdir));
    }
  }

  ++totalTilesWritten;
}


// ****************************************************************************
// ***  GridIndexedWriter
// ****************************************************************************
ffio::GridIndexedWriter::GridIndexedWriter(Type type,
                                           const std::string &outdir_,
                                           const khInsetCoverage &coverage,
                                           uint64 splitSize_,
                                           void* littleEndianTypeData) :
    ffio::Writer(type, outdir_, splitSize_),
    index(type, ffio::IndexFilename(outdir),
          coverage, littleEndianTypeData),
    presence(ffio::PresenceFilename(outdir), coverage)
{
}


void
ffio::GridIndexedWriter::WritePacket(char *buf, uint32 buflen, uint32 bufsize,
                                     const khTileAddr& addr)
{
  FFRecHeader hdr(buflen, addr.level, addr.col, addr.row);

  // pad buffer as dictated by the header
  size_t wlen = hdr.paddedLen();
  size_t reclen = (sizeof(hdr) + wlen);

  Writer::WritePacket(buf, buflen, bufsize, addr);
  index.AddTile(addr, buflen, reclen);
  presence.SetPresence(addr, true);
}
